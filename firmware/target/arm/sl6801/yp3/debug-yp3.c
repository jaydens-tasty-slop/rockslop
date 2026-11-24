/* SPDX-License-Identifier: GPL-2.0-or-later */
/* System > Debug > View HW info */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "power.h"
#include "powermgmt.h"
#include "core_alloc.h"
#include "storage.h"
#include "mmio-sl6801.h"
#include "pmu-yp3.h"

bool dbg_hw_info(void)
{
    lcd_setfont(FONT_SYSFIXED);
    while (1) {
        int line = 0;
        struct storage_info info;
        lcd_clear_display();
        lcd_putsf(0, line++, "SoC   SL6801 (2x M4)");
        lcd_putsf(0, line++, "CPU   %ld MHz", cpu_frequency / 1000000);
        lcd_putsf(0, line++, "CLK   %08lx", (unsigned long)REG(CLK_CPU));
        lcd_putsf(0, line++, "Batt  %d mV", _battery_voltage());
        lcd_putsf(0, line++, "Power %s%s",
                  power_input_status() ? "USB" : "battery",
                  charging_state() ? ", charging" : "");
        lcd_putsf(0, line++, "PMU boot 24=%02x", yp3_pmu_boot_regs[0]);
        lcd_putsf(0, line++, " 26-29 %02x %02x %02x %02x",
                  yp3_pmu_boot_regs[1], yp3_pmu_boot_regs[2],
                  yp3_pmu_boot_regs[3], yp3_pmu_boot_regs[4]);
        lcd_putsf(0, line++, "Heap  %lu KB free",
                  (unsigned long)core_available() / 1024);
        lcd_putsf(0, line++, "      %lu KB largest",
                  (unsigned long)core_allocatable() / 1024);
        storage_get_info(0, &info);
        lcd_putsf(0, line++, "SD    %lu MB",
                  (unsigned long)((uint64_t)info.num_sectors *
                                  info.sector_size >> 20));
        lcd_putsf(0, line++, "Up    %ld s", current_tick / HZ);
        lcd_update();
        int btn = button_get_w_tmo(HZ / 2);
        if (btn == BUTTON_LEFT || btn == BUTTON_SELECT)
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}
