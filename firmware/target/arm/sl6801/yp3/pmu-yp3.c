/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PMU indexed bridge access (bounded polls). Registers and protocol:
 * sl6801.h "PMU bridge" and pmu-yp3.h. */
#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "kernel.h"
#include "pmu-yp3.h"
#include "mmio-sl6801.h"

#define PMU_COMMAND REG(PMU_BRIDGE_CMD)
#define PMU_WDATA   REG(PMU_BRIDGE_WDATA)
#define PMU_RDATA   REG(PMU_BRIDGE_RDATA)
#define PMU_STATUS  REG(PMU_BRIDGE_STATUS)
#define PMU_BUSY    PMU_BRIDGE_START    /* START reads back set while busy */

#define POLL_INTERVAL_US  500u      /* between busy polls (see pmu_command) */
#define POLL_BUDGET_MAX   64u       /* cap: 64 * 500 us = 32 ms per command */

static volatile bool pmu_locked;

bool yp3_pmu_try_lock(void)
{
    int oldlevel = disable_irq_save();
    bool acquired = !pmu_locked;
    if (acquired)
        pmu_locked = true;
    restore_irq(oldlevel);
    return acquired;
}

void yp3_pmu_unlock(void)
{
    int oldlevel = disable_irq_save();
    pmu_locked = false;
    restore_irq(oldlevel);
}

/* Load the command, read it back (a mismatch means the
 * bridge did not take it), then set START. Poll for completion at 500 us
 * intervals (as yp3dev does): reading the result right after the busy bit
 * clears returned the previous transaction's data.
 * budget = maximum number of polls. On completion the command word reads
 * back without START; anything else is treated as a failure. */
static bool command_start(uint32_t command)
{
    if (PMU_COMMAND & PMU_BUSY)
        return false;
    PMU_COMMAND = command;
    if (PMU_COMMAND != command)
        return false;
    PMU_COMMAND = command | PMU_BUSY;
    return true;
}

static uint32_t command_word(uint8_t index, bool write)
{
    return ((uint32_t)index << PMU_BRIDGE_CMD_INDEX_SHIFT) |
           (write ? PMU_BRIDGE_CMD_WRITE : PMU_BRIDGE_CMD_READ);
}

static bool pmu_command(uint32_t command, uint32_t budget)
{
    if (!command_start(command))
        return false;
    if (budget > POLL_BUDGET_MAX)
        budget = POLL_BUDGET_MAX;
    do {
        sl6801_udelay(POLL_INTERVAL_US);
        uint32_t v = PMU_COMMAND;
        if (!(v & PMU_BUSY))
            return v == command;
    } while (--budget);
    return false;
}

bool yp3_pmu_read(uint8_t index, uint32_t poll_budget, uint8_t *value)
{
    if (!pmu_command(command_word(index, false), poll_budget))
        return false;
    *value = (uint8_t)PMU_RDATA;
    return !(PMU_STATUS & PMU_BRIDGE_STATUS_ERROR);
}

bool yp3_pmu_write(uint8_t index, uint32_t value, uint32_t poll_budget)
{
    PMU_WDATA = value;
    if (!pmu_command(command_word(index, true), poll_budget))
        return false;
    return !(PMU_STATUS & PMU_BRIDGE_STATUS_ERROR);
}

static uint32_t async_command;
static bool async_read;

bool yp3_pmu_async_start(uint8_t index, bool write, uint8_t value)
{
    if (write)
        PMU_WDATA = value;
    async_command = command_word(index, write);
    async_read = !write;
    return command_start(async_command);
}

int yp3_pmu_async_done(uint8_t *value)
{
    uint32_t v = PMU_COMMAND;
    if (v & PMU_BUSY)
        return 0;
    if (v != async_command || (PMU_STATUS & PMU_BRIDGE_STATUS_ERROR))
        return -1;
    if (async_read)
        *value = (uint8_t)PMU_RDATA;
    return 1;
}
