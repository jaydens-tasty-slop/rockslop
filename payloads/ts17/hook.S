/* Copied unchanged from rockbox-yp3 ts17dev/gadgets/ for rockslop-flasher. */
/* Resident hook for the ADFU ROM. The switch command (0x20) does not call its
 * target once: it registers it, and the ROM's USB loop calls it continuously
 * (from ROM 0x1a98, ~750k calls/s). So switch is pointed once at this stub,
 * which does nothing until the host arms it:
 *
 *   CTL+0 ARM   host writes ARM_MAGIC last; the stub clears it, then calls FN
 *   CTL+4 FN    gadget entry (Thumb bit added here)
 *   CTL+8 DONE  set to DONE_MAGIC after FN returns
 *
 * Gadgets are loaded while the stub is idle, so the ROM never runs code that
 * is still being written. Position independent (literal pool only); throwaway
 * probe tooling.
 *
 * Every call also re-arms the boot-to-ADFU magic (0x4000c03c = 0x42520100,
 * read and cleared by mbrec), so while this hook is installed any wedge or
 * reset comes back in download mode: the ROM's watchdog fires ~11 s after
 * its loop stops, and the player re-enumerates as 10d6:10d6 ~0.6 s later. */
    .syntax unified
    .thumb
    .text
    .global hook
    .thumb_func
hook:
    ldr     r1, =0x4000c03c         @ retained boot-mode register
    ldr     r2, =0x42520100         @ "BR" + type 0x0100: enter ADFU
    str     r2, [r1]
    ldr     r1, =0x01019980         @ CTL
    ldr     r2, [r1]
    ldr     r3, =0xa4a4c0de         @ ARM_MAGIC
    cmp     r2, r3
    bne     1f
    push    {r4, lr}
    movs    r2, #0
    str     r2, [r1]                @ disarm before running
    dsb
    ldr     r3, [r1, #4]
    orr     r3, r3, #1
    blx     r3
    ldr     r1, =0x01019980
    ldr     r2, =0xd0d0d0d0         @ DONE_MAGIC
    str     r2, [r1, #8]
    dsb
    pop     {r4, pc}
1:  bx      lr
    .ltorg
