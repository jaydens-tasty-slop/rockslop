/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "Game.h"

//==========================================================
Game::Game()
    : map( player.getPos() )
    , entitiesManager( &arduboy )
    , soundManager( &arduboy )
    , mode( GameMode::CINEMATIC )
{
    
}

//==========================================================
void Game::init()
{
    arduboy.boot();
    arduboy.display();
    arduboy.flashlight();
    arduboy.systemButtons();
    arduboy.audio.begin();
    arduboy.waitNoButtons();

    map.startLevel();
    player.levelStart();
    entitiesManager.spawnEntities( &map );
    soundManager.startMusic(69, 6, 22, 8);
}

//==========================================================
void Game::update()
{
    soundManager.update();
    if (!arduboy.nextFrame())
        return;

    arduboy.clear();
    arduboy.pollButtons();

    switch(mode)
    {
        /****************************PLAY/ANIM*******************************/
        case PLAY: 
        case ANIM:
        {
            //PRINT HUD
            arduboy.print(player.getLife());

            //MOVE OBJECTS
            if(mode == PLAY) //don't move player if in animation
            {
                player.move( &arduboy );
            }
            entitiesManager.moveEntities();

            //DRAW OBJECTS
            map.draw( &arduboy );
            player.draw( &arduboy );
            entitiesManager.drawEntities();
            dialogManager.draw( &arduboy );

            TriggerEvent event = entitiesManager.popTriggerEvent();
            if(event == TriggerEvent::END_LEVEL) //next level triggered
            {
                if(map.getCurrentLevel() == 4) //last level
                {
                    map.reset();
                    map.startLevel();
                    player.levelStart();
                    menu.setInGame(false);
                    entitiesManager.startNewLevel();
                    soundManager.stopMusic();
                    dialogManager.printSingleSentence(F("END OF TOME 1"));
                    mode = END;
                }
                else if(map.getCurrentLevel() == 1 && itemsManager.getCluesFound() != 0b00000011)
                {
                    if(warningShown == false)
                    {
                        dialogManager.printSingleSentence(F("no clue"));
                        warningShown = true;
                    }
                }
                else
                {
                    map.startNextLevel();
                    entitiesManager.startNewLevel();
                    soundManager.startMusic();
                    player.levelStart();
                }
            }
            else if(event == TriggerEvent::START_ANIM) //start animation
            {
                mode = ANIM;
            }
            else if(event == TriggerEvent::STOP_ANIM) //start animation
            {
                mode = PLAY;
            }
            else if(isStartDialogEvent(event))
            {
                mode = PLAY;
                dialogManager.printTextForTrigger(entitiesManager.getTriggerForEvent(event));
            }
            else if(arduboy.pressed( DOWN_BUTTON ) && !leftHouseTimer) //menu triggered
            {
                mode = GameMode::MENU;
                soundManager.stopMusic();
            }
            else if(arduboy.pressed( UP_BUTTON ) && map.checkPlayerIsOnADoor()) //up key pressed
            {
                mode = GameMode::HOUSE;
                boxView.reset();
            }
            
            if(player.getLife() <= 0 || (map.getCurrentLevel() >= 2 && entitiesManager.countNumberOfLiveEnemiesOfType(2) == 0))
            {
                map.startLevel();
                player.setLife(PLAYER_LIFE);
                player.levelStart();
                entitiesManager.startNewLevel();
                SoundManager::instance()->playSound(DEATH);
            }

            if(leftHouseTimer > 0) leftHouseTimer--;
        }
        break;

        /****************************MENU*******************************/
        case MENU:
            menu.update(&arduboy);

            if(menu.getSelectedOption() == MenuOption::GO)
            {
                if(menu.isInGame())
                {
                    soundManager.resumeMusic();
                }
                else
                {
                    soundManager.startMusic(69, 8, 22, 5);
                }
                
                mode = GameMode::PLAY;
                menu.setInGame(true);
                menu.clearSelectedOption();
            }

            break;

        /****************************HOUSE*******************************/
        case HOUSE:
            if( boxView.update(&arduboy) == false)
            {
                mode = GameMode::PLAY;
                leftHouseTimer = 50;
            }
            break;

       /****************************CINEMATIC*****************************/
        case CINEMATIC:

            if( boxView.updateCinematic(&arduboy) == false)
            {
                mode = GameMode::MENU;
            }
            break;


        /****************************END*********************************/
        case END:

            if( arduboy.pressed( B_BUTTON ))
            {
                mode = GameMode::MENU;
            }
            
            dialogManager.draw( &arduboy );

            break;
    }

    arduboy.display();
}
