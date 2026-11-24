/* Declarations shared by the Rockbox front end of Terrario. */
#pragma once

#define TR_DIR ROCKBOX_DIR "/rocks/games/terrario"
#define TR_SAVE_PATH TR_DIR "/world.sav"
#define TR_DATA_PATH TR_DIR "/terrario.dat"

/* Reads the world size out of the save file. */
bool tr_save_peek(int *width, int *height);
