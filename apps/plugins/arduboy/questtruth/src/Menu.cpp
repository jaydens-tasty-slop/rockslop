/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "Menu.h"
#include "ItemsManager.h"
#include "SoundManager.h"

namespace
{
    static const short MENU_CURSOR_LEFT_X = 50;
    static const short MENU_CURSOR_TOP_Y = 25;
    static const short MENU_INGAME_CURSOR_TOP_Y = 15;

    static const short CLUES_CURSOR_LEFT_X = 5;
    static const short CLUES_CURSOR_TOP_Y = 15;

    static const short INVENTORY_CURSOR_LEFT_X = 5;
    static const short INVENTORY_CURSOR_TOP_Y = 15;

    static const short LINE_HEIGHT = 15;
    static const short NB_ITEMS_INGAME = 3;
    static const short NB_ITEMS_PREGAME = 2;
    static const short SELECTION_BALL_WIDTH = 5;
    static const short SELECTION_BALL_UP_PADDING = 3;

    PROGMEM static const char NOTHING_YET_SENTENCE[] = "Nothing yet";
}

//=============================================================
Menu::Menu()
    : selection( 1 )
    , inGame( false )
{
}

//=============================================================
void Menu::update(Arduboy2 * arduboy)
{
    if( arduboy->justPressed(B_BUTTON) )
    {
        selectedOption = NONE;
    }

    switch(selectedOption)
    {
        case ABOUT: 
            arduboy->println(F("Timmy O'Toole"));
            return;
        case CLUES:
            displayClues(arduboy);
            return;
        case INVENTORY:
            displayInventory(arduboy);
            return;
    }

    short initCursorY;

    if(inGame)
    {
        initCursorY = MENU_INGAME_CURSOR_TOP_Y;
        short cursorY = initCursorY;
        arduboy->setCursor(MENU_CURSOR_LEFT_X, cursorY);
        arduboy->println(F("RESUME"));
        cursorY += LINE_HEIGHT;
        arduboy->setCursor(MENU_CURSOR_LEFT_X, cursorY);
        arduboy->println(F("CLUES"));
        cursorY += LINE_HEIGHT;
        arduboy->setCursor(MENU_CURSOR_LEFT_X, cursorY);
        arduboy->println(F("INVENTORY"));
    }
    else
    {
        initCursorY = MENU_CURSOR_TOP_Y;
        arduboy->setCursor(MENU_CURSOR_LEFT_X, initCursorY);
        arduboy->println(F("PLAY"));
        arduboy->setCursor(MENU_CURSOR_LEFT_X, initCursorY + LINE_HEIGHT);
        arduboy->println(F("ABOUT"));
    }

    if( arduboy->justPressed( DOWN_BUTTON ) )
    {
        if(selection < getNbItems())
        {
            selection++;
            SoundManager::instance()->playSound(DOWN);
        }
    }
    else if( arduboy->justPressed(UP_BUTTON) )
    {
        if(selection > 1)
        {
            selection--;
            SoundManager::instance()->playSound(UP);
        }
    }
    if( arduboy->justPressed(A_BUTTON) )
    {
        if(selection == 1)
        {
            selectedOption = GO;
        }
        if(inGame)
        {
            if(selection == 2)
            {
                selectedOption = MenuOption::CLUES;
            }
            else if(selection == 3)
            {
                selectedOption = MenuOption::INVENTORY;
            }
        }
        else //start
        {
           if(selection == 2)
            {
                selectedOption = MenuOption::ABOUT;
            }
        }
        SoundManager::instance()->playSound(OK);
    }

    short ballY = initCursorY + SELECTION_BALL_UP_PADDING + LINE_HEIGHT * (selection - 1);
    arduboy->drawCircle(MENU_CURSOR_LEFT_X - SELECTION_BALL_WIDTH, ballY, SELECTION_BALL_WIDTH / 2);
}

//=============================================================
void Menu::displayClues(Arduboy2 * arduboy)
{
    short lineY = CLUES_CURSOR_TOP_Y;
    for(short clueId=1; clueId <= NUMBER_OF_CLUES; ++clueId)
    {
        if(ItemsManager::instance()->hasClue( clueId ))
        {
            arduboy->setCursor(CLUES_CURSOR_LEFT_X, lineY); 
            arduboy->print(F("-"));
            printFromProgmem(arduboy, (PGM_P)pgm_read_word(&(ALL_CLUES[clueId - 1])));

            lineY += LINE_HEIGHT;
        }
    }

    if(lineY == CLUES_CURSOR_TOP_Y)
    {
        arduboy->setCursor(CLUES_CURSOR_LEFT_X, CLUES_CURSOR_TOP_Y); 
        printFromProgmem(arduboy, NOTHING_YET_SENTENCE);
    }
}

//=============================================================
void Menu::displayInventory(Arduboy2 * arduboy)
{
    arduboy->setCursor(INVENTORY_CURSOR_LEFT_X, INVENTORY_CURSOR_TOP_Y);

    if((ItemsManager::instance()->getItems() & 0b00000001))
    {
        arduboy->print(F("-Pepper spray"));
    } 
    else
    {
        printFromProgmem(arduboy, NOTHING_YET_SENTENCE);
    }
}

//=============================================================
void Menu::printFromProgmem(Arduboy2 * arduboy, const char * textInProgMem)
{
    PGM_P p = (PGM_P)pgm_read_word(&(textInProgMem));

    char tBuffer[strlen_P(p)];
    strcpy_P(tBuffer, textInProgMem);
    arduboy->print(tBuffer);
}

//=============================================================
void Menu::setInGame(bool inGame)
{
    this->inGame = inGame;
}

//=============================================================
bool Menu::isInGame() const
{
    return this->inGame;
}

//=============================================================
const MenuOption Menu::getSelectedOption() const
{   
    return selectedOption;
}

//=============================================================
void Menu::clearSelectedOption()
{   
    selectedOption = NONE;
}

//=============================================================
short Menu::getNbItems() const
{
    if(inGame)
    {
        return NB_ITEMS_INGAME;
    }
    return NB_ITEMS_PREGAME;
}
