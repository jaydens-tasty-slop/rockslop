/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "Map.h"
#include "../Tiles.h"

//=============================================================
Map::Map(const Position & playerPos)
    : playerPosition(playerPos)
    , current_level(0)
    , scroll_x(0)
    , scroll_y(0)
    , level_length(0)
    , snowflakes_frame_count(0)
{
}

//=============================================================
void Map::draw(Arduboy2 * arduboy)
{
    //COMPUTE SCROLL_Y BASED ON PLAYER POSITION
    scroll_y = playerPosition.y + PLAYER_HEIGHT - MID_HEIGHT;
    if(scroll_y < 0)
    {
        scroll_y = 0;
    }
    else if(scroll_y + SCREEN_HEIGHT > LEVEL_HEIGHT_PIXELS)
    {
        scroll_y = LEVEL_HEIGHT_PIXELS - SCREEN_HEIGHT;
    }

    //COMPUTE SCROLL_X BASED ON PLAYER POSITION
    scroll_x = playerPosition.x - MID_WIDTH;
    if(scroll_x < 0)
    {
        scroll_x = 0;
    }
    else if(scroll_x + SCREEN_WIDTH > level_width * TILE_LENGTH)
    {
        scroll_x = level_width * TILE_LENGTH - SCREEN_WIDTH;
    }

    // DRAW SNOWFLAKES
    drawSnowflakes(scroll_x, scroll_y, arduboy);

    //DRAW TILES
    short x = 0;
    short y = 0;

    for(short index = 0; index < level_length; index ++)
    {        
        levels::Cell c = levels::getCell(current_level, index);
        
        if(c.count == 0xFF)
        {
            drawTile(c.tile, x, y, arduboy); 
                        
            y += TILE_LENGTH;
            x = 0;
        }
        else
        {
            short stopX = x + c.count * TILE_LENGTH;
            for(; x < stopX; x += TILE_LENGTH)
            {
                drawTile(c.tile, x, y, arduboy);                
            }
        }
    }
}

//=============================================================
void Map::drawTile(levels::Tile tile, short x, short y, Arduboy2 * arduboy)
{
    short screenX = x - scroll_x;
    short screenY = y - scroll_y;
    if(screenX > -TILE_LENGTH && screenX <= SCREEN_WIDTH && screenY > -TILE_LENGTH && screenY <= SCREEN_HEIGHT)
    {
        const uint8_t * bitmap;
       
        switch(tile)
        {
            default:
                return;
            case levels::Tile::WALL:
                bitmap = tiles::WALL;
                break;
            case levels::Tile::WALL_BACK:
                bitmap = tiles::WALL_BACK;
                break;
            case levels::Tile::DOOR_UP:
                bitmap = tiles::DOOR_UP;
                break;
            case levels::Tile::BRICK:
                bitmap = tiles::BRICK;
                break;   
            case levels::Tile::FIRTREE_UPLEFT:
                bitmap = tiles::FIRTREE_UPLEFT;
                break;
            case levels::Tile::FIRTREE_UPRIGHT:
                bitmap = tiles::FIRTREE_UPRIGHT;
                break;
            case levels::Tile::FIRTREE_DOWNLEFT:
                bitmap = tiles::FIRTREE_DOWNLEFT;
                break;
            case levels::Tile::FIRTREE_DOWNRIGHT:
                bitmap = tiles::FIRTREE_DOWNRIGHT;
                break;
            case levels::Tile::DIRT:
                bitmap = tiles::DIRT;
                break;
            case levels::Tile::_TRIGGER_END_LEVEL:
                static int circleSize = 0;
                circleSize = (circleSize + 1) % (TILE_LENGTH / 2);
                arduboy->drawCircle(screenX, screenY, circleSize);
                return;
        }

        arduboy->drawBitmap(screenX, screenY, bitmap, TILE_LENGTH, TILE_LENGTH);
    }
}

//=============================================================
void Map::drawSnowflakes(short scrollX, short scrollY, Arduboy2 * arduboy)
{
    ++snowflakes_frame_count;
    bool move_snowflakes = false;
    if(snowflakes_frame_count >= SNOWFLAKES_FRAME_MOVE)
    {
        snowflakes_frame_count = 0;
        move_snowflakes = true;
    }

    for(short i = 0; i < SNOWFLAKES_NUMBER; i ++)
    {
        if(snowflakes[i].visible && arduboy != nullptr)
        {
            arduboy->drawPixel(snowflakes[i].pos.x - scroll_x, snowflakes[i].pos.y - scroll_y);
        }

        if(move_snowflakes)
        {
            snowflakes[i].pos.y++;

            short rand = random(0,4);
            if(rand == 0)
            {
                snowflakes[i].pos.x--;
            }
            else if(rand == 1)
            {
                snowflakes[i].pos.x++;
            }

            if(getTile(current_level, snowflakes[i].pos.y / TILE_LENGTH, snowflakes[i].pos.x / TILE_LENGTH) != levels::Tile::VOID)
            {
                snowflakes[i].visible = false;
            }
            else if(snowflakes[i].pos.y >= LEVEL_HEIGHT_PIXELS)
            {
                snowflakes[i].pos.x = random(0, level_width * TILE_LENGTH);
                snowflakes[i].pos.y = 0;
                snowflakes[i].visible = true;
            }
        }
    }
}

//=============================================================
void Map::startLevel()
{
    level_length = levels::getLevelSize(current_level);
    level_width = 1; //precount the last cell of the row
    bool parseWidth = true;

    short row = 0;
    level_row_indexes[0] = 0;

    for(short index = 0; index < level_length; index ++)
    {
        levels::Cell c = levels::getCell(current_level, index);
        
        if(c.count == 0xFF)
        {      
            parseWidth = false;
            if(index < level_length - 1)
            {
                ++row;
                level_row_indexes[row] = index + 1;
            }
        }
        else if(parseWidth)
        {
            level_width += c.count;
        }
    }

    for(short i = 0; i< SNOWFLAKES_NUMBER; ++i)
    {
        snowflakes[i].pos.x = random(0, level_width * TILE_LENGTH);
        snowflakes[i].pos.y = random(0, LEVEL_HEIGHT_PIXELS);
        snowflakes[i].visible = true;
    }

    for(short i=0; i < 400; ++i) //forces snow to fall until it reaches ground at least once
    {
        drawSnowflakes(0, 0, nullptr);
    }
}

//=============================================================
void Map::startNextLevel()
{
    current_level++;
    startLevel();
}

//=============================================================
bool Map::checkCollision(short x, short y, short w, short h) const
{
    return checkCollisionForPoint(x             , y         )   //1
        || checkCollisionForPoint(x + w / 2     , y         )   //2      1-----2-----3
        || checkCollisionForPoint(x + w         , y         )   //3      |           |
        || checkCollisionForPoint(x             , y + h / 2 )   //4      4           7
        || checkCollisionForPoint(x             , y + h     )   //5      |           |
        || checkCollisionForPoint(x + w / 2     , y + h     )   //6      5-----6-----8
        || checkCollisionForPoint(x + w         , y + h / 2 )   //7
        || checkCollisionForPoint(x + w         , y + h     );  //8
}

//=============================================================
bool Map::checkCollisionForPoint(short x, short y) const
{
    short tile_i = y / TILE_LENGTH;
    short tile_j = x / TILE_LENGTH;
    
    levels::Tile tile = getTile(current_level, tile_i, tile_j);
    return tile == levels::Tile::WALL || tile == levels::Tile::DIRT || tile == levels::Tile::BRICK;
}

//=============================================================
bool Map::checkPlayerIsOnADoor() const
{
    return getCurrentDoorNumber() >= 0;
}

//=============================================================
short Map::getCurrentDoorNumber() const
{
    short tile_i = playerPosition.y / TILE_LENGTH + 1;
    short tile_j = playerPosition.x / TILE_LENGTH;

    levels::Tile tile = getTile(current_level, tile_i, tile_j);

    short doorNb = tile - levels::Tile::DOOR_HOUSE_0;
    if(doorNb > 3)
    {
        return -1;
    }
    return doorNb;
}

//=============================================================
short Map::getScrollX() const
{
    return scroll_x;
}

//=============================================================
short Map::getScrollY() const
{
    return scroll_y;
}

//=============================================================
short Map::getLevelLength() const
{
    return level_length;
}

//=============================================================
short Map::getCurrentLevel() const
{
    return current_level;
}

//=============================================================
void Map::reset()
{
    current_level = 0;
}


//=============================================================
levels::Tile Map::getTile(short level, short i, short j) const
{
    short colCount = 0;
    short index = level_row_indexes[i];
    short stopI = index + level_width;
    for(; index < stopI; ++index)
    {
        levels::Cell c = levels::getCell(current_level, index);

        colCount += (c.count == 0xFF) ? 1 : c.count;
        if(colCount >= j + 1)
        {
            return c.tile;
        }
        
    }
    return levels::Tile::VOID;
}
