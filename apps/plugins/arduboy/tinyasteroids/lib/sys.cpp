#include <Arduino.h>
#include "sys.h"
#include <Arduboy2.h>
Arduboy2 arduboy;
#include "frame.h"
#include "snd.h"
#include "key.h"
#include "oled.h"
#include "eep.h"
#include "math.h"
#include "rnd.h"

//---------------------------------------------------------------------------
void SysInit(void)
{

	arduboy.begin();
	OledInit();
	FrameInit();
	SndInit();
	EepInit();
	KeyInit();
	MathInit();
	RndInit();
}
//---------------------------------------------------------------------------
void SysIdle(void)
{
	delay(1);
}
//---------------------------------------------------------------------------
bool SysLoop(void)
{
	if(FrameLoop() == FALSE)
	{
		return FALSE;
	}

	KeyLoop();
	return TRUE;
}
//---------------------------------------------------------------------------
void SysLoopEnd(void)
{
	OledDisplay();
	OledDrawCls();
}
