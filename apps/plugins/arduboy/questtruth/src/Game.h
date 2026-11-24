/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _GAME_H_
#define _GAME_H_

#include <Arduboy2.h>

#include "EntitiesManager.h"
#include "DialogManager.h"
#include "ItemsManager.h"
#include "SoundManager.h"
#include "Menu.h"
#include "BoxView.h"
#include "drawable/Player.h"
#include "drawable/Map.h"

typedef enum __attribute__ ((packed))
{
    PLAY,
    MENU,
    ANIM,
    HOUSE,
    CINEMATIC,
    END
} GameMode;

class Game
{
    public:	
        Game();
        void init();
        void update();

    private:
        Arduboy2 arduboy;

        Menu menu;
        Player player;
        Map map;
        EntitiesManager entitiesManager;
        DialogManager dialogManager;
        ItemsManager itemsManager;
        SoundManager soundManager;
        BoxView boxView;

        GameMode mode;
        int8 leftHouseTimer; //avoids mistakenly going to the menu when backing down from a house
        bool warningShown;
};

#endif
