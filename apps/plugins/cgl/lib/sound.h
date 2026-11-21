#ifndef CGL_SOUND_H
#define CGL_SOUND_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#include <stdbool.h>

EXTERNC bool isSoundEnabled;
EXTERNC bool isPlayingBgm;
EXTERNC void initSound(char *title, char *description, int soundSeed);
EXTERNC void updateSound(void);
EXTERNC void playSoundEffect(int type);

#endif