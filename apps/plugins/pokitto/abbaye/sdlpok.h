#pragma once
# include "structs.h"
# include <stdint.h>
# include "Pokitto.h"
# include "gfx/gfx.h"

void SDL_RenderCopySolid(rect *srcFull, rect *dstFull);
void SDL_RenderCopyEx(rect *src, rect *dst, uint8_t flip);

void SDL_RenderCopy(rect *src, rect *dst);

void Mix_PlaySound(uint8_t sound);

void Mix_HaltMusic();
void Mix_PlayMusic(uint8_t music);
void Mix_PauseMusic();
void Mix_ResumeMusic();

void Pok_sleep(uint32_t ms);
