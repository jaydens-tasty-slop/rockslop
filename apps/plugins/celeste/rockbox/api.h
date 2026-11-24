/* CEleste Rockbox bridge. MIT; see ../LICENSE. No C++ runtime API required. */
#ifndef CE_ROCKBOX_API_H
#define CE_ROCKBOX_API_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { CE_LEFT, CE_RIGHT, CE_UP, CE_DOWN, CE_JUMP, CE_DASH };
extern const uint8_t ce_map[8192], ce_tiles[4096], ce_flags[128], ce_font[480];
extern const uint8_t ce_rgb[16][3];
extern const int16_t ce_sine[65];
extern uint8_t ce_frame[8192], ce_screen_palette[16];
extern int ce_fault;
void ce_zero(void *p, size_t n);
void ce_init(void);
void ce_shutdown(void);
/* Returns whether this step accepted player input (freeze does not). */
int ce_step(unsigned keys);
void ce_status(int *level, int *deaths, int *fruit, int *dashes, int *time);
void ce_new_game(int practice);
void ce_change_room(int delta);
void ce_retry(void);
int ce_practice(void);
int ce_best_time(void);
int ce_title(void);
unsigned ce_save_export(uint8_t *out);
int ce_save_import(const uint8_t *in, unsigned size);
void ce_text(int x, int y, const char *s, unsigned color);
void ce_clear(unsigned color);
unsigned ce_pixel(unsigned x, unsigned y);
#ifdef __cplusplus
}
#endif
#endif
