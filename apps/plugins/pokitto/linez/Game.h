/*********************************************************************************************************************/
/*!
    @file     Game.h
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
#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED
// ------ Includes -----
#include "Board.h"
#include "Pieces.h"
#include "IO.h"
#include "Pokitto.h"

// ------ Defines -----

//#define WAIT_TIME 1000           // Number of milliseconds that the piece remains before going 1 block down */
const uint32_t waitTime = 1000;

// --------------------------------------------------------------------------------
//                                   Game
// --------------------------------------------------------------------------------

class Game
{
public:

	Game(Board &pBoard, int pScreenHeight);

	void DrawScene(int level, int nroflines, int score);
	void CreateNewPiece();
	void DetermineVerticalPosGhost();

	int mPosX, mPosY;               // Position of the piece that is falling down
	int mPosY_ghost;
	int mPiece, mRotation;          // Kind and rotation the piece that is falling down

private:

	int mScreenHeight;              // Screen height in pixels
	int mNextPosX, mNextPosY;       // Position of the next piece
	int mNextPiece, mNextRotation;  // Kind and rotation of the next piece
	int pos_raster_lines = 0;

	Board *mBoard;
	//Pieces *mPieces;

	int GetRand(int pA, int pB);
	void InitGame();
	void DrawPiece(int pX, int pY, int pPiece, int pRotation);
	void DrawPiece_Ghost(int pX, int pY, int pPiece, int pRotation);
	void DrawBoard();
	void DrawLevel(int level);
	void DrawNrofLines(int lines);
	void DrawScore(int score);
};



#endif // GAME_H_INCLUDED
