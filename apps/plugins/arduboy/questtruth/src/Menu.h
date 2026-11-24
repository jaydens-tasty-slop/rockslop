/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#ifndef _MENU_H_
#define _MENU_H_

#include <Arduboy2.h>

#include "Constants.h"

typedef enum __attribute__ ((packed))
{
    NONE,
    GO,
    ABOUT,
    CLUES,
    INVENTORY
} MenuOption;

class Menu
{
    public:
        Menu();
        void update(Arduboy2 * arduboy);
        void setInGame(bool inGame);
        bool isInGame() const;
        const MenuOption getSelectedOption() const;
        void clearSelectedOption();
    private:
       void displayClues(Arduboy2 * arduboy);
       void displayInventory(Arduboy2 * arduboy);
       void printFromProgmem(Arduboy2 * arduboy, const char * textInProgMem);
       inline short getNbItems() const;
       
       MenuOption selectedOption;
       int8 selection;
       bool inGame;
};

#endif
