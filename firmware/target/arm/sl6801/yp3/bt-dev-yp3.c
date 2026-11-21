/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Dev shell "bt PART ARGS" (YP3_DEV builds): hands ARGS to the part's own
 * handler. "bt stack ..." is the shared shell's (apps/dev_shell.c). */
#include <string.h>
#include "bt-yp3.h"

/* "bt PART ...": the handler for each PART, given the rest of the line */
static const struct {
    const char *name;
    bool (*handler)(char *arg);
} parts[] = {
    { "rf",   bt_rf_yp3_dev },
    { "sbc",  bt_sbc_yp3_dev },
    { "ctrl", bt_ctrl_yp3_dev },
    { "hci",  bt_hci_yp3_dev },
};

/* Split off PART and dispatch; false for an unknown PART or bad usage. */
bool bt_yp3_dev_cmd(char *arg)
{
    char *rest = strchr(arg, ' ');
    if (rest)
        *rest++ = '\0';
    for (unsigned i = 0; i < sizeof parts / sizeof parts[0]; i++)
        if (!strcmp(arg, parts[i].name))
            return parts[i].handler(rest ? rest : arg + strlen(arg));
    return false;
}
