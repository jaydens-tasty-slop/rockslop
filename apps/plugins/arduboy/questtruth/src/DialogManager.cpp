/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "DialogManager.h"
#include "drawable/Map.h"
#include "ItemsManager.h"
#include "SoundManager.h"

namespace
{
    static const short DEFAULT_TRIGGER_TEXT_PADDING_X = 30;
    static const short TRIGGER_TEXT_PADDING_Y = 20;

    size_t getStringLength(PGM_P p)
    {
        size_t n = 0;
        while (1) {
            unsigned char c = pgm_read_byte(p++);
            if (c == 0) break;
            n++;
        }
        return n;
    }

}

//======================================================================
DialogManager::DialogManager()
    : currentSentenceSize(0)
    , currentLineIndex(0)
    , currentTrigger(nullptr)
    , extraTextPaddingX(0)
    , sound(false)
{
    
}

//======================================================================
void DialogManager::printTextForTrigger(Trigger * trigger)
{
    currentTrigger = trigger;
    currentLineIndex = 0;
    printNextLineForTrigger();
    SoundManager::instance()->stopMusic();
    sound = true;
}

//======================================================================
void DialogManager::printNextLineForTrigger()
{
    extraTextPaddingX = DEFAULT_TRIGGER_TEXT_PADDING_X;
    const __FlashStringHelper * sentencePtr;

    switch(currentTrigger->id)
    {
        case 1:
            currentNbOfLines = 2;
            switch(currentLineIndex)
            {
                case 0: sentencePtr = F("Hello"); break;
                case 1: sentencePtr = F("Merry Christmas"); break;
            }

        break;

        case 2:
            currentNbOfLines = 3;
            extraTextPaddingX = 69;

            switch(currentLineIndex)
            {
                case 0: extraTextPaddingX = -5; sentencePtr = F("Hello"); break;
                case 1: sentencePtr = F("There are two clues"); break;
                case 2: sentencePtr = F("hidden in the village"); break;
            }

        break;

        case 3:
            currentNbOfLines = 3;
            extraTextPaddingX = 60;
            switch(currentLineIndex)
            {
                case 0: sentencePtr = F("Mind those big balls"); break;
                case 1: sentencePtr = F("Grab the pepper spray"); break;
                case 2: sentencePtr = F("And jump the bastards"); break;
            }
            
            break;

        case 4:
            currentNbOfLines = 1;
            extraTextPaddingX = -6;
            switch(currentLineIndex)
            {
                case 0: sentencePtr = F("you again?"); break;
            }
            
            break;
        case 5:
            currentNbOfLines = 2;
            switch(currentLineIndex)
            {
                case 0: sentencePtr = F("I saw you"); break; 
                case 1: extraTextPaddingX = 68; sentencePtr = F("with those big balls"); break;
            }
    }

    if(currentLineIndex < currentNbOfLines) 
    {
        printSentence(sentencePtr);
    }
}
//======================================================================
void DialogManager::printSingleSentence(const __FlashStringHelper * stringSentence, int8 extraPadX, bool enableSound)
{
    printSingleSentence( reinterpret_cast<PGM_P>(stringSentence),  extraPadX, enableSound);
}

//======================================================================
void DialogManager::printSingleSentence(PGM_P stringSentence, int8 extraPadX, bool enableSound)
{
    extraTextPaddingX = extraPadX;
    currentTrigger = nullptr;
    printSentence(stringSentence);
    sound = enableSound;
}

//======================================================================
void DialogManager::printSentence(const __FlashStringHelper * sentenceString)
{
    printSentence( reinterpret_cast<PGM_P>(sentenceString) );
}

//======================================================================
void DialogManager::printSentence(PGM_P sentenceString)
{
    currentLetterPosition = 1;

    currentSentence = sentenceString;
    currentSentenceSize = getStringLength(sentenceString);
}

//======================================================================
void DialogManager::draw(Arduboy2 * arduboy)
{
    if(currentSentenceSize == 0) return;

    if(letterFrameCounter > DIALOG_LETTER_NBFRAMES)
    {
        if(currentLetterPosition >= currentSentenceSize)
        {
            if(letterFrameCounter > DIALOG_LETTER_NBFRAMES + DIALOG_SENTENCE_END_NBFRAMES)
            {
                currentSentenceSize = 0;
                currentLetterPosition = 0;
                if(currentTrigger != nullptr && currentLineIndex < currentNbOfLines)
                {
                    currentLineIndex++;
                    printNextLineForTrigger();
                }

                return;
            }
            else if(currentLineIndex >= currentNbOfLines - 1 && sound)
            {
                SoundManager::instance()->resumeMusic();
                sound = false;
            }
        }
        else
        {
            currentLetterPosition++;
            letterFrameCounter = 0;
            if(sound) SoundManager::instance()->playSound(LETTER);
        }
    }

    short startScreenX = DIALOG_DEFAULT_INIT_CURSOR_X;
    short screenY = DIALOG_DEFAULT_CURSOR_Y;
    if(currentTrigger)
    {
        startScreenX = currentTrigger->pos.x - Map::instance()->getScrollX();
        screenY =  currentTrigger->pos.y - Map::instance()->getScrollY() - TRIGGER_TEXT_PADDING_Y;
    }
    
    startScreenX -= extraTextPaddingX;

    short nbCharacterOnCurrentLine = 0;
    for(int8 i=0; i < currentLetterPosition; i++)
    {
        short cursorX = startScreenX + nbCharacterOnCurrentLine * DIALOG_CHAR_WIDTH;
        char currentChar = getChar(i);
        if(currentChar == '\n')
        {
            screenY += DIALOG_CHAR_HEIGHT;
            cursorX = startScreenX;
            nbCharacterOnCurrentLine = 0;
        }
        else
        {
            arduboy->setCursor(cursorX, screenY);
            arduboy->write(currentChar);
            nbCharacterOnCurrentLine++;
        }
    }

    letterFrameCounter++;
}

//======================================================================
char DialogManager::getChar(short i)
{
    return pgm_read_byte(currentSentence + i);
}