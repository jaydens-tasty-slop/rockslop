/* RUMBLO artwork, indexed colour (index 0 transparent); see gen_assets.py. */
#ifndef RUMBLO_ASSETS_H
#define RUMBLO_ASSETS_H

#define RUMBLO_COLORS 132
#define RUMBLO_TILES 28

#define SCREEN_ARENA0 0 /* arena 1 has three animation frames: 1..3 */
#define SCREEN_ARENA1 1
#define SCREEN_WIN 4
#define SCREEN_LOSE 5

/* RGB triples */
extern const unsigned char rumblo_palette[RUMBLO_COLORS * 3];
/* 16x16 tiles, row major */
extern const unsigned char rumblo_tiles[RUMBLO_TILES][256];
/* 20x15 tile indices per screen */
extern const unsigned char rumblo_screens[6][300];
/* characters: stand, move 1, move 2, in the air; 32x48 facing right */
extern const unsigned char rumblo_char[2][4][32 * 48];
/* 8x8 projectile */
extern const unsigned char rumblo_weapon[2][64];
/* 56x56 */
extern const unsigned char rumblo_shield[56 * 56];

#endif
