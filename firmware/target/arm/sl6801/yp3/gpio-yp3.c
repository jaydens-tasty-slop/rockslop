/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Pad configuration (translated from the ROM routine) and pad ownership. */
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "system.h"
#include "gpio-yp3.h"
#include "mmio-sl6801.h"
#ifdef YP3_DEV
#include "dev-yp3.h"
#endif


/* Every pad the firmware configures, and the one driver that owns it. */
static const struct {
    uint8_t bank, pin, owner;
} yp3_pins[] __attribute__((section(".irodata"))) = {
    { 1, 0, PIN_OWNER_LCD }, { 1, 1, PIN_OWNER_LCD }, { 1, 2, PIN_OWNER_LCD },
    { 1, 3, PIN_OWNER_LCD }, { 1, 4, PIN_OWNER_LCD }, { 1, 5, PIN_OWNER_LCD },
    { 1, 10, PIN_OWNER_LCD }, { 1, 18, PIN_OWNER_LCD },
    { 3, 1, PIN_OWNER_LCD }, { 3, 2, PIN_OWNER_LCD }, { 3, 3, PIN_OWNER_LCD },
    { 3, 10, PIN_OWNER_LCD },
    { 3, 4, PIN_OWNER_SD }, { 3, 5, PIN_OWNER_SD }, { 3, 6, PIN_OWNER_SD },
    { 3, 7, PIN_OWNER_SD }, { 3, 8, PIN_OWNER_SD }, { 3, 9, PIN_OWNER_SD },
    { 1, 11, PIN_OWNER_TUNER }, { 1, 13, PIN_OWNER_TUNER },
    { 3, 11, PIN_OWNER_TUNER },
    { 1, 7, PIN_OWNER_AUDIO }, { 1, 9, PIN_OWNER_AUDIO },
    { 1, 14, PIN_OWNER_AUDIO }, { 1, 15, PIN_OWNER_AUDIO },
    { 1, 20, PIN_OWNER_AUDIO }, { 1, 21, PIN_OWNER_AUDIO },
    { 1, 30, PIN_OWNER_AUDIO },
    { 1, 17, PIN_OWNER_BACKLIGHT },
    { 3, 0, PIN_OWNER_KEYS }, { 3, 12, PIN_OWNER_KEYS },
    { 2, 2, PIN_OWNER_KEYS },
    { 1, 22, PIN_OWNER_USB }, { 1, 23, PIN_OWNER_USB },
};

__attribute__((section(".icode"))) static bool
pin_allowed(enum yp3_pin_owner owner, uint32_t desc)
{
    uint32_t bank = YP3_PIN_BANK(desc), pin = YP3_PIN_NUM(desc);
    for (unsigned i = 0; i < sizeof yp3_pins / sizeof yp3_pins[0]; i++) {
        if (yp3_pins[i].bank != bank || yp3_pins[i].pin != pin)
            continue;
        if (yp3_pins[i].owner == owner || owner == PIN_OWNER_DEV) {
#ifdef YP3_DEV
            if (owner == PIN_OWNER_DEV)
                dev_log("pin GPIO%lu.%lu: dev override of owner %u",
                            (unsigned long)bank, (unsigned long)pin,
                            (unsigned)yp3_pins[i].owner);
#endif
            return true;
        }
#ifdef YP3_DEV
        dev_log("pin GPIO%lu.%lu REFUSED: owner %u, requested by %u",
                    (unsigned long)bank, (unsigned long)pin,
                    (unsigned)yp3_pins[i].owner, (unsigned)owner);
#endif
        return false;
    }
#ifdef YP3_DEV
    dev_log("pin GPIO%lu.%lu %s: not in the board pin map",
                (unsigned long)bank, (unsigned long)pin,
                owner == PIN_OWNER_DEV ? "allowed (dev)" : "REFUSED");
#endif
    return owner == PIN_OWNER_DEV;
}

__attribute__((section(".icode"))) static void
set_field(uintptr_t address, uint32_t shift, uint32_t width_mask,
          uint32_t value)
{
    uint32_t v = REG(address);
    REG(address) = (v & ~(width_mask << shift)) |
                   ((value & width_mask) << shift);
}

bool yp3_pin_write(enum yp3_pin_owner owner, uint32_t desc, bool high)
{
    if (!pin_allowed(owner, desc))
        return false;
    uint32_t bank = YP3_PIN_BANK(desc) & 3u;
    REG(high ? GPIO_SET(bank) : GPIO_CLR(bank)) = 1u << YP3_PIN_NUM(desc);
    return true;
}

__attribute__((section(".icode"))) bool
yp3_pin_config(enum yp3_pin_owner owner, uint32_t desc)
{
    /* Pull codes 4..15 select (field A, field B) pairs: A in bits 1:0,
     * B in bits 17:16; codes 0..3 mean no pull. */
    static const uint32_t pull_fields[12] = {
        0x80020001u, 0x80030001u, 0x80000000u, 0x80000000u,
        0x80000002u, 0x80000000u, 0x80000000u, 0x80020002u,
        0x80000003u, 0x80010003u, 0x80020003u, 0x80030003u
    };
    if (!pin_allowed(owner, desc))
        return false;
    uint32_t bank = YP3_PIN_BANK(desc) & 3u;
    uint32_t pin = YP3_PIN_NUM(desc);
    uint32_t pull = YP3_PIN_PULL(desc);
    uint32_t fields = pull < 4u ? 0x80000000u : pull_fields[pull - 4u];
    uint32_t shift = GPIO_FIELD_SHIFT(pin);

    /* The mux/dir/pull words are shared by up to 16 pads of different
     * owners, and the key scan reconfigures GPIO3.12 from the tick: keep
     * each read-modify-write whole. */
    int irq = disable_irq_save();
    set_field(GPIO_MUX(bank, pin), GPIO_MUX_SHIFT(pin), 0xfu,
              YP3_PIN_MUX(desc));
    REG(YP3_PIN_LEVEL(desc) ? GPIO_SET(bank) : GPIO_CLR(bank)) = 1u << pin;
    set_field(GPIO_DIR(bank, pin), shift, 3u, YP3_PIN_DIR(desc));
    set_field(GPIO_PULL_A(bank, pin), shift, 3u, fields & 3u);
    set_field(GPIO_PULL_B(bank, pin), shift, 3u, (fields >> 16) & 3u);
    restore_irq(irq);
    return true;
}
