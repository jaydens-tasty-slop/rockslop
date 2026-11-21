#include <stddef.h>
/*********************************************************************************************************************/
/*!
    @file     Pieces.cpp
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
#include "Pieces.h"
// Pieces definition
char mPieces[7 /*kind */][4 /* rotation */][5 /* horizontal blocks */][5 /* vertical blocks */] =
{
	// Square
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	},

	// I
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 2, 1, 1 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 1, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 1, 1, 2, 1, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	}
	,
	// L
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 1, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 1, 0, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 1, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	},
	// L mirrored
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 1, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 0, 0, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 0, 0, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	},
	// N
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 1, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 2, 0, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 1, 2, 0, 0 },
			{ 0, 1, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},



		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 1, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	},
	// N mirrored
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 0, 1, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 1, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 0, 0, 0 },
			{ 0, 1, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 1, 0 },
			{ 0, 1, 2, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	},
	// T
	{
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 2, 1, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 1, 2, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 1, 0, 0 },
			{ 0, 1, 2, 1, 0 },
			{ 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0 }
		}
	}
};

// Displacement of the piece to the position where it is first drawn in the board when it is created
int mPiecesInitialPosition[7 /*kind */][4 /* r2otation */][2 /* position */] =
{
	/* Square */
	{
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 }
	},
	/* I */
	{
		{ -2, -2 },
		{ -2, -3 },
		{ -2, -2 },
		{ -2, -3 }
	},
	/* L */
	{
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -2 }
	},
	/* L mirrored */
	{
		{ -2, -3 },
		{ -2, -2 },
		{ -2, -3 },
		{ -2, -3 }
	},
	/* N */
	{
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -2 }
	},
	/* N mirrored */
	{
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -2 }
	},
	/* T */
	{
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -3 },
		{ -2, -2 }
	},
};

color pieceColorLookup[] =
{
  color::Yellow,
  color::Cyan,
  color::Orange,
  color::Blue,
  color::Red,
  color::Green,
  color::Purple,
};
constexpr const size_t pieceColorLookupSize = sizeof(pieceColorLookup) / sizeof(color);


/*
======================================
Return the type of a block (0 = no-block, 1 = normal block, 2 = pivot block)

Parameters:

>> pPiece:        Piece to draw
>> pRotation: 1 of the 4 possible rotations
>> pX:            Horizontal position in blocks
>> pY:            Vertical position in blocks
======================================
*/
int Pieces::GetBlockType(int pPiece, int pRotation, int pX, int pY)
{
	return mPieces[pPiece][pRotation][pX][pY];
}


/*
======================================
Returns the horizontal displacement of the piece that has to be applied in order to create it in the
correct position.

Parameters:

>> pPiece:    Piece to draw
>> pRotation: 1 of the 4 possible rotations
======================================
*/
int Pieces::GetXInitialPosition(int pPiece, int pRotation)
{
	return mPiecesInitialPosition[pPiece][pRotation][0];
}


/*
======================================
Returns the vertical displacement of the piece that has to be applied in order to create it in the
correct position.

Parameters:

>> pPiece:    Piece to draw
>> pRotation: 1 of the 4 possible rotations
======================================
*/
int Pieces::GetYInitialPosition(int pPiece, int pRotation)
{
	return mPiecesInitialPosition[pPiece][pRotation][1];
}


/*
======================================
Returns the color of the piece.

Parameters:

>> pPiece:    Piece to draw
======================================
*/
color Pieces::GetPieceColor(int pieceKind)
{
  if(static_cast<size_t>(pieceKind) < pieceColorLookupSize)
    return pieceColorLookup[pieceKind];
  else
    return color::Blue;
}
