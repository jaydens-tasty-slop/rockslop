/* Anarch Rockbox frontend. CC0-1.0; see ../license. */
#ifndef ANARCH_ROCKBOX_PORT_H
#define ANARCH_ROCKBOX_PORT_H
#include "plugin.h"

/* Bound the indexed framebuffer on large screens. No second full framebuffer. */
#ifndef ANARCH_SCALE
#define ANARCH_SCALE MAX(1, (MAX(LCD_WIDTH, LCD_HEIGHT) + 191) / 192)
#endif
#define ANARCH_W ((LCD_WIDTH / ANARCH_SCALE) & ~1)
#define ANARCH_H (LCD_HEIGHT / ANARCH_SCALE)
#if ANARCH_W < 64 || ANARCH_H < 64
#error "Anarch needs at least 64 x 64 logical pixels; reduce ANARCH_SCALE"
#endif
#ifndef ANARCH_COLOR
#ifdef HAVE_LCD_COLOR
#define ANARCH_COLOR 1
#else
#define ANARCH_COLOR 0
#endif
#endif
#ifndef ANARCH_SOUND
#define ANARCH_SOUND 1
#endif
#define ANARCH_SAVE_SIZE 12
enum { K_UP, K_RIGHT, K_DOWN, K_LEFT, K_FIRE, K_BACK, K_MOD,
       K_JUMP, K_STRAFE_LEFT, K_STRAFE_RIGHT, K_MAP, K_FREELOOK,
       K_NEXT, K_PREV, K_MENU, K_CYCLE };
extern uint8_t anarch_pixels[ANARCH_H][ANARCH_W];
extern uint16_t anarch_keys;
extern bool anarch_io_error;
extern uint32_t anarch_time;
void anarch_init(void);
bool anarch_frame(void);
bool anarch_playing(void);
bool anarch_map(void);
bool anarch_menu_state(void);
bool anarch_has_level(void);
bool anarch_has_save(void);
unsigned anarch_max_level(void);
void anarch_menu_pick(unsigned item, unsigned level);
unsigned anarch_get_setting(unsigned which);
void anarch_set_setting(unsigned which, unsigned v);
void anarch_label(const char *text);
void anarch_text(unsigned x, unsigned y, const char *text);
uint16_t anarch_color(unsigned index);
bool anarch_assets_open(const char *path);
void anarch_assets_close(void);
bool anarch_load_save(uint8_t *data);
void anarch_store_save(const uint8_t *data);
void anarch_audio_init(void);
void anarch_audio_close(void);
void anarch_audio_sound(uint8_t sound, uint8_t volume);
void anarch_audio_music(uint8_t command);
#endif
