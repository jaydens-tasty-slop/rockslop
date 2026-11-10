/* YP3 ROM-recovery service payload.
 *
 * Adapted for rockslop-flasher from rockbox-yp3 yp3dev/payload/svc.c
 * (unchanged except OP_JEDEC, a read-only JEDEC ID read).
 *
 * Runs inside the boot ROM's USB RUNRAM handler (exec_ret), on a private
 * stack, in the 0x806000..0x808000 window that ROM leaves unused in both the
 * direct (Down+RESET) and watchdog-return ROM states.
 *
 * The host writes a struct svc_args at ARGS, then runs `exec_ret 0x806001 64`.
 * The 64-byte reply goes to the ROM reply buffer and is mirrored at RESULT.
 *
 * SPI sequencing (command words, ACK-first completion, WIP+WEL polling,
 * status preconditions) is carried over unchanged from the writer that was
 * physically validated in October 2026.
 */
#include <stdint.h>

#define R32(a)   (*(volatile uint32_t *)(uintptr_t)(a))
#define W32(a,v) (*(volatile uint32_t *)(uintptr_t)(a) = (uint32_t)(v))
#define R8(a)    (*(volatile uint8_t *)(uintptr_t)(a))
#define W8(a,v)  (*(volatile uint8_t *)(uintptr_t)(a) = (uint8_t)(v))

#define ARGS    0x806c00u
#define RESULT  0x806c40u
#define BOUNCE  0x806d00u           /* 256B SPI DMA bounce buffer */
#define STAGE   0x807000u           /* 4KiB sector stage */

#define SPIC    0x30000000u
#define DONE    2u
#define START   0x80000000u
#define COMMON  0x00a08000u
#define WINDOW  16384u

#define TIMER_LO 0x40082104u        /* 1 MHz free-running counter */
#define TIMER_HI 0x40082108u

#define PMU_CMD    0x40085104u
#define PMU_OUT    0x40085108u
#define PMU_IN     0x4008510cu
#define PMU_STATUS 0x40085110u

#define FLASH_MIN  0x0000e000u      /* never below: bootloader + partition table */
#define FLASH_MAX  0x001fc000u      /* never at/above: settings/calibration */
#define SECTOR     4096u
#define PAGE       256u
#define READ_CHUNK 64u

#define SVC_MAGIC   0x56533359u     /* "Y3SV" */
#define REPLY_MAGIC 0x50523359u     /* "Y3RP" */

enum op {
    OP_INFO = 0, OP_READ32 = 1, OP_WRITE32 = 2, OP_PMU_READ = 3,
    OP_PMU_WRITE = 4, OP_BOOT = 5, OP_RESET_TO_ROM = 6,
    OP_FLASH_CRC = 7, OP_FLASH_SECTOR = 8, OP_FLASH_READ = 9,
    OP_JEDEC = 10,
};

enum status {
    ST_OK = 0, ST_BAD_ARGS = 1, ST_SPI_BUSY = 2, ST_SPI_TIMEOUT = 3,
    ST_FLASH_STATE = 4, ST_WEL = 5, ST_WIP_TIMEOUT = 6, ST_VERIFY = 7,
    ST_STAGE_CRC = 8, ST_PMU = 9, ST_PMU_FLAGS = 10, ST_UNCHANGED = 11,
};

struct svc_args { uint32_t magic, op, a0, a1, a2; };

/* reply words live in the payload's spare RAM (0x806c80..0x806cc0, between
 * RESULT and BOUNCE) instead of .bss, so OP_JEDEC fits in the code window */
#define reply ((uint32_t *)0x806c80u)

static inline void barrier(void) { __asm volatile("dmb" ::: "memory"); }

static uint32_t now(void) { return R32(TIMER_LO); }

/* Busy-wait us microseconds on the 1MHz counter, with an iteration cap in
 * case the counter is not running. */
static void delay_us(uint32_t us)
{
    uint32_t t0 = now();
    for (uint32_t i = 0; i < 1000000u && now() - t0 < us; ++i)
        ;
}

/* ---- SPI NOR ---------------------------------------------------------- */

/* Stock ROM SPI pin/clock setup (ROM 0x7fb2 GPIO stores and selector-43
 * clock RMWs), as {address, clear mask, set value}. A mask with bit31 set
 * means a plain store to a set/clear alias register. */
static const uint32_t spi_setup_table[][3] = {
    {0x40081000u, 0x0000000fu, 0x00000002u},
    {0x40081038u, 0x80000000u, 0x00000001u},
    {0x40081014u, 0x00000003u, 0x00000001u},
    {0x40081024u, 0x00000003u, 0x00000000u},
    {0x4008102cu, 0x00000003u, 0x00000000u},
    {0x40081000u, 0x0000f000u, 0x00002000u},
    {0x40081038u, 0x80000000u, 0x00000008u},
    {0x40081014u, 0x000000c0u, 0x00000040u},
    {0x40081024u, 0x000000c0u, 0x00000080u},
    {0x4008102cu, 0x000000c0u, 0x00000080u},
    {0x40081000u, 0x000000f0u, 0x00000020u},
    {0x40081038u, 0x80000000u, 0x00000002u},
    {0x40081014u, 0x0000000cu, 0x00000004u},
    {0x40081024u, 0x0000000cu, 0x00000000u},
    {0x4008102cu, 0x0000000cu, 0x00000000u},
    {0x40081000u, 0x00000f00u, 0x00000200u},
    {0x40081038u, 0x80000000u, 0x00000004u},
    {0x40081014u, 0x00000030u, 0x00000010u},
    {0x40081024u, 0x00000030u, 0x00000000u},
    {0x4008102cu, 0x00000030u, 0x00000000u},
    {0x40081000u, 0x00f00000u, 0x00f00000u},
    {0x40081038u, 0x80000000u, 0x00000020u},
    {0x40081014u, 0x00000c00u, 0x00000400u},
    {0x40081024u, 0x00000c00u, 0x00000800u},
    {0x4008102cu, 0x00000c00u, 0x00000800u},
    {0x40081000u, 0x000f0000u, 0x000f0000u},
    {0x40081038u, 0x80000000u, 0x00000010u},
    {0x40081014u, 0x00000300u, 0x00000100u},
    {0x40081024u, 0x00000300u, 0x00000200u},
    {0x4008102cu, 0x00000300u, 0x00000200u},
    {0x400800a0u, 0x00000001u, 0x00000000u},
    {0x400800a0u, 0x00000f00u, 0x00000500u},
    {0x400800a0u, 0x00000000u, 0x00000001u},
};

/* Neither ROM state leaves the controller configured; replicate the setup
 * the stock bootloader/ROM 0x7ff8 performs (minus its CPSIE). */
static void spi_setup(void)
{
    W32(0x40080070u, R32(0x40080070u) | 2u);     /* gate 1 request */
    W32(0x40080060u, R32(0x40080060u) | 2u);     /* gate 1 ack */
    delay_us(200);
    for (unsigned i = 0; i < sizeof(spi_setup_table) / sizeof(spi_setup_table[0]); ++i) {
        uint32_t a = spi_setup_table[i][0], m = spi_setup_table[i][1], v = spi_setup_table[i][2];
        W32(a, (m & 0x80000000u) ? v : ((R32(a) & ~m) | v));
    }
    W32(SPIC, 1);
    W32(SPIC + 0x20, 0x02a09503u);
    W32(0x30000204u, R32(0x30000204u) | 0x100u);
    barrier();
}

static void spi_ensure(void)
{
    if (R32(SPIC) != 1u || R32(SPIC + 0x20) != 0x02a09503u)
        spi_setup();
}

/* 0 ok, else status. Stock ROM order: wait DONE, ack DONE, then wait for
 * DONE and START to retire. Never replay saved controller registers. */
static int transfer(uint32_t op, uint32_t addr, uint32_t count, uint32_t buf)
{
    uint32_t i;
    if ((R32(SPIC + 0x0c) & DONE) || (R32(SPIC + 0x10) & START))
        return ST_SPI_BUSY;
    W32(SPIC + 0x14, count - 1);
    W32(SPIC + 0x18, addr);
    W32(SPIC + 0x1c, buf);
    barrier();
    W32(SPIC + 0x10, op | COMMON | START);
    for (i = 0; i < WINDOW && !(R32(SPIC + 0x0c) & DONE); ++i) ;
    if (i == WINDOW) return ST_SPI_TIMEOUT;
    barrier();
    W32(SPIC + 0x0c, DONE);
    barrier();
    for (i = 0; i < WINDOW && (R32(SPIC + 0x0c) & DONE); ++i) ;
    if (i == WINDOW) return ST_SPI_TIMEOUT;
    for (i = 0; i < WINDOW && (R32(SPIC + 0x10) & START); ++i) ;
    if (i == WINDOW) return ST_SPI_TIMEOUT;
    barrier();
    return ST_OK;
}

static int status_reg(uint32_t op, uint8_t *out)
{
    W8(BOUNCE, 0xff);
    barrier();
    int r = transfer(op, 0, 1, BOUNCE);
    *out = R8(BOUNCE);
    return r;
}
#define RDSR1(p) status_reg(0x00400505u, (p))
#define RDSR2(p) status_reg(0x00400535u, (p))
#define RDCFG(p) status_reg(0x00400515u, (p))

static int flash_read(uint32_t addr, uint32_t buf)
{
    return transfer(0x00401503u, addr, READ_CHUNK, buf);
}

/* Validated idle state: SR1 == 0, SR2 == 2 (QE), no protection bits. */
static int flash_idle_ok(void)
{
    uint8_t sr1, sr2, cfg;
    int r;
    if ((r = RDSR1(&sr1)) || (r = RDSR2(&sr2)) || (r = RDCFG(&cfg)))
        return r;
    return (sr1 || sr2 != 2u || (cfg & 0x9cu)) ? ST_FLASH_STATE : ST_OK;
}

/* Wait until both WIP and WEL clear (SR1 & 3 == 0). */
static int wait_ready(uint32_t timeout_us)
{
    uint8_t sr1;
    uint32_t t0 = now();
    for (uint32_t i = 0; i < 1000000u; ++i) {
        int r = RDSR1(&sr1);
        if (r) return r;
        if (!(sr1 & 3u)) return ST_OK;
        if (now() - t0 > timeout_us) break;
    }
    return ST_WIP_TIMEOUT;
}

static int write_enable(void)
{
    uint8_t sr1;
    int r = transfer(0x00000106u, 0, 1, BOUNCE);
    if (r) return r;
    if ((r = RDSR1(&sr1))) return r;
    return sr1 == 2u ? ST_OK : ST_WEL;
}

static int erase_sector(uint32_t addr)
{
    int r;
    if ((r = flash_idle_ok()) || (r = write_enable())) return r;
    if ((r = transfer(0x00001120u, addr, 1, BOUNCE))) return r;
    if ((r = wait_ready(1000000u))) return r;
    return flash_idle_ok();
}

static int program_page(uint32_t addr, uint32_t src)
{
    int r;
    if ((r = flash_idle_ok()) || (r = write_enable())) return r;
    if ((r = transfer(0x00001502u, addr, PAGE, src))) return r;
    if ((r = wait_ready(100000u))) return r;
    return flash_idle_ok();
}

static uint32_t crc32_update(uint32_t c, uint32_t p, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) {
        c ^= R8(p + i);
        for (unsigned b = 0; b < 8; ++b)
            c = (c >> 1) ^ ((0u - (c & 1u)) & 0xedb88320u);
    }
    return c;
}

/* Compare flash [addr, addr+n) with memory at src (or with 0xff if !src). */
static int flash_compare(uint32_t addr, uint32_t src, uint32_t n, int *same)
{
    *same = 1;
    for (uint32_t off = 0; off < n; off += READ_CHUNK) {
        int r = flash_read(addr + off, BOUNCE);
        if (r) return r;
        for (uint32_t i = 0; i < READ_CHUNK; ++i) {
            uint8_t want = src ? R8(src + off + i) : 0xffu;
            if (R8(BOUNCE + i) != want) { *same = 0; return ST_OK; }
        }
    }
    return ST_OK;
}

static int is_blank(uint32_t p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i += 4)
        if (R32(p + i) != 0xffffffffu) return 0;
    return 1;
}

/* a0 = sector address, a1 = CRC32 of STAGE. reply[3] = pages programmed. */
static int op_flash_sector(uint32_t addr, uint32_t crc)
{
    int r, same;
    if ((addr & (SECTOR - 1)) || addr < FLASH_MIN || addr >= FLASH_MAX)
        return ST_BAD_ARGS;
    if ((crc32_update(0xffffffffu, STAGE, SECTOR) ^ 0xffffffffu) != crc)
        return ST_STAGE_CRC;
    if ((r = flash_compare(addr, STAGE, SECTOR, &same))) return r;
    if (same) return ST_UNCHANGED;
    if ((r = erase_sector(addr))) return r;
    if ((r = flash_compare(addr, 0, SECTOR, &same))) return r;
    if (!same) return ST_VERIFY;
    for (uint32_t pg = 0; pg < SECTOR; pg += PAGE) {
        if (is_blank(STAGE + pg, PAGE)) continue;
        if ((r = program_page(addr + pg, STAGE + pg))) return r;
        reply[3]++;
    }
    if ((r = flash_compare(addr, STAGE, SECTOR, &same))) return r;
    return same ? ST_OK : ST_VERIFY;
}

/* a0 = start (sector aligned), a1 = sector count; CRC32 per sector -> STAGE. */
static int op_flash_crc(uint32_t addr, uint32_t count)
{
    if ((addr & (SECTOR - 1)) || count == 0 || count > SECTOR / 4 ||
        addr + count * SECTOR > 0x200000u)
        return ST_BAD_ARGS;
    for (uint32_t s = 0; s < count; ++s) {
        uint32_t c = 0xffffffffu;
        for (uint32_t off = 0; off < SECTOR; off += READ_CHUNK) {
            int r = flash_read(addr + s * SECTOR + off, BOUNCE);
            if (r) return r;
            c = crc32_update(c, BOUNCE, READ_CHUNK);
        }
        W32(STAGE + s * 4, c ^ 0xffffffffu);
    }
    return ST_OK;
}

/* a0 = address, a1 = length (multiple of 64, <= 4KiB) -> STAGE. */
static int op_flash_read(uint32_t addr, uint32_t len)
{
    if ((len % READ_CHUNK) || len == 0 || len > SECTOR || addr + len > 0x200000u)
        return ST_BAD_ARGS;
    for (uint32_t off = 0; off < len; off += READ_CHUNK) {
        int r = flash_read(addr + off, STAGE + off);
        if (r) return r;
    }
    return ST_OK;
}

/* ---- PMU indexed bridge ---------------------------------------------- */

/* Each poll is spaced 500us; a tight poll loop was observed to fail. */
static int pmu_xfer(uint32_t index, int writing, uint32_t *value)
{
    uint32_t cmd = (index << 8) | (writing ? 0x40000060u : 0x40000061u);
    if ((R32(PMU_CMD) & START) || (R32(PMU_STATUS) & 0xf000u)) return ST_PMU;
    if (writing) W32(PMU_OUT, *value & 0xffu);
    W32(PMU_CMD, cmd);
    if (R32(PMU_CMD) != cmd) return ST_PMU;
    W32(PMU_CMD, cmd | START);
    for (unsigned poll = 0; poll < 64; ++poll) {
        delay_us(500);
        uint32_t v = R32(PMU_CMD);
        if (v & START) continue;
        if (v != cmd) return ST_PMU;
        if (!writing) *value = R32(PMU_IN) & 0xffu;
        return (R32(PMU_STATUS) & 0xf000u) ? ST_PMU : ST_OK;
    }
    return ST_PMU;
}

static int pmu_write_verify(uint32_t index, uint32_t value)
{
    uint32_t v = value;
    int r = pmu_xfer(index, 1, &v);
    if (r) return r;
    if ((r = pmu_xfer(index, 0, &v))) return r;
    return v == value ? ST_OK : ST_PMU;
}

static void counter_park(void) __attribute__((noreturn));
static void counter_park(void)
{
    /* A plain `b .` is not a reliable watchdog park; keep reading the bus. */
    for (;;) (void)R32(TIMER_LO);
}

/* Reset through PMU20 bit7. to_rom: arm the recovery reason first (PMU00
 * bit0 before PMU23 bits7:6, so a partial arm never requests the SD-update
 * reason) else clear it (PMU23 first) so the bootloader runs the NOR image. */
static int op_reset(int to_rom)
{
    uint32_t f23, f00, f20;
    int r;
    if ((r = pmu_xfer(0x23, 0, &f23)) || (r = pmu_xfer(0, 0, &f00)) ||
        (r = pmu_xfer(0x20, 0, &f20)))
        return r;
    reply[3] = f23; reply[4] = f00; reply[5] = f20;
    /* PMU00 bits 7:6 = 2 under stock and the soft-wake board setup */
    if ((f23 != 0 && f23 != 0xc0u) || ((f00 & ~0x80u) != 8u && (f00 & ~0x80u) != 9u) ||
        f20 != 0x2fu)
        return ST_PMU_FLAGS;
    if (to_rom) {
        if ((r = pmu_write_verify(0, f00 | 1u)) ||
            (r = pmu_write_verify(0x23, f23 | 0xc0u)))
            return r;
    } else {
        /* PMU00 back to Rockbox's 0x08: an image without the soft-wake
         * board setup reads bits 7:6 = 2 as a recovery request */
        if ((r = pmu_write_verify(0x23, f23 & ~0xc0u)) ||
            (r = pmu_write_verify(0, f00 & ~0xc1u)))
            return r;
    }
    if ((R32(PMU_CMD) & START)) return ST_PMU;
    __asm volatile("cpsid i" ::: "memory");
    W32(PMU_OUT, f20 | 0x80u);
    W32(PMU_CMD, 0x40002060u);
    if (R32(PMU_CMD) != 0x40002060u) {
        __asm volatile("cpsie i" ::: "memory");
        return ST_PMU;
    }
    W32(PMU_CMD, 0xc0002060u);
    counter_park();
}

/* ---- dispatch --------------------------------------------------------- */

void svc_main(uint32_t caller_msp, uint32_t descriptor)
{
    const volatile struct svc_args *a = (const volatile struct svc_args *)ARGS;
    uint32_t op = a->op, a0 = a->a0, a1 = a->a1;
    int st = ST_OK;

    for (unsigned i = 0; i < 16; ++i) reply[i] = 0;
    reply[0] = REPLY_MAGIC;
    reply[1] = op;

    if (a->magic != SVC_MAGIC) {
        st = ST_BAD_ARGS;
        op = ~0u;
    } else if (op >= OP_FLASH_CRC) {
        spi_ensure();
    }
    switch (op) {
    case OP_INFO: {
        uint32_t ipsr, primask, control;
        __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
        __asm volatile("mrs %0, primask" : "=r"(primask));
        __asm volatile("mrs %0, control" : "=r"(control));
        reply[3] = caller_msp; reply[4] = ipsr; reply[5] = primask;
        reply[6] = control; reply[7] = R32(0xe000ed08u); reply[8] = R32(0xe000ed00u);
        reply[9] = descriptor; reply[10] = now(); reply[11] = R32(TIMER_HI);
        reply[12] = R32(SPIC + 0x0c); reply[13] = R32(SPIC + 0x10);
        reply[14] = R32(SPIC + 0x20); reply[15] = R32(0x40083000u);
        break;
    }
    case OP_READ32:  reply[3] = R32(a0); break;
    case OP_WRITE32: W32(a0, a1); __asm volatile("dsb"); reply[3] = R32(a0); break;
    case OP_PMU_READ: reply[3] = 0; st = pmu_xfer(a0 & 0xff, 0, &reply[3]); break;
    case OP_PMU_WRITE: st = pmu_write_verify(a0 & 0xff, a1 & 0xff); break;
    case OP_BOOT:         st = op_reset(0); break;
    case OP_RESET_TO_ROM: st = op_reset(1); break;
    case OP_FLASH_CRC:    st = op_flash_crc(a0, a1); break;
    case OP_FLASH_SECTOR: st = op_flash_sector(a0, a1); break;
    case OP_FLASH_READ:   st = op_flash_read(a0, a1); break;
    case OP_JEDEC:      /* RDID 0x9f, 3 bytes: the no-address read form of RDSR */
        W32(BOUNCE, 0xffffffffu);
        st = transfer(0x0040059fu, 0, 3, BOUNCE);
        reply[3] = R32(BOUNCE);
        break;
    case ~0u: break;
    default: st = ST_BAD_ARGS; break;
    }

    reply[2] = (uint32_t)st;
    for (unsigned i = 0; i < 16; ++i) {
        W32(descriptor + 8 + i * 4, reply[i]);
        W32(RESULT + i * 4, reply[i]);
    }
}
