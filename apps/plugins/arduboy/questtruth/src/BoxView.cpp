/** 
 * This file is part of TheQuestForTruth project for the Arduboy platform by Guillaume Elias
 * You can reuse it in accordance with the GNU LESSER GENERAL PUBLIC LICENSE (see LICENSE) 
 **/

#include "BoxView.h"
#include "DialogManager.h"
#include "ItemsManager.h"
#include "drawable/Map.h"

namespace
{
    //CINEMATIC MODE
    static const short CINEMATIC_FRAME_MOVE = 5;

    static const short NB_INSTRUCTIONS_SEQ_1 = 40;
    static const short NB_INSTRUCTIONS_SEQ_2 = 40;
    static const short NB_INSTRUCTIONS_SEQ_3 = DIALOG_LETTER_NBFRAMES * 19 + DIALOG_SENTENCE_END_NBFRAMES;     //depends on number of letters in "THE QUEST FOR TRUTH"
    static const short NB_INSTRUCTIONS_SEQ_4 = 80;
    static const short END_SEQ_2 = NB_INSTRUCTIONS_SEQ_1 + NB_INSTRUCTIONS_SEQ_2;
    static const short END_SEQ_3 = NB_INSTRUCTIONS_SEQ_1 + NB_INSTRUCTIONS_SEQ_2 + NB_INSTRUCTIONS_SEQ_3;
    static const short END_SEQ_4 = NB_INSTRUCTIONS_SEQ_1 + NB_INSTRUCTIONS_SEQ_2 + NB_INSTRUCTIONS_SEQ_3 + NB_INSTRUCTIONS_SEQ_4;

    //HOUSE MODE
    static const short NB_INSTRUCTIONS_NOTHING = DIALOG_LETTER_NBFRAMES * 8 + DIALOG_SENTENCE_END_NBFRAMES;  //depends on number of letters in "Nothing."
    static const short NB_INSTRUCTIONS_CLUE_PRE = DIALOG_LETTER_NBFRAMES * 17 + DIALOG_SENTENCE_END_NBFRAMES;
    static const short NB_INSTRUCTIONS_CLUE_1 = DIALOG_LETTER_NBFRAMES * 7 + DIALOG_SENTENCE_END_NBFRAMES;
    static const short NB_INSTRUCTIONS_CLUE_2 = DIALOG_LETTER_NBFRAMES * 11 + DIALOG_SENTENCE_END_NBFRAMES;
    static const short NB_INSTRUCTIONS_CLUE_3 = DIALOG_LETTER_NBFRAMES * 10 + DIALOG_SENTENCE_END_NBFRAMES;
    static const short END_INSTRUCTIONS_CLUE_1 = NB_INSTRUCTIONS_CLUE_1 + NB_INSTRUCTIONS_CLUE_PRE;
    static const short END_INSTRUCTIONS_CLUE_2 = NB_INSTRUCTIONS_CLUE_2 + NB_INSTRUCTIONS_CLUE_PRE;
    static const short END_INSTRUCTIONS_CLUE_3 = NB_INSTRUCTIONS_CLUE_3 + NB_INSTRUCTIONS_CLUE_PRE;

}

//====================================================================
BoxView::BoxView()
    : frameCount(0)
    , inscructionNb(0)
{
    reset();
}

//====================================================================
void BoxView::reset()
{
    aX = 20;
    aY = 14;
  
    bX = 110;
    bY = 14;
  
    cX = 20;
    cY = 50;
  
    dX = 110;
    dY = 50;
  
    aaX = 0;
    aaY = 0;
  
    bbX = 128;
    bbY = 0;
  
    ccX = 0;
    ccY = 64;
  
    ddX = 128;
    ddY = 64;

    frameCount = 0;
    inscructionNb = 0;
}

//====================================================================
bool BoxView::update(Arduboy2 * arduboy)
{
    if(inscructionNb > 0)
    {
        if( inscructionNb == 1 && frameCount > NB_INSTRUCTIONS_NOTHING || 
            inscructionNb == 2 && frameCount > END_INSTRUCTIONS_CLUE_1 ||
            inscructionNb == 3 && frameCount > END_INSTRUCTIONS_CLUE_2 ||
            inscructionNb == 4 && frameCount > END_INSTRUCTIONS_CLUE_3 )
        {
            inscructionNb = 0;
            frameCount = 0;
            return false;
        }
        else if(frameCount == NB_INSTRUCTIONS_CLUE_PRE)
        {
            DialogManager::instance()->printSingleSentence((PGM_P)pgm_read_word(&ALL_CLUES[inscructionNb - 2]), 20);
        }

        DialogManager::instance()->draw( arduboy );
        frameCount++;
    }
    else
    {
        if((aY > 14 && bY > 14) || dY < bY || cY < aY)
        {
            return false;
        }

        bool checkClue = false;

        if( arduboy->justPressed( UP_BUTTON ) )
        {
            if( aY > 4 || bY > 4 ) moveUp();
            
            checkClue = true;
        }    
        else if( arduboy->justPressed( DOWN_BUTTON ) )
        {
            moveDown();
        }
        else if( arduboy->pressed( RIGHT_BUTTON ) )
        {
            if( dY - 20 > bY && ddY - 20 > bbY) moveRight();
            checkClue = true;
        }
        else if( arduboy->pressed( LEFT_BUTTON ) )
        {
            if( cY - 20 > aY && ccY - 20 > aaY) moveLeft();
            checkClue = true;
        }

        if(checkClue && 
        aY < 4 && bY < 4  && dX + 4 < SCREEN_WIDTH)
        {
            short clueId = Map::instance()->getCurrentDoorNumber();
            switch( clueId )
            {
                case 0:
                    DialogManager::instance()->printSingleSentence(F("Nothing."));
                    inscructionNb = 1;

                    break;

                default:
                    if(ItemsManager::instance()->hasClue(clueId) == false)
                    {
                        DialogManager::instance()->printSingleSentence(CLUE_PRE_TEXT, 24);
                        ItemsManager::instance()->foundClue(clueId);
                        inscructionNb = clueId + 1;
                    }
            }
        }
    }
    
    drawLines(arduboy);

    return true;
}

//====================================================================
bool BoxView::updateCinematic(Arduboy2 * arduboy)
{
    if(aX < 0)
    {
        return false;
    }

    //SEQUENCE 1
    if(inscructionNb < NB_INSTRUCTIONS_SEQ_1)
    {
        moveDown();
        inscructionNb++;
    }
    //SEQUENCE 3
    else if(inscructionNb >= END_SEQ_2 && inscructionNb < END_SEQ_3)
    {
        if(inscructionNb == END_SEQ_2)
        {
            DialogManager::instance()->printSingleSentence(F("THE QUEST\nFOR TRUTH"));
        }
        else if(inscructionNb == END_SEQ_3 - 1)
        {
            DialogManager::instance()->printSingleSentence(F(""));
        }
        DialogManager::instance()->draw( arduboy );
        inscructionNb++;
    }
    else if(frameCount > CINEMATIC_FRAME_MOVE)
    {
        //SEQUENCE 2
        if(inscructionNb >= NB_INSTRUCTIONS_SEQ_1 && inscructionNb < END_SEQ_2)
        {
            moveUp();
        }
        //SEQUENCE 4
        else if(inscructionNb >= END_SEQ_3 && inscructionNb < END_SEQ_4)
        {
            moveUp();
        }

        inscructionNb++;
        frameCount = 0;
    }
    
    frameCount++;
    drawLines(arduboy);

    return true;
}

//====================================================================
void BoxView::drawLines(Arduboy2 * arduboy)
{
    //inner lines
    arduboy->drawLine(aX, aY, bX, bY);
    arduboy->drawLine(cX, cY, dX, dY);
    arduboy->drawLine(aX, aY, cX, cY);
    arduboy->drawLine(bX, bY, dX, dY);

    //outer lines
    arduboy->drawLine(aaX, aaY, aX, aY);
    arduboy->drawLine(bbX, bbY, bX, bY);
    arduboy->drawLine(ccX, ccY, cX, cY);
    arduboy->drawLine(ddX, ddY, dX, dY);
}

//====================================================================
void BoxView::moveUp()
{
    aX -= 4;
    aY -= 4;
    bX += 4;
    bY -= 4;
    cX -= 4;
    cY += 4;
    dX += 4;
    dY += 4;
}

//====================================================================
void BoxView::moveDown()
{
    aX += 4;
    aY += 4;
    bX -= 4;
    bY += 4;
    cX += 4;
    cY -= 4;
    dX -= 4;
    dY -= 4;
}

//====================================================================
void BoxView::moveLeft()
{
    aX += 4;
    aY += 1;
    aaY += 1;
    cX += 4;
    cY -= 1;
    ccY -= 1;
    
    bX += 4;
    bY -= 1;
    bbY -= 1;
    dX += 4;
    dY += 1;
    ddY += 1;
}

//====================================================================
void BoxView::moveRight()
{
    aX -= 4;
    aY -= 1;
    aaY -= 1;
    cX -= 4;
    cY += 1;
    ccY += 1;
    
    bX -= 4;
    bY += 1;
    bbY += 1;
    dX -= 4;
    dY -= 1;
    ddY -= 1;
}