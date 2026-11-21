/*********************************************************************************************************************/
/*!
    @file     Main.cpp
    @author   sbmrgd

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2018, sbmrgd
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    This entire game is based on a tutorial by Javier Lopez Lopez
    Link: http://javilop.com/gamedev/tetris-tutorial-in-c-platform-independent-focused-in-game-logic-for-beginners/
*/
/*********************************************************************************************************************/
#define DISABLEAVRMIN

#include "Pokitto.h"


#include "Game.h"
Pokitto::Core mygame;

enum class GameState { TitleScreen,   GamePlay,   GameOver };
uint32_t mTime1 = 0;   /* upstream reads the clock here, before the plugin runs */
uint32_t mTime2 = 0;
double speed = 1;
int nr_lines = 0;
int nr_lines2 = 0;
int total_nr_lines = 0;
int level = 0;
int score = 0;
int mScreenHeight = mygame.display.getHeight();
int offset = 0;
int direction = 1;
color c_color = color::White;
GameState gameState = GameState::TitleScreen;
// Pieces
//Pieces mPieces2;

// Board
//Board mBoard(mPieces2, mScreenHeight);
Board mBoard(mScreenHeight);

// Game
//Game mGame(mBoard, mPieces2, mScreenHeight);
Game mGame(mBoard, mScreenHeight);

void showTitleScreen() {
    mygame.display.setColor(colorToIndex(color::Orange));
    mygame.display.setFont(fontKoubit);
    mTime2 = mygame.getTime();
    if ((mTime2 - mTime1)>waitTime/8){
        offset+=direction;
        if (offset==0) direction = 1;
        if (offset==80) direction = -1;
        if (c_color==color::White) c_color = color::Black; else c_color = color::White;
        mTime1 = mygame.getTime();
    }
    for(int i=10;i<mygame.display.getHeight()/2-mygame.display.fontHeight/2;i+=10)
        mygame.display.drawLine(0,i+offset,mygame.display.getWidth(),i+offset);

    mygame.display.setCursor(mygame.display.getWidth()/2+mygame.display.fontWidth*3+1-offset,mygame.display.getHeight()/2-mygame.display.fontHeight/2);
    mygame.display.println("Linez");

    for(int i=mygame.display.getHeight()-10;i>mygame.display.getHeight()/2+mygame.display.fontHeight/2;i-=10)
        mygame.display.drawLine(0,i-offset,mygame.display.getWidth(),i-offset);

    mygame.display.setColor(colorToIndex(c_color));
    mygame.display.setCursor(mygame.display.getWidth()/2-mygame.display.fontWidth*3+5,mygame.display.getHeight()-mygame.display.fontHeight-3);
    mygame.display.println("Press C");

    if (mygame.buttons.pressed(BTN_C)) {
        gameState = GameState::GamePlay;
        mygame.display.setFont(font3x5);
//        mBoard = Board(mPieces2, mScreenHeight);
        mBoard = Board(mScreenHeight);
        speed = 1;
        level = 0;
        score = 0;
        nr_lines = 0;
        total_nr_lines = 0;
        mGame.CreateNewPiece();
        mGame.DetermineVerticalPosGhost();
    }
}

void playGame() {
    if(!(mBoard.IsGameOver())){
        mGame.DrawScene(level,total_nr_lines,score);

        if(mygame.buttons.pressed(BTN_LEFT)) {
            if (mBoard.IsPossibleMovement(mGame.mPosX - 1, mGame.mPosY, mGame.mPiece, mGame.mRotation)){
                mGame.mPosX--;
                mGame.DetermineVerticalPosGhost();
            }
        }
        if(mygame.buttons.pressed(BTN_RIGHT)) {
            if (mBoard.IsPossibleMovement(mGame.mPosX + 1, mGame.mPosY, mGame.mPiece, mGame.mRotation)){
                mGame.mPosX++;
                mGame.DetermineVerticalPosGhost();
            }
        }

        if(mygame.buttons.downBtn()) {
            if (mBoard.IsPossibleMovement(mGame.mPosX, mGame.mPosY + 1, mGame.mPiece, mGame.mRotation))
				mGame.mPosY++;
        }

        if (mygame.buttons.pressed(BTN_B)) {
            while (mBoard.IsPossibleMovement(mGame.mPosX, mGame.mPosY, mGame.mPiece, mGame.mRotation)) { mGame.mPosY++; }
            mBoard.StorePiece(mGame.mPosX, mGame.mPosY - 1, mGame.mPiece, mGame.mRotation);
			score++;
			nr_lines2 = mBoard.DeletePossibleLines();
			nr_lines += nr_lines2;
			total_nr_lines += nr_lines2;
            score += nr_lines2*(1+level)*10;
			mGame.CreateNewPiece();
            mGame.DetermineVerticalPosGhost();
        }

        if(mygame.buttons.pressed(BTN_A)) {
            if (mBoard.IsPossibleMovement(mGame.mPosX, mGame.mPosY, mGame.mPiece, (mGame.mRotation + 1) % 4)){
                mGame.mRotation = (mGame.mRotation + 1) % 4;
                mGame.DetermineVerticalPosGhost();
            }
        }

        mTime2 = mygame.getTime();

		if ((mTime2 - mTime1) > waitTime / speed)
		{
			if (mBoard.IsPossibleMovement(mGame.mPosX, mGame.mPosY + 1, mGame.mPiece, mGame.mRotation))
			{
				mGame.mPosY++;
			}
			else
			{
				mBoard.StorePiece(mGame.mPosX, mGame.mPosY, mGame.mPiece, mGame.mRotation);
				score++;
				nr_lines2 = mBoard.DeletePossibleLines();
				nr_lines += nr_lines2;
				total_nr_lines += nr_lines2;
                score += nr_lines2*(1+level)*10;
                mGame.CreateNewPiece();
                mGame.DetermineVerticalPosGhost();
			}

			mTime1 = mygame.getTime();
		}

		//Every 10 lines the level and the speed are increased
		if (nr_lines >= 10) {
			nr_lines = nr_lines - 10;
			speed += 0.5;
			level++;

		}
    }

    else{
        gameState = GameState::GameOver;

    }
}

void showGameOver(){
    mygame.display.setFont(fontKoubit);
    mygame.display.setColor(colorToIndex(color::Red));
    mygame.display.setCursor(mygame.display.getWidth()/2-mygame.display.fontWidth*3+2,10);
    mygame.display.println("Game Over");
    mygame.display.println();
    mygame.display.setColor(colorToIndex(color::Orange));
    mygame.display.print(" Level: ");
    mygame.display.println(level);
    mygame.display.print(" Lines: ");
    mygame.display.println(total_nr_lines);
    mygame.display.print(" Score: ");
    mygame.display.println(score);
    mygame.display.println();

    mTime2 = mygame.getTime();
    if ((mTime2 - mTime1)>waitTime/8)
        {
            if (c_color==color::White) c_color= color::Black; else c_color = color::White;
            mTime1 = mygame.getTime();
        }

    mygame.display.setCursor(mygame.display.getWidth()/2-mygame.display.fontWidth*3+5,mygame.display.getHeight()-mygame.display.fontHeight-3);
    mygame.display.setColor(colorToIndex(c_color));
    mygame.display.println("Press C");

    if (mygame.buttons.pressed(BTN_C)) {
                    gameState = GameState::TitleScreen;
    }
}

int main () {
    mygame.begin();
    mygame.display.setFont(font3x5);
    mygame.display.loadRGBPalette(paletteLinez);
    //gameState = GameState::TitleScreen;
    while (mygame.isRunning()) {
        if (mygame.update()) {
                switch(gameState){
                    case GameState::TitleScreen: showTitleScreen(); break;
                    case GameState::GamePlay: playGame(); break;
                    case GameState::GameOver: showGameOver(); break;
                }
        }
    }
    return 0;
}

/* Rockbox port: lets the key mapping tell play from the menus. */
bool pk_linez_playing(void) { return gameState == GameState::GamePlay; }
