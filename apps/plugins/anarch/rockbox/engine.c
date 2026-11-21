/* Original engine and artwork compiled separately from the Rockbox frontend.
 * All arithmetic is integer; CPU1 and the FPU are unused. CC0-1.0. */
#include "port.h"
#define SFG_AVR 0
#define SFG_EXTERNAL_LEVELS 1
#define SFG_SCREEN_RESOLUTION_X ANARCH_W
#define SFG_SCREEN_RESOLUTION_Y ANARCH_H
#define SFG_FPS 25
#define SFG_FOV_HORIZONTAL 240
#define SFG_SPRITE_MAX_SIZE ANARCH_W
#define SFG_RAYCASTING_SUBSAMPLE 2
#define SFG_RAYCASTING_MAX_STEPS 24
#define SFG_DITHERED_SHADOW 1
#define SFG_CAN_EXIT 1
#include "../game.h"

uint8_t anarch_pixels[ANARCH_H][ANARCH_W];
uint16_t anarch_keys;
uint32_t anarch_time;
static inline void SFG_setPixel(uint16_t x, uint16_t y, uint8_t c)
{
    anarch_pixels[y][x] = c;
}
int8_t SFG_keyPressed(uint8_t key) { return (anarch_keys >> key) & 1; }
void SFG_getMouseOffset(int16_t *x, int16_t *y) { *x = *y = 0; }
uint32_t SFG_getTimeMs(void) { return anarch_time; }
void SFG_sleepMs(uint16_t ms) { (void)ms; rb->yield(); }
void SFG_playSound(uint8_t sound, uint8_t volume)
{ anarch_audio_sound(sound, volume); }
void SFG_setMusic(uint8_t command) { anarch_audio_music(command); }
void SFG_processEvent(uint8_t event, uint8_t data) { (void)event; (void)data; }
void SFG_save(uint8_t data[SFG_SAVE_SIZE]) { anarch_store_save(data); }
uint8_t SFG_load(uint8_t data[SFG_SAVE_SIZE]) { return anarch_load_save(data); }
void anarch_init(void)
{
    /* Rockbox normally zeros BSS; reset explicitly for test/reentry as well. */
    rb->memset(&SFG_game, 0, sizeof(SFG_game));
    rb->memset(&SFG_player, 0, sizeof(SFG_player));
    rb->memset(&SFG_currentLevel, 0, sizeof(SFG_currentLevel));
    anarch_keys = 0; anarch_time = 0;
    SFG_init();
}
bool anarch_frame(void) { return SFG_mainLoopBody() != 0; }
bool anarch_playing(void) { return SFG_game.state == SFG_GAME_STATE_PLAYING; }
bool anarch_map(void) { return SFG_game.state == SFG_GAME_STATE_MAP; }
void anarch_label(const char *text)
{ SFG_drawText(text, 2, 2, 1, 7, 0, ANARCH_W - 2); }
void anarch_text(unsigned x, unsigned y, const char *text)
{ SFG_drawText(text, x, y, 1, 7, 0, ANARCH_W - 2); }
uint16_t anarch_color(unsigned index) { return paletteRGB565[index]; }
/* Hooks for the native Rockbox menu, which replaces the engine's own menu. */
bool anarch_menu_state(void) { return SFG_game.state == SFG_GAME_STATE_MENU; }
bool anarch_has_level(void) { return SFG_currentLevel.levelPointer != 0; }
bool anarch_has_save(void) { return (SFG_game.save[0] >> 4) != 0; }
unsigned anarch_max_level(void) { return SFG_game.save[0] & 0x0f; }
/* Select an engine menu item (SFG_MENU_ITEM_*); the caller then presses
 * K_FIRE for one frame to confirm it. */
void anarch_menu_pick(unsigned item, unsigned level)
{
    for (uint8_t i = 0; SFG_getMenuItem(i) != SFG_MENU_ITEM_NONE; ++i)
        if (SFG_getMenuItem(i) == item) SFG_game.selectedMenuItem = i;
    SFG_game.selectedLevel = level;
}
unsigned anarch_get_setting(unsigned which)
{
    return which ? (SFG_game.settings >> 2) & 3 : SFG_game.settings & 3;
}
void anarch_set_setting(unsigned which, unsigned v)
{
    uint8_t old = SFG_game.settings;
    SFG_game.settings = which ? (old & ~0x0c) | ((v & 3) << 2)
                              : (old & ~0x03) | (v & 3);
    if ((old ^ SFG_game.settings) & 0x02)
        SFG_setMusic((SFG_game.settings & 0x02) ? SFG_MUSIC_TURN_ON : SFG_MUSIC_TURN_OFF);
    SFG_game.save[1] = SFG_game.settings;
    SFG_gameSave();
}
