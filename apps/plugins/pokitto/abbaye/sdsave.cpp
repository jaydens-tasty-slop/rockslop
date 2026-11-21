/* Rockbox port of the SD card access: files are below the game's data dir
 * (.rockbox/rocks/games/pokitto_abbaye/) through the layer's file API. */
#include "sdsave.h"

static const char *pathMap = "abbaye/map.bin";
static const char *pathMapSaved = "abbaye/mapsave.bin";
static const char *pathEnemies = "abbaye/enemies.bin";

void initsd()
{
}

void closesd()
{
}

/* Full-screen background images are raw 220x176 4bpp pictures. */
void loadBackgroundImage(const char *img)
{
    char path[64] = "abbaye/gfx/";
    strcat(path, img);
    strcat(path, ".gfx");
    int fd = pk_file_open(path);
    if (fd < 0)
        return;
    pk_file_read(fd, Pokitto::Display::screenbuffer, 0x4BA0);
    pk_file_close(fd);
}

/* Starts a new game: the map in play is a copy of the original. */
void initrooms()
{
    int src = pk_file_open(pathMap);
    int dst = pk_file_create(pathMapSaved, 1);
    char line[512];
    int r;

    if (src >= 0 && dst >= 0)
        while ((r = pk_file_read(src, line, sizeof(line))) > 0)
            pk_file_write(dst, line, r);
    if (src >= 0)
        pk_file_close(src);
    if (dst >= 0)
        pk_file_close(dst);
}

void saveroom(uint8_t room, uint16_t roomdata[26][35])
{
    int fd = pk_file_create(pathMapSaved, 0);
    if (fd >= 0) {
        pk_file_seek(fd, (long)room * 35 * 22 * 2);
        pk_file_write(fd, roomdata, 2 * 22 * 35);
        pk_file_close(fd);
    }
}

void loadroom(uint8_t room, uint16_t roomdata[26][35])
{
    int fd = pk_file_open(pathMapSaved);
    if (fd >= 0) {
        pk_file_seek(fd, (long)room * 35 * 22 * 2);
        pk_file_read(fd, roomdata, 2 * 22 * 35);
        pk_file_close(fd);
    }
}

void loadenemiesroom(uint8_t room, uint16_t enemiesdata[7][15])
{
    for (uint8_t j = 0; j < 7; j++)
        for (uint8_t k = 0; k < 15; k++)
            enemiesdata[j][k] = 0;

    int fd = pk_file_open(pathEnemies);
    if (fd >= 0) {
        pk_file_seek(fd, (long)room * 7 * 15 * 2);
        pk_file_read(fd, enemiesdata, 2 * 7 * 15);
        pk_file_close(fd);
    }
}
