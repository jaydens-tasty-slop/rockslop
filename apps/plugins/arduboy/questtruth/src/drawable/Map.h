/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _MAP_H_
#define _MAP_H_

#include <Arduboy2.h>

#include "../Constants.h"
#include "../abstract/Drawable.h"
#include "../abstract/Singleton.h"
#include "../Levels.h"

//COLLISION CHECK RESULT
typedef struct __attribute__ ((packed))
{
    Position pos;
    bool visible;
} Snowflake;


class Map : public Drawable, public Singleton<Map>
{
    public:
        Map(const Position & playerPos);
        void draw( Arduboy2 * arduboy );
        void startLevel();
        void startNextLevel();

        bool checkCollision(short x, short y, short w, short h) const;
        bool checkCollisionForPoint(short x, short y) const;
        bool checkPlayerIsOnADoor() const;
        short getCurrentDoorNumber() const; // will be < 0 if none

        short getScrollX() const;
        short getScrollY() const;
        short getLevelLength() const;
        short getCurrentLevel() const;
        void reset();
    private:
        void drawTile(levels::Tile tile, short screenX, short screenY, Arduboy2 * arduboy);
        void drawSnowflakes(short scrollX, short scrollY, Arduboy2 * arduboy);
        levels::Tile getTile(short level, short i, short j) const;

        const Position & playerPosition;

        short current_level;
        short scroll_x, scroll_y;
        short level_length, level_width;

        byte level_row_indexes[LEVEL_HEIGHT];
        Snowflake snowflakes[SNOWFLAKES_NUMBER];
        int8 snowflakes_frame_count;
};

#endif
