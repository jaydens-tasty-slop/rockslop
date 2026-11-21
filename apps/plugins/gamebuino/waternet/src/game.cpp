#include <stdint.h>
#include <Gamebuino-Meta.h>

#include "level.h"
#include "commonvars.h"
#include "helperfuncs.h"
#include "sound.h"
#include "printfuncs.h"
#include "cursor.h"
#include "savestate.h"
#include "palettes.h"

uint8_t paused, wasMusicOn, wasSoundOn,DrawLevelDoneBit, nextDrawWhat, levelDoneFrames;
constexpr uint8_t drwNone = 0;
constexpr uint8_t drwMoves = 1;
constexpr uint8_t drwUi = 2;
constexpr uint8_t drwLevel = 4;
constexpr uint8_t drwLevelDone = 8;
constexpr uint8_t drwPause = 16;
constexpr uint8_t drwPartialLevel = 32;
constexpr uint8_t maxLevelDoneFrames = 8 * frameRate / 15;


uint8_t drawGame(uint8_t drawWhat)
{
    if (drawWhat == drwNone)
        return drwNone;

    //background
    if(drawWhat & drwUi)
    {
        gb.display.setColor(INDEX_BLACK);
        gb.display.fillRect(maxBoardBgWidth * 8, 0, gb.display.width() - (maxBoardBgWidth * 8), gb.display.height());
         //LEVEL:
        printMessage(maxBoardBgWidth, 0, F("LEVEL:"));
        
        //[LEVEL NR] 2 chars
        printNumber(maxBoardBgWidth + 4, 1, selectedLevel, 2);
        
        //A:XXXXXX (XXXXXX="ROTATE" or XXXXXX="SLIDE " or XXXXXX="ROSLID")
        switch (gameMode)
        {
            case gmRotate:
                printMessage(maxBoardBgWidth, 4, F("a:"));
                printMessage(maxBoardBgWidth, 5, F("ROTATE"));
                break;
            case gmSlide:
                printMessage(maxBoardBgWidth, 4, F("a:"));
                printMessage(maxBoardBgWidth, 5, F("SLIDE"));
                break;
            case gmRotateSlide:
                printMessage(maxBoardBgWidth, 4, F("a:"));
                printMessage(maxBoardBgWidth, 5, F("ROSLID"));
                break;
        }

        //B:BACK
        printMessage(maxBoardBgWidth, 6, F("b:"));
        printMessage(maxBoardBgWidth, 7, F("BACK"));

        //MOVES:
        printMessage(maxBoardBgWidth, 2, F("MOVES:"));
    }

    if(drawWhat & drwMoves)
    {
        if(!(drawWhat & drwUi))
        {
             gb.display.setColor(INDEX_BLACK);
             gb.display.fillRect(gb.display.width() - (5*8), 3 * 8, 5*8, 8);
        }
        printNumber(maxBoardBgWidth + 1, 3, moves, 5);
    }

    if(drawWhat & drwLevel)
    {
        drawLevel();
        drawCursors();
    }

    //used when redrawing cursor stuff for example (not full background)
    if((drawWhat & drwPartialLevel) && !(drawWhat & drwLevel))
    {
        drawLevel(boardX -1 - posAdd, boardY -1 - posAdd, boardX + boardWidth + 1 + posAdd, boardY + boardHeight + 1 + posAdd);
        drawCursors();
    }
    
    if(drawWhat & drwLevelDone)
    {
        gb.display.setColor(INDEX_BLACK);
        gb.display.fillRect(((16 - 13) >> 1) * 8, ((maxBoardBgHeight >> 1) - 2) * 8, 14*8, 5*8);
        printMessage(((16 - 13) >> 1), (maxBoardBgHeight >> 1) - 2, F("[************]"));
        printMessage(((16 - 13) >> 1), (maxBoardBgHeight >> 1) - 1, F("| LEVEL DONE +"));
        printMessage(((16 - 13) >> 1), (maxBoardBgHeight >> 1)    , F("|            +"));
        printMessage(((16 - 13) >> 1), (maxBoardBgHeight >> 1) + 1, F("| a CONTINUE +"));
        printMessage(((16 - 13) >> 1), (maxBoardBgHeight >> 1) + 2, F("<############>"));
    }

    if(drawWhat & drwPause)
    {
        gb.display.setColor(INDEX_BLACK);
        gb.display.fillRect(0, ((maxBoardBgHeight >> 1) - 3) * 8, 16*8, 6*8);
        printMessage(0, (maxBoardBgHeight >> 1) - 3, F("[**************]"));
        printMessage(0, (maxBoardBgHeight >> 1) - 2, F("|PLEASE CONFIRM+")); 
        printMessage(0, (maxBoardBgHeight >> 1) - 1, F("|              +")); 
        printMessage(0, (maxBoardBgHeight >> 1) + 0, F("|   a PLAY     +"));
        printMessage(0, (maxBoardBgHeight >> 1) + 1, F("|   b TO QUIT  +"));
        printMessage(0, (maxBoardBgHeight >> 1) + 2, F("<##############>"));
    }
    
    return drwNone;
}

void initGame()
{
    paused = 0;
    SelectMusic(musGame);
    setPaletteGame();
    //set background tiles
    setBlockTilesAsBackground();
    //set sprite for selector / cursor
    initCursors();
    setCursorPos(0, boardX + selectionX, boardY + selectionY);
    showCursors();
    levelDoneFrames = 0;
    levelDone = 0;
    nextDrawWhat = drwUi + drwLevel + drwMoves;
}

void doPause()
{
    paused = 1;
    playMenuBackSound();
    wasSoundOn = isSoundOn();
    wasMusicOn = isMusicOn();
    setMusicOn(0);
    setSoundOn(0);
    hideCursors();
    nextDrawWhat |= drwPause;
}

void doUnPause()
{
    paused = 0;
    setMusicOn(wasMusicOn);
    setSoundOn(wasSoundOn);
    setCursorPos(0, boardX + selectionX, boardY + selectionY);
    showCursors();
    nextDrawWhat |= drwUi + drwLevel + drwMoves;
}

void game()
{
    if(gameState == gsInitGame)
    {
        initGame();
        gameState -= gsInitDiff;
    }

    nextDrawWhat = drawGame(nextDrawWhat);
    
    if(levelDone)
    {
        if(levelDoneFrames > 1)
            --levelDoneFrames;
        else if (levelDoneFrames == 1)
        {
            nextDrawWhat |= drwLevelDone;
            --levelDoneFrames;
        }
    }
    //don't redraw level anymore based on cursors if level is done
    else
        if (updateCursorFrame())
            nextDrawWhat |= drwPartialLevel;

    if (gb.buttons.released(BUTTON_DOWN))
    {
        if(!levelDone && !paused)
        {
            playGameMoveSound();
            //if not touching border on bottom
            if (selectionY + 1 < boardHeight + posAdd)
                selectionY += 1;
            else
            //set to border on top
                selectionY = -posAdd;
            setCursorPos(0, boardX + selectionX, boardY + selectionY);
            nextDrawWhat |= drwPartialLevel;
        }
    } 

    if (gb.buttons.released(BUTTON_UP))
    {
        if (!levelDone && !paused)
        {
            //if not touching border on top
            playGameMoveSound();
            if (selectionY -1 >= -posAdd)
                selectionY -= 1;
            else
            //set to border on bottom
                selectionY = boardHeight -1 +posAdd;
            setCursorPos(0, boardX + selectionX, boardY + selectionY);
            nextDrawWhat |= drwPartialLevel;
        }
    }

    if (gb.buttons.released(BUTTON_RIGHT))
    {
        if (!levelDone && !paused)
        {
            playGameMoveSound();
            //if not touching border on right
            if(selectionX + 1 < boardWidth + posAdd)
                selectionX += 1;
            else
            //set to border on left
                selectionX = -posAdd;
            setCursorPos(0, boardX + selectionX, boardY + selectionY);
            nextDrawWhat |= drwPartialLevel;
        }
    }

    if (gb.buttons.released(BUTTON_LEFT))
    {
        if(!levelDone && !paused)
        {
            playGameMoveSound();
            //if not touching border on left
            if( selectionX -1 >= -posAdd)
                selectionX -= 1;
            //set to border on right
            else
                selectionX = boardWidth -1 + posAdd;
            setCursorPos(0, boardX + selectionX, boardY + selectionY);
            nextDrawWhat |= drwPartialLevel;
        }
    }

    if (gb.buttons.released(BUTTON_A))
    {
        if(paused)
        {
            doUnPause();
            playMenuAcknowlege();
        }
        else
        {
            if(!levelDone)
            {
                if ((selectionX > -1) && (selectionX < boardWidth) &&
                    (selectionY > -1) && (selectionY < boardHeight))
                {   
                    if (gameMode != gmSlide)
                    {
                        rotateBlock((uint8_t)selectionX + ((uint8_t)selectionY * boardWidth));
                        moves++;
                        updateConnected();
                        playGameAction();
                        nextDrawWhat |= (drwPartialLevel+drwMoves);
                    }
                    else
                    {
                        playErrorSound();
                    }
                }
                else
                {
                    if ((selectionX > -1) && (selectionX < boardWidth))
                    {
                        if (selectionY == -1)
                        {
                            moveBlockDown((uint8_t)selectionX + ((uint8_t)(selectionY+1) * boardWidth));
                            moves++;
                            updateConnected();
                            playGameAction();
                            nextDrawWhat |= (drwPartialLevel+drwMoves);
                        }
                        else
                        {
                            if (selectionY == boardHeight)
                            {
                                moveBlockUp((uint8_t)selectionX + ((uint8_t)(selectionY-1) * boardWidth));
                                moves++;
                                updateConnected();
                                playGameAction();
                                nextDrawWhat |= (drwPartialLevel+drwMoves);
                            }
                        }
                    }
                    else
                    {
                        if ((selectionY > -1) && (selectionY < boardHeight))    
                        {
                            if (selectionX == -1)
                            {
                                moveBlockRight((uint8_t)(selectionX + 1) + ((uint8_t)selectionY * boardWidth));
                                moves++;
                                updateConnected();
                                playGameAction();
                                nextDrawWhat |= (drwPartialLevel+drwMoves);
                            }
                            else
                            {
                                if (selectionX == boardWidth)
                                {
                                    moveBlockLeft( (uint8_t)(selectionX - 1) + ((uint8_t)selectionY * boardWidth));
                                    updateConnected();
                                    moves++;
                                    playGameAction();
                                    nextDrawWhat |= (drwPartialLevel+drwMoves);
                                }
                            }
                        }
                        else
                        {
                            playErrorSound();
                        }
                    }
                }
                levelDone = isLevelDone(); 
                if(levelDone)
                {
                    levelDoneFrames = maxLevelDoneFrames;
                    SelectMusic(musLevelClear);
                    //hide cursor it's only sprite we use
                    hideCursors();
                }
            }
            else
            {
                if (levelDoneFrames == 0)
                {
                    //goto next level
                    if (difficulty == diffRandom)
                    {
                        //ned new seed based on time
                        randomSeedGame = getRandomSeed();
                        initLevel(randomSeedGame);
                        SelectMusic(musGame);
                        //show cursor again (it's actually to early but i'm not fixing that)
                        setCursorPos(0, boardX + selectionX, boardY + selectionY);
                        showCursors();
                        nextDrawWhat = drwUi + drwLevel + drwMoves;
                    }
                    else
                    {   
                        //goto next level if any
                        if (selectedLevel < maxLevel)
                        {
                            selectedLevel++;
                            unlockLevel(gameMode, difficulty, selectedLevel-1);
                            initLevel(randomSeedGame);
                            SelectMusic(musGame);
                            //show cursor again (it's actually to early but i'm not fixing that)
                            setCursorPos(0, boardX + selectionX, boardY + selectionY);
                            showCursors();
                            nextDrawWhat = drwUi + drwLevel + drwMoves;
                        }
                        else //Goto some congrats screen
                        {
                            gameState = gsInitLevelsCleared;
                        }
                    }
                }
            }
        }
    }

    if(gb.buttons.released(BUTTON_B)) 
    {
        if(!levelDone)
        {
            if(!paused)
            {
                doPause();
            }
            else
            {
                //need to enable early again to play backsound
                //normally unpause does it but we only unpause
                //after fade
                setSoundOn(wasSoundOn);
                hideCursors();
                playMenuBackSound();
                gameState = gsInitLevelSelect;
                doUnPause();
                //unpause sets cursor visible !
                hideCursors();
                //need to reset the level to initial state when going back to level selector
                //could not find a better way unfortunatly
                //also we do not want to reset the randomseed used for random level generating
                //or a new level would have been created when going back we only want the level
                //with random to change when pressing left and right in the level selector
                //this way it stays consistent with the normal levels
                //and the player can replay the level if he wants to
                initLevel(randomSeedGame);
            }
        }
    }
}
