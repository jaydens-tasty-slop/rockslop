/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "usb_class_driver.h"

void usb_serial_send(const unsigned char *data, int length);
int usb_serial_send_space(void);
#ifdef HAVE_DEVLOG
/* Bytes queued and in transit, and sends the driver refused (data left
 * "in transit" with no transfer to complete it) */
void usb_serial_dev_state(int *queued, int *in_transit, unsigned *refused);
#endif
#ifdef USB_SERIAL_RX_HOOK
/* Target hook for received data, called from the USB thread. */
void USB_SERIAL_RX_HOOK(const unsigned char *data, int length);
#endif

extern struct usb_class_driver usb_cdrv_serial;

#endif
