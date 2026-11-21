/*********************************************************************************************************************/
/*!
    @file     Board.cpp
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
#include "Board.h"


/*
==================
Init
==================
*/
Board::Board(int pScreenHeight)
{
	// Get the screen height
	mScreenHeight = pScreenHeight;

	// Get the pointer to the pieces class
	//mPieces = &pPieces;

	//Init the board blocks with free positions
	InitBoard();
}


/*
======================================
Init the board blocks with free positions
======================================
*/
void Board::InitBoard()
{
	for (int i = 0; i < boardWidth; i++)
		for (int j = 0; j < boardHeight; j++)
		{
			//mBoard[i][j] = POS_FREE;
			mBoardcolor[i][j] = color::Black;
		}
}

/*
======================================
Store a piece in the board by filling the blocks

Parameters:

>> pX:		Horizontal position in blocks
>> pY:		Vertical position in blocks
>> pPiece:	Piece to draw
>> pRotation:	1 of the 4 possible rotations
======================================
*/
void Board::StorePiece(int pX, int pY, int pPiece, int pRotation)
{
	// Store each block of the piece into the board
	//const color mColor = mPieces->GetPieceColor(pPiece);
	const color mColor = Pieces::GetPieceColor(pPiece);
	for (int i1 = pX, i2 = 0; i1 < pX + pieceBlocks; i1++, i2++)
	{
		for (int j1 = pY, j2 = 0; j1 < pY + pieceBlocks; j1++, j2++)
		{
			// Store only the blocks of the piece that are not holes
			//if (mPieces->GetBlockType(pPiece, pRotation, j2, i2) != 0)
            if (Pieces::GetBlockType(pPiece, pRotation, j2, i2) != 0)
			{
				//mBoard[i1][j1] = POS_FILLED;
				mBoardcolor[i1][j1] = mColor;
			}
		}
	}
}


/*
======================================
Check if the game is over becase a piece have achived the upper position

Returns true or false
======================================
*/
bool Board::IsGameOver()
{
	//If the first line has blocks, then, game over
	for (int i = 0; i < boardWidth; i++)
	{
	//	if (mBoard[i][0] == POS_FILLED) return true;
		if (mBoardcolor[i][0] != color::Black) return true;
	}

	return false;
}


/*
======================================
Delete a line of the board by moving all above lines down

Parameters:

>> pY:		Vertical position in blocks of the line to delete
======================================
*/
void Board::DeleteLine(int pY)
{
	// Moves all the upper lines one row down
	for (int j = pY; j > 0; j--)
	{
		for (int i = 0; i < boardWidth; i++)
		{
			//mBoard[i][j] = mBoard[i][j - 1];
			mBoardcolor[i][j] = mBoardcolor[i][j - 1];
		}
	}
}


/*
======================================
Delete all the lines that should be removed
======================================
*/
int Board::DeletePossibleLines()
{
	int nr_of_lines = 0;
	for (int j = 0; j < boardHeight; j++)
	{
		int i = 0;
		while (i < boardWidth)
		{
			//if (mBoard[i][j] != POS_FILLED) break;
			if (mBoardcolor[i][j] == color::Black) break;
			i++;
		}

		if (i == boardWidth) {
//			for (int k = 0; k < BOARD_WIDTH; k++){
//				mBoardcolor[k][j] = BLUE;
//			}
			DeleteLine(j);
			nr_of_lines++;
		}
	}
	return nr_of_lines;
}


/*
======================================
Returns 1 (true) if the this block of the board is empty, 0 if it is filled

Parameters:

>> pX:		Horizontal position in blocks
>> pY:		Vertical position in blocks
======================================
*/
bool Board::IsFreeBlock(int pX, int pY)
{
	//if (mBoard[pX][pY] == POS_FREE) return true; else return false;
	if (mBoardcolor[pX][pY] == color::Black) return true; else return false;
}


color Board::GetColor(int pX, int pY)
{
	return mBoardcolor[pX][pY];
}
/*
======================================
Returns the horizontal position (isn pixels) of the block given like parameter

Parameters:

>> pPos:	Horizontal position of the block in the board
======================================
*/
int Board::GetXPosInPixels(int pPos)
{
	return  ((boardPosition - (blockSize * (boardWidth / 2))) + (pPos * blockSize));
}


/*
======================================
Returns the vertical position (in pixels) of the block given like parameter

Parameters:

>> pPos:	Horizontal position of the block in the board
======================================
*/
int Board::GetYPosInPixels(int pPos)
{
	return ((mScreenHeight - (blockSize * boardHeight)) + (pPos * blockSize));
}


/*
======================================
Check if the piece can be stored at this position without any collision
Returns true if the movement is  possible, false if it not possible

Parameters:

>> pX:		Horizontal position in blocks
>> pY:		Vertical position in blocks
>> pPiece:	Piece to draw
>> pRotation:	1 of the 4 possible rotations
======================================
*/
bool Board::IsPossibleMovement(int pX, int pY, int pPiece, int pRotation)
{
	// Checks collision with pieces already stored in the board or the board limits
	// This is just to check the 5x5 blocks of a piece with the appropiate area in the board
	for (int i1 = pX, i2 = 0; i1 < pX + pieceBlocks; i1++, i2++)
	{
		for (int j1 = pY, j2 = 0; j1 < pY + pieceBlocks; j1++, j2++)
		{
			// Check if the piece is outside the limits of the board
			if (i1 < 0 ||
				i1 > boardWidth - 1 ||
				j1 > boardHeight - 1)
			{
				//if (mPieces->GetBlockType(pPiece, pRotation, j2, i2) != 0)
                if (Pieces::GetBlockType(pPiece, pRotation, j2, i2) != 0)
					return 0;
			}

			// Check if the piece have collisioned with a block already stored in the map
			if (j1 >= 0)
			{
				//if ((mPieces->GetBlockType(pPiece, pRotation, j2, i2) != 0) && (!IsFreeBlock(i1, j1)))
                if ((Pieces::GetBlockType(pPiece, pRotation, j2, i2) != 0) && (!IsFreeBlock(i1, j1)))
					return false;
			}
		}
	}

	// No collision
	return true;
}
