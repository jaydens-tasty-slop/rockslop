/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _LEVELS_H_
#define _LEVELS_H_

namespace levels
{
    typedef enum __attribute__ ((packed))
    {
        VOID = 0,
        WALL = 1,
        WALL_BACK = 2,
        DOOR_UP = 3,
        BRICK = 4,
        FIRTREE_UPLEFT = 5,
        FIRTREE_UPRIGHT = 6,
        FIRTREE_DOWNLEFT = 7,
        FIRTREE_DOWNRIGHT = 8,
        DIRT = 9,
        DOOR_HOUSE_0 = 40, //NOTHING
        DOOR_HOUSE_1 = 41, //FIRST CLUE
        DOOR_HOUSE_2 = 42, //SECOND CLUE
        DOOR_HOUSE_3 = 43, //THIRD CLUE
        _ENEMY_1 = 51,
        _ENEMY_2 = 52,
        _CHARACTER_1 = 61,
        _CHARACTER_2 = 62,
        _CHARACTER_3 = 63,
        _ITEM_1 = 71,
        _TRIGGER_CHARACTER_1 = 81,
        _TRIGGER_CHARACTER_2 = 82,
        _TRIGGER_CHARACTER_3 = 83,
        _TRIGGER_CHARACTER_4 = 84,
        _TRIGGER_CHARACTER_5 = 85,
        _TRIGGER_CHARACTER_6 = 86,
        _TRIGGER_END_LEVEL = 99
    } Tile;

    struct Cell
    {
        Tile tile;
        byte count; // FF -> end of row
        //Cell(byte t, byte c) : tile(static_cast<Tile>(t)), count(c){} //commenting that out fixes fpermissive issues but mysteriously increases pgm size
    };
    static_assert (sizeof(Cell) == 2, "Size of Cell is not correct");

    union CellValue
    {
        uint16_t val;
        Cell c;
    };

    PROGMEM static const Cell LEVEL_0[] =
    {
        {0,47}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,39}, {51,1}, {0,7}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,38}, {9,2}, {0,6}, {4,1}, {1,0xFF},
        {0,24}, {5,1}, {6,1}, {0,10}, {9,1}, {0,10}, {1,0xFF},
        {0,24}, {7,1}, {8,1}, {0,4}, {51,1}, {0,4}, {9,2}, {0,5}, {9,1}, {0,4}, {1,0xFF},
        {0,23}, {9,4}, {0,8}, {9,2}, {0,4}, {9,4}, {0,2}, {1,0xFF},
        {0,17}, {51,1}, {0,4}, {9,5}, {0,8}, {9,10}, {0,2}, {0,0xFF},
        {0,21}, {9,6}, {0,4}, {1,1}, {0,3}, {9,11}, {0,1}, {99,0xFF},
        {0,14}, {9,13}, {0,8}, {9,11}, {0,1}, {0,0xFF},
        {0,5}, {81,1}, {0,1}, {61,1}, {0,5}, {9,14}, {0,3}, {51,1}, {0,4}, {9,12}, {1,0xFF},
        {0,12}, {9,15}, {0,7}, {9,13}, {1,0xFF}
    };
    PROGMEM static const Cell LEVEL_1[] =
    {
        {0,47}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,5}, {1,5}, {0,7}, {4,10}, {0,20}, {1,0xFF},
        {0,4}, {1,7}, {0,3}, {4,1}, {0,12}, {4,3}, {0,17}, {1,0xFF},
        {0,3}, {1,1}, {2,1}, {3,1}, {2,3}, {3,1}, {2,1}, {0,2}, {2,1}, {3,1}, {2,1}, {0,23}, {1,6}, {0,2}, {1,0xFF},
        {0,1}, {99,1}, {0,1}, {1,1}, {2,1}, {42,1}, {2,3}, {40,1}, {2,1}, {0,2}, {2,1}, {40,1}, {2,1}, {0,22}, {2,2}, {3,1}, {2,3}, {3,1}, {2,1}, {0,1}, {1,0xFF},
        {0,3}, {1,1}, {2,1}, {42,1}, {2,3}, {40,1}, {2,1}, {0,2}, {2,1}, {40,1}, {2,1}, {0,17}, {4,1}, {0,4}, {2,2}, {41,1}, {2,3}, {40,1}, {2,1}, {0,1}, {1,0xFF},
        {1,20}, {0,4}, {1,2}, {0,12}, {2,2}, {41,1}, {2,3}, {40,1}, {2,1}, {1,1}, {1,0xFF},
        {0,1}, {2,2}, {0,15}, {2,2}, {0,3}, {1,4}, {0,11}, {1,9}, {1,0xFF},
        {0,1}, {2,2}, {0,15}, {2,2}, {0,2}, {1,6}, {0,3}, {5,1}, {6,1}, {0,1}, {5,1}, {6,1}, {0,2}, {1,9}, {1,0xFF},
        {0,1}, {2,2}, {0,15}, {2,2}, {0,2}, {2,2}, {3,1}, {2,3}, {0,3}, {7,1}, {8,1}, {0,1}, {7,1}, {8,1}, {0,1}, {1,10}, {1,0xFF},
        {0,1}, {2,2}, {0,7}, {82,1}, {0,6}, {62,1}, {2,2}, {0,2}, {2,2}, {40,1}, {2,3}, {0,2}, {9,7}, {1,10}, {1,0xFF},
        {0,1}, {2,2}, {0,15}, {2,2}, {0,2}, {2,2}, {40,1}, {2,3}, {0,1}, {9,8}, {1,10}, {1,0xFF}
    };
    PROGMEM static const Cell LEVEL_2[] =
    {
        {0,40}, {4,1}, {0,6}, {1,0xFF},
        {0,40}, {4,1}, {0,6}, {1,0xFF},
        {0,3}, {1,18}, {0,19}, {4,1}, {0,6}, {1,0xFF},
        {0,9}, {1,1}, {0,13}, {4,1}, {0,3}, {4,1}, {0,5}, {4,1}, {0,1}, {4,2}, {0,1}, {4,3}, {0,5}, {52,1}, {1,0xFF},
        {0,8}, {99,1}, {1,1}, {0,31}, {52,1}, {0,5}, {1,0xFF},
        {0,9}, {1,1}, {0,37}, {1,0xFF},
        {0,2}, {4,1}, {0,1}, {4,2}, {0,25}, {4,1}, {0,15}, {1,0xFF},
        {0,29}, {52,1}, {0,4}, {52,1}, {0,10}, {1,2}, {1,0xFF},
        {0,23}, {52,1}, {0,16}, {52,1}, {0,3}, {1,3}, {1,0xFF},
        {0,43}, {2,2}, {3,1}, {2,1}, {1,0xFF},
        {0,43}, {2,2}, {43,1}, {2,1}, {1,0xFF},
        {0,6}, {83,1}, {0,1}, {63,1}, {0,17}, {9,10}, {0,7}, {2,2}, {43,1}, {2,1}, {1,0xFF},
        {0,14}, {71,1}, {0,4}, {9,28}, {9,0xFF}
    };
    PROGMEM static const Cell LEVEL_3[] =
    {
        {0,47}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,1}, {99,1}, {0,45}, {1,0xFF},
        {0,19}, {51,2}, {0,9}, {51,1}, {0,16}, {1,0xFF},
        {0,33}, {1,2}, {0,12}, {1,0xFF},
        {1,9}, {0,5}, {1,21}, {0,12}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,47}, {1,0xFF},
        {0,37}, {1,3}, {0,7}, {1,0xFF},
        {0,37}, {1,4}, {0,6}, {1,0xFF},
        {0,3}, {84,1}, {0,1}, {61,1}, {0,7}, {52,1}, {0,7}, {5,1}, {6,1}, {0,1}, {5,1}, {6,1}, {0,21}, {1,0xFF},
        {0,21}, {7,1}, {8,1}, {0,1}, {7,1}, {8,1}, {0,21}, {1,0xFF},
        {0,1}, {4,46}, {4,0xFF}
    };
    PROGMEM static const Cell LEVEL_4[] =
    {
        {0,47}, {1,0xFF},
        {0,45}, {99,1}, {0,1}, {1,0xFF},
        {0,10}, {1,16}, {0,21}, {1,0xFF},
        {0,25}, {1,1}, {0,12}, {4,1}, {0,8}, {1,0xFF},
        {0,5}, {4,1}, {0,1}, {4,2}, {0,16}, {1,1}, {0,21}, {1,0xFF},
        {0,4}, {4,1}, {0,20}, {1,1}, {0,2}, {51,2}, {0,17}, {1,0xFF},
        {0,25}, {1,1}, {0,13}, {4,1}, {0,7}, {1,0xFF},
        {0,14}, {4,5}, {1,1}, {0,5}, {1,1}, {0,15}, {51,1}, {0,5}, {1,0xFF},
        {0,19}, {1,1}, {0,3}, {52,1}, {0,1}, {1,1}, {0,21}, {1,0xFF},
        {0,9}, {4,1}, {0,2}, {4,2}, {0,5}, {1,1}, {0,5}, {1,1}, {0,16}, {4,1}, {0,4}, {1,0xFF},
        {0,19}, {1,2}, {0,4}, {1,1}, {0,21}, {1,0xFF},
        {0,2}, {4,3}, {0,18}, {52,1}, {0,1}, {1,1}, {0,4}, {5,1}, {6,1}, {0,2}, {85,1}, {0,4}, {63,1}, {0,6}, {86,1}, {1,0xFF},
        {0,25}, {1,1}, {0,4}, {7,1}, {8,1}, {0,15}, {1,0xFF}
    };

    static short getLevelSize(short level)
    {
        switch (level)
        {
            case 0: return arraysize(LEVEL_0);
            case 1: return arraysize(LEVEL_1);
            case 2: return arraysize(LEVEL_2);
            case 3: return arraysize(LEVEL_3);
            case 4: return arraysize(LEVEL_4);
            default: return 0;
        }
    }

    static Cell getCell(short level, short index)
    {
        CellValue cv = {0};
        switch(level)
        {
            case 0:  cv = {pgm_read_word(&LEVEL_0[index])}; break;
            case 1:  cv = {pgm_read_word(&LEVEL_1[index])}; break;
            case 2:  cv = {pgm_read_word(&LEVEL_2[index])}; break;
            case 3:  cv = {pgm_read_word(&LEVEL_3[index])}; break;
            case 4:  cv = {pgm_read_word(&LEVEL_4[index])}; break;
        }

        return cv.c;
    }

    static int8 getTileEnemyType(Tile t)
    {
        if(t >= _ENEMY_1 && t <= _ENEMY_2)
        {
            return t - _ENEMY_1 + 1;
        }
        return -1;
    }

    static bool isCharacterTile(Tile t)
    {
        return t >= _CHARACTER_1 && t <= _CHARACTER_3;
    }

    static int8 getTileItemId(Tile t)
    {
        if(t >= _ITEM_1 && t <= _ITEM_1)
        {
            return t - _ITEM_1 + 1;
        }
        return -1;
    }

    static int8 getTileTriggerId(Tile t)
    {
        if(static_cast<short> (t) >= _TRIGGER_CHARACTER_1 && static_cast<short> (t) <= _TRIGGER_END_LEVEL)
        {
            return t - _TRIGGER_CHARACTER_1 + 1;
        }
        return -1;
    }
    
}

#endif
