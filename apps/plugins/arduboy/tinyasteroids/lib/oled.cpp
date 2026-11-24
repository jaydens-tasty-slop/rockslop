#include <Arduino.h>
#include "oled.h"
#include <stdarg.h>
#include <Arduino.h>
#include <Arduboy2.h>
extern Arduboy2 arduboy;
#include "../res/font.h"

// SSD1306 OLED

//---------------------------------------------------------------------------
PROGMEM const u8 OledBootProgram[] = {
	0xD5,	// Set Display Clock Divisor 0xF0
	0xF0,
	0x8D,	// Charge Pump Setting 0x14
	0x14,
	0xA1,	// Set Segment Re-map (A0) | (b0001)
	0xC8,	// Set COM Output Scan Direction
	0x81,	// Set Contrast 0xCF
	0xCF,
	0xD9,	// Set Precharge 0xF1
	0xF1,
	0xAF,	// Display On
	0x20,	// Set display mode
	0x00,	// Horizontal addressing mode
};

//---------------------------------------------------------------------------
ST_OLED Oled;


//---------------------------------------------------------------------------
void OledInit(void)
{
	_Memset(&Oled, 0x00, sizeof(ST_OLED));
}
//---------------------------------------------------------------------------
void OledDisplay(void)
{
	memcpy(arduboy.getBuffer(), Oled.buf, sizeof(Oled.buf));
	arduboy.display();
}
//---------------------------------------------------------------------------
void OledDrawStr(u8 fx, u8 fy, const char* fmt, ...)
{
	char s[40];

	va_list ap;
	va_start(ap, fmt);
	_SprintfDo(s, fmt, ap);
	va_end(ap);


	u8 i;

	for(i=0; s[i] != '\0'; i++)
	{
		OledDrawChr(fx++, fy, s[i]);
	}
}
//---------------------------------------------------------------------------
void OledDrawChr(u8 fx, u8 fy, char chr)
{
	if(fx >= OLED_SCREEN_FONT_CX || fy >= OLED_SCREEN_FONT_CY)
	{
		return;
	}

	u8 x;

	for(x=0; x<OLED_FONT_SIZE; x++)
	{
		Oled.buf[(fx * OLED_FONT_CX + x) + (fy * OLED_SCREEN_CX)] = pgm_read_byte(font + (chr * OLED_FONT_SIZE) + x);
	}
}
//---------------------------------------------------------------------------
void OledDrawDot(u8 x, u8 y)
{
	if(x >= OLED_SCREEN_CX || y >= OLED_SCREEN_CY)
	{
		return;
	}

	Oled.buf[x + (y / 8) * OLED_SCREEN_CX] |= _BV(y % 8);
}
//---------------------------------------------------------------------------
void OledDrawCls(void)
{
//	u16 i;
//	for(i=0; i<sizeof(Oled.buf); i++) Oled.buf[i] = 0x00;

	memset(Oled.buf, 0, sizeof(Oled.buf));
}
//---------------------------------------------------------------------------
void OledDrawBmp(s8 sx, s8 sy, u8* p)
{
	u8* d  = (u8*)p;
	u8  cx = pgm_read_byte(d++);
	u8  cy = pgm_read_byte(d++);

	u8 chr, mask;
	u8 x, y, b;

	for(y=0; y<cy; y++)
	{
		if(sy + y < 0 || sy + y >= OLED_SCREEN_CY)
		{
			d += cx / 8;
			continue;
		}

		for(x=0; x<cx; x+=8)
		{
			chr  = pgm_read_byte(d++);
			mask = 0x80;

			for(b=0; b<8; b++)
			{
				if(sx + x + b >= 0 && sx + x + b < OLED_SCREEN_CX)
				{
					if(chr & mask)
					{
						Oled.buf[(sx + x + b) + ((sy + y) / 8) * OLED_SCREEN_CX] |= _BV((sy + y) & 0x7);
					}
				}

			 	mask >>=1;
			}
		}
	}
}
//---------------------------------------------------------------------------
void OledDrawPng8(s8 bx, s8 by, u8* p, u8 num)
{
	u8* d = (u8*)p + num * 8;
	u8  i;

	for(i=0; i<8; i++)
	{
		Oled.buf[(bx * 8 + by * OLED_SCREEN_CX) + i] |= pgm_read_byte(d + i);
	}
}
