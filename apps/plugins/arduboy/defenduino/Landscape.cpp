#include "Landscape.h"
#include "Globals.h"

// Each byte represents four pixels on the landscape.  The first 5 bits == the height (minus 35) of the first pixel.  The next three bits
// represents the following three pixels and are either 1 if its value should be 1 higher than the preceding pixel, or 0 if it should
// be one lower.
const uint8_t landscapeData[256] PROGMEM = {
	185,185,185,185,153,247,155,185,155,185,185,153,55,21,17,13,235,239,243,247,185,57,23,147,243,247,185,185,185,25,247,185,185,185,57,
	23,147,243,247,185,185,185,185,25,21,17,13,201,235,239,243,247,185,185,185,57,23,245,185,185,25,247,185,185,185,185,59,25,21,241,245,
	185,185,185,185,25,247,155,25,21,145,241,53,243,247,185,185,185,121,91,91,91,91,91,155,57,89,153,183,183,183,55,21,243,23,117,149,85,
	21,17,141,237,241,245,121,153,151,149,149,117,183,217,153,57,87,53,179,213,215,153,23,21,51,241,245,55,245,121,57,23,51,147,19,207,15,
	203,13,169,139,43,9,7,5,65,225,197,167,233,205,239,179,245,151,119,249,27,247,185,155,185,121,91,91,91,91,91,91,91,91,185,185,185,57,
	89,90,90,90,90,90,90,90,90,184,184,184,184,24,116,150,118,182,120,90,154,26,54,150,20,48,78,110,240,148,116,84,244,120,26,150,148,50,
	16,110,208,80,238,242,212,246,90,90,90,90,90,26,88,88,182,120,154,186,122,90,90,90,90,90,26,22,244,248,90,90,90,90,90
};


void Landscape::render(int cameraXPos)
{
	int xPos = cameraXPos - 64;
	if (xPos < 0)
	{
		xPos += 1024;
	}

	uint8_t remainder = xPos % 4;
	// Got to start at the beginning of the first 4-point chunk
	if (remainder != 0)
	{
		xPos -= remainder;
	}

	uint8_t chunkIdx = xPos / 4;

	for (int i = 0 - remainder; i < 128; i += 4)
	{
		uint8_t chunk = pgm_read_byte(landscapeData + chunkIdx);

		// First position is first five bits
		uint8_t height = chunk & 0x1F;
		arduboy.drawPixel(i, height + 35);

		// Then next three bits specify if the next three are above or below
		for (uint8_t sub = 1; sub < 4; sub++)
		{
			bool up = chunk & (1 << (4 + sub));
			if (up)
			{
				height++;
			}
			else
			{
				height--;
			}
			arduboy.drawPixel(i + sub, height + 35);
		}
		chunkIdx++;
		if (chunkIdx == 256)
		{
			chunkIdx = 0;
		}
	}
}

byte Landscape::getHeight(int xPos)
{
	uint8_t remainder = xPos % 4;
	// Got to start at the beginning of the first 4-point chunk
	if (remainder != 0)
	{
		xPos -= remainder;
	}

	uint8_t chunk = pgm_read_byte(landscapeData + (xPos / 4));

	uint8_t height = chunk & 0x1F;

	for (int i = 0; i < remainder; i++)
	{
		bool up = chunk & (1 << (5 + i));
		if (up)
		{
			height++;
		}
		else
		{
			height--;
		}
	}

	return height + 35;
}