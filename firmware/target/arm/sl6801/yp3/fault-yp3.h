/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FAULT_YP3_H
#define FAULT_YP3_H

/* "kind" argument of yp3_fault_record() (fault-yp3.c), passed by the entry
 * stubs in fault-entry-yp3.S. Plain integers: assembly includes this. */
#define FAULT_KIND_EXCEPTION 0  /* HardFault/MemManage/BusFault/UsageFault */
#define FAULT_KIND_UNHANDLED 1  /* vector without a handler */
#define FAULT_KIND_DIVZERO   2  /* division by zero (called, not an
                                   exception) */

#ifndef __ASSEMBLER__
#include <stdint.h>
/* Fill the retained fault record. frame_sp: the stacked exception frame
 * (unused for FAULT_KIND_DIVZERO); exc_return: EXC_RETURN or caller LR. */
void yp3_fault_record(uint32_t frame_sp, uint32_t exc_return, unsigned kind);
#endif

#endif
