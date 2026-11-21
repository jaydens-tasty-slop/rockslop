// ------ Includes -----
//#include "stdafx.h"
#include "IO.h"


uint8_t colorToIndex(color c)
{
  return static_cast<uint8_t>(c);
}

//static SDL_Surface *mScreen;										// Screen
/*static Uint32 mColors[COLOR_MAX] = { 0x000000ff,					// OLD Colors
0xff0000ff, 0x00ff00ff, 0x0000ffff,
0x00ffffff, 0xff00ffff, 0xffff00ff, 0xffa500ff, 0x800080ff,
0xffffffff,0x2f4f4f4f,0x4f4f4fff };
TTF_Font* font;
/*static Uint32 mColors[COLOR_MAX] = { 0x140c1cff,					// Colors
0xd04648ff, 0x6daa2cff, 0x0000ffff,
0x00ffffff, 0xff00ffff, 0xffff00ff, 0xffa500ff, 0x800080ff,
0xffffffff, 0x2f4f4f, 0x4f4f4f };*/
/*
======================================
Init
======================================
*/
/*IO::IO()
{
	InitGraph();
}


/*
======================================
Clear the screen to black
======================================
*/
/*void IO::ClearScreen()
{
	boxColor(mScreen, 0, 0, mScreen->w - 1, mScreen->h - 1, mColors[BLACK]);

}


/*
======================================
Draw a filled rectangle of a given color

Parameters:
>> pX1, pY1: 		Upper left corner of the rectangle
>> pX2, pY2: 		Lower right corner of the rectangle
>> pC				Rectangle color
======================================
*/
/*void IO::DrawRectangle(int pX1, int pY1, int pX2, int pY2, enum color pC)
{
	boxColor(mScreen, pX1, pY1, pX2, pY2 - 1, mColors[pC]);

}

/*
======================================
Draw a rectangle of a given color

Parameters:
>> pX1, pY1: 		Upper left corner of the rectangle
>> pX2, pY2: 		Lower right corner of the rectangle
>> pC				Rectangle color
======================================
*/
/*void IO::DrawRectangle2(int pX1, int pY1, int pX2, int pY2, enum color pC)
{
	rectangleColor(mScreen, pX1, pY1, pX2, pY2 - 1, mColors[pC]);
	//boxColor(mScreen, pX1, pY1, pX2, pY2 - 1, mColors[pC]);

}

/*
=====================================
Draws a textstring in a given color

Parameters
>> pX, pY:			Upper left positon of the text string on the screen
>> Text:			Text to be drawn
>> pC				Text color
*/
/*void IO::DrawString(int pX, int pY, int fontsize, const char *Text, enum color pC)
{
	//stringColor(mScreen, pX, pY, _strdup(Text), mColors[pC]);
	//TTF_Font* font = TTF_OpenFont("D:/Visual Studio Projects/Project1/Debug/Commodore Pixelized v1.2.ttf", 10);
	TTF_Font* font = TTF_OpenFont("Commodore Pixelized v1.2.ttf", fontsize);
	SDL_Color foregroundColor;
	//TTF_SetFontStyle(font, TTF_STYLE_ITALIC);
	foregroundColor.r = 150;
	foregroundColor.g = 150;
	foregroundColor.b = 150;
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, _strdup(Text), foregroundColor);
	SDL_Rect textLocation = { pX + (fontsize / 10), pY + (fontsize / 10), 0, 0 };
	SDL_BlitSurface(textSurface, NULL, mScreen, &textLocation);
	foregroundColor.r = mColors[pC] >> 24;
	foregroundColor.g = mColors[pC] >> 16;
	foregroundColor.b = mColors[pC] >> 8;
	textSurface = TTF_RenderText_Solid(font, _strdup(Text), foregroundColor);
	textLocation = { pX, pY, 0, 0 };
	SDL_BlitSurface(textSurface, NULL, mScreen, &textLocation);
	SDL_FreeSurface(textSurface);

	TTF_CloseFont(font);
}



/*
======================================
Return the screen height
======================================
*/
/*int IO::GetScreenHeight()
{
	return mScreen->h;
}


/*
======================================
Update screen
======================================
*/
/*void IO::UpdateScreen()
{
	SDL_Flip(mScreen);
}


/*
======================================
Keyboard Input
======================================
*/
/*int IO::Pollkey()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type) {
		case SDL_KEYDOWN:
			return event.key.keysym.sym;
		case SDL_QUIT:
			exit(3);
		}
	}
	return -1;
}

/*
======================================
Keyboard Input
======================================
*/
/*int IO::Getkey()
{
	SDL_Event event;
	while (true)
	{
		SDL_WaitEvent(&event);
		if (event.type == SDL_KEYDOWN)
			break;
		if (event.type == SDL_QUIT)
			exit(3);
	};
	return event.key.keysym.sym;
}

/*
======================================
Keyboard Input
======================================
*/
/*int IO::IsKeyDown(int pKey)
{
	Uint8* mKeytable;
	int mNumkeys;
	SDL_PumpEvents();
	mKeytable = SDL_GetKeyState(&mNumkeys);
	return mKeytable[pKey];
}

/*
======================================
SDL Graphical Initialization
======================================
*/
/*int IO::InitGraph()
{
	const SDL_VideoInfo *info;
	Uint8  video_bpp;
	Uint32 videoflags;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);


	// Alpha blending doesn't work well at 8-bit color
	info = SDL_GetVideoInfo();
	if (info->vfmt->BitsPerPixel > 8) {
		video_bpp = info->vfmt->BitsPerPixel;
	}
	else {
		video_bpp = 16;
	}
	//videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN;
	videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF;

	// Set 640x480 video mode
	if ((mScreen = SDL_SetVideoMode(640, 480, video_bpp, videoflags)) == NULL) {
		fprintf(stderr, "Couldn't set %ix%i video mode: %s\n", 640, 480, SDL_GetError());
		return 2;
	}
	TTF_Init();
	SDL_WM_SetCaption("Tetris", NULL);
	return 0;
}
*/
