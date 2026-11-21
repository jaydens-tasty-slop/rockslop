/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "power.h"
#include "usb.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "dev-yp3.h"

/* Fills the endpoint type table; without it usb_core cannot allocate bulk
 * endpoints and silently drops mass storage from the configuration. */
void usb_init_device(void)
{
    usb_drv_startup();
}

/* Called from the USB tick: external power (PMU[0x2a] bit 2, cached by the
 * power thread) is the only cable indication; it cannot tell a PC from a
 * charger. Dev builds keep USB off unless the dev config asks for it, so the
 * permanently attached dev cable does not start mass storage. */
int usb_detect(void)
{
#ifdef YP3_DEV
    if (!dev_usb_enabled)
        return USB_EXTRACTED;
#endif
    return power_input_present() ? USB_INSERTED : USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if (on)
        usb_core_init();
    else
        usb_core_exit();
}
