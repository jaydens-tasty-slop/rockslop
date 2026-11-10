/* ROM RUNRAM entry: r4 = ROM USB descriptor (0x800de8). Switch to a private
 * stack, run svc_main, publish a 64-byte reply length and return to ROM. */
.syntax unified
.cpu cortex-m4
.thumb
.section .entry,"ax"
.global _start
.thumb_func
_start:
    push {r4-r11, lr}
    ldr r1, =0x800de8
    cmp r4, r1
    bne 1f
    add r0, sp, #36          @ caller MSP before our push
    mov r5, sp
    ldr r2, =0x806c00
    mov sp, r2
    mov r1, r4
    bl svc_main
    mov sp, r5
    movs r1, #64
    strh r1, [r4, #6]
1:  pop {r4-r11, pc}
.ltorg
