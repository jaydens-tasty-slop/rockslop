#pragma once
#include <stdint.h>

#include "Pokitto.h"

void initsd();
void closesd();
void initrooms();
void loadBackgroundImage(const char *img);
void loadenemiesroom(uint8_t r,uint16_t enemiesdata[7][15]);
void saveroom(uint8_t r,uint16_t roomdata[26][35]);
void loadroom(uint8_t r,uint16_t roomdata[26][35]);
