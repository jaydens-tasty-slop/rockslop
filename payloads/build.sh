#!/bin/sh
# Rebuild the on-device payloads. They are committed prebuilt, so building
# the flasher needs no ARM toolchain.
#   yp3/svc.bin    clang + ld.lld + llvm-objcopy (as yp3dev/payload/build.sh)
#   ts17/*.bin     arm-none-eabi-gcc (as ts17dev/adfu.py), only if $TC is set:
#                  TC=/path/to/bin/arm-none-eabi- payloads/build.sh
set -e
cd "$(dirname "$0")"
CF="--target=arm-none-eabi -mcpu=cortex-m4 -mthumb -Os -ffreestanding -fno-builtin -nostdlib -Wall -Wextra"
o=$(mktemp -d)
clang $CF -c yp3/svc.c -o $o/svc.o
clang $CF -c yp3/entry.S -o $o/entry.o
ld.lld -T yp3/svc.ld --gc-sections $o/entry.o $o/svc.o -o $o/svc.elf
llvm-objcopy -O binary --only-section=.text $o/svc.elf yp3/svc.bin
if [ -n "$TC" ]; then
    ${TC}gcc -mcpu=cortex-m4 -mthumb -c ts17/hook.S -o $o/hook.o
    ${TC}objcopy -O binary -j .text $o/hook.o ts17/hook.bin
    ${TC}gcc -mcpu=cortex-m4 -mthumb -Os -ffreestanding -nostdlib -fno-builtin -Wall \
        -T ts17/gadget.ld ts17/ops.c -o $o/ops.elf
    ${TC}objcopy -O binary $o/ops.elf ts17/ops.bin
fi
rm -rf $o
ls -l yp3/svc.bin ts17/hook.bin ts17/ops.bin
