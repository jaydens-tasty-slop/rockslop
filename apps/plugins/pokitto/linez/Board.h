/*********************************************************************************************************************/
/*!
    @file     Board.h
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
#ifndef BOARD_H_INCLUDED
#define BOARD_H_INCLUDED

// ------ Includes -----

#include "Pieces.h"
#include "IO.h"

// ------ Defines -----

const int boardLineWidth = 4;          // Width of each of the two lines that delimit the board
const int blockSize = 4;               // Width and Height of each block of a piece
const int boardPosition = 50;          // Center position of the board from the left of the screen
const int boardWidth = 10;              // Board width in blocks
const int boardHeight = 20;             // Board height in blocks
const int pieceBlocks = 5;              // Number of horizontal and vertical blocks of a matrix piece


// --------------------------------------------------------------------------------
//                                   Board
// --------------------------------------------------------------------------------

class Board
{
public:

	//Board(Pieces &pPieces, int pScreenHeight);
	Board(int pScreenHeight);

	int GetXPosInPixels(int pPos);
	int GetYPosInPixels(int pPos);
	color GetColor(int pX, int pY);
	bool IsFreeBlock(int pX, int pY);
	bool IsPossibleMovement(int pX, int pY, int pPiece, int pRotation);
	void StorePiece(int pX, int pY, int pPiece, int pRotation);
	int DeletePossibleLines();
	bool IsGameOver();

private:

	color mBoardcolor[boardWidth][boardHeight];
	//Pieces *mPieces;
	int mScreenHeight;

	void InitBoard();
	void DeleteLine(int pY);
};


#endif // BOARD_H_INCLUDED
