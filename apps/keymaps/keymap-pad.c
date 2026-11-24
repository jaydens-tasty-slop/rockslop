/* SPDX-License-Identifier: GPL-2.0-or-later */
/* A game controller's buttons (BUTTON_PAD_*, button.h), alike on every
 * target and screen: A is OK, B back, Options the screen's menu, PS the main
 * menu, L1/R1 page (skip tracks on Now Playing), L2/R2 volume (in lists
 * where the target has HAVE_VOLUME_IN_LIST), Y switches
 * between Now Playing and the browser, X stops. Its d-pad holds the target's
 * own directions, which the target's keymap maps. action.c looks pad buttons
 * up here instead of in the target's keymap. A acts on its release, so a
 * screen that A closes takes the release with it: none reaches a plugin's
 * own button loop behind it. */
#include "config.h"
#include "action.h"
#include "button.h"

static const struct button_mapping standard[] = {
    { ACTION_STD_OK, BUTTON_PAD_A|BUTTON_REL, BUTTON_PAD_A },
    { ACTION_STD_CANCEL, BUTTON_PAD_B, BUTTON_NONE },
    { ACTION_STD_CONTEXT, BUTTON_PAD_MENU, BUTTON_NONE },
    { ACTION_STD_MENU, BUTTON_PAD_HOME, BUTTON_NONE },
#ifdef HAVE_QUICKSCREEN
    { ACTION_STD_QUICKSCREEN, BUTTON_PAD_VIEW, BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST
};
static const struct button_mapping lists[] = {
    { ACTION_LISTTREE_PGUP, BUTTON_PAD_L1, BUTTON_NONE },
    { ACTION_LISTTREE_PGUP, BUTTON_PAD_L1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN, BUTTON_PAD_R1, BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN, BUTTON_PAD_R1|BUTTON_REPEAT, BUTTON_NONE },
#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLDOWN, BUTTON_PAD_L2, BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_PAD_L2|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_LIST_VOLUP, BUTTON_PAD_R2, BUTTON_NONE },
    { ACTION_LIST_VOLUP, BUTTON_PAD_R2|BUTTON_REPEAT, BUTTON_NONE },
#endif
    { ACTION_TREE_WPS, BUTTON_PAD_Y, BUTTON_NONE },
    { ACTION_TREE_STOP, BUTTON_PAD_X, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};
/* Now Playing: L1/R1 skip on release, seek while held */
static const struct button_mapping wps[] = {
    { ACTION_WPS_PLAY, BUTTON_PAD_A, BUTTON_NONE },
    { ACTION_WPS_BROWSE, BUTTON_PAD_B, BUTTON_NONE },
    { ACTION_WPS_BROWSE, BUTTON_PAD_Y, BUTTON_NONE },
    { ACTION_WPS_STOP, BUTTON_PAD_X, BUTTON_NONE },
    { ACTION_WPS_SKIPPREV, BUTTON_PAD_L1|BUTTON_REL, BUTTON_PAD_L1 },
    { ACTION_WPS_SEEKBACK, BUTTON_PAD_L1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_PAD_L1|BUTTON_REL, BUTTON_PAD_L1|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT, BUTTON_PAD_R1|BUTTON_REL, BUTTON_PAD_R1 },
    { ACTION_WPS_SEEKFWD, BUTTON_PAD_R1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_PAD_R1|BUTTON_REL, BUTTON_PAD_R1|BUTTON_REPEAT },
    { ACTION_WPS_VOLDOWN, BUTTON_PAD_L2, BUTTON_NONE },
    { ACTION_WPS_VOLDOWN, BUTTON_PAD_L2|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP, BUTTON_PAD_R2, BUTTON_NONE },
    { ACTION_WPS_VOLUP, BUTTON_PAD_R2|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_CONTEXT, BUTTON_PAD_MENU, BUTTON_NONE },
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_PAD_VIEW, BUTTON_NONE },
    { ACTION_WPS_MENU, BUTTON_PAD_HOME, BUTTON_NONE },
    LAST_ITEM_IN_LIST
};
static const struct button_mapping yesno[] = {
    { ACTION_YESNO_ACCEPT, BUTTON_PAD_A|BUTTON_REL, BUTTON_PAD_A },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};
/* Keyboard: A types, B deletes, Options is done, PS cancels, Y shows more
 * characters, L1/R1 move the cursor (as on game consoles) */
static const struct button_mapping keyboard[] = {
    { ACTION_KBD_SELECT, BUTTON_PAD_A, BUTTON_NONE },
    { ACTION_KBD_BACKSPACE, BUTTON_PAD_B, BUTTON_NONE },
    { ACTION_KBD_BACKSPACE, BUTTON_PAD_B|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP, BUTTON_PAD_Y, BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT, BUTTON_PAD_L1, BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT, BUTTON_PAD_L1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_PAD_R1, BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_PAD_R1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_DONE, BUTTON_PAD_MENU, BUTTON_NONE },
    { ACTION_KBD_ABORT, BUTTON_PAD_HOME, BUTTON_NONE },
    LAST_ITEM_IN_LIST
};
#if CONFIG_TUNER
/* FM radio: like Now Playing; L1/R1 change station */
static const struct button_mapping fm[] = {
    { ACTION_FM_PLAY, BUTTON_PAD_A, BUTTON_NONE },
    { ACTION_FM_EXIT, BUTTON_PAD_B, BUTTON_NONE },
    { ACTION_FM_STOP, BUTTON_PAD_X, BUTTON_NONE },
    { ACTION_STD_PREV, BUTTON_PAD_L1, BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_PAD_L1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT, BUTTON_PAD_R1, BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_PAD_R1|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC, BUTTON_PAD_L2, BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_PAD_L2|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_INC, BUTTON_PAD_R2, BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_PAD_R2|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_FM_MENU, BUTTON_PAD_MENU, BUTTON_NONE },
    { ACTION_FM_EXIT, BUTTON_PAD_HOME, BUTTON_NONE },
    LAST_ITEM_IN_LIST
};
#endif
#ifdef HAVE_RECORDING
static const struct button_mapping recscreen[] = {
    { ACTION_REC_PAUSE, BUTTON_PAD_A, BUTTON_NONE },
    { ACTION_STD_MENU, BUTTON_PAD_MENU, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};
#endif

const struct button_mapping *get_pad_mapping(int context)
{
    switch (context & ~CONTEXT_LOCKED) {
    case CONTEXT_WPS: return wps;
    case CONTEXT_YESNOSCREEN: return yesno;
    case CONTEXT_TREE:
    case CONTEXT_LIST:
    case CONTEXT_MAINMENU: return lists;
    case CONTEXT_KEYBOARD: return keyboard;
#if CONFIG_TUNER
    case CONTEXT_FM: return fm;
#endif
#ifdef HAVE_RECORDING
    case CONTEXT_RECSCREEN: return recscreen;
#endif
    default: return standard;
    }
}
