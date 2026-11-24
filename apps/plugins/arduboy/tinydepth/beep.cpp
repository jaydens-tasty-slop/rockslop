#include <Arduino.h>
#include "beep.h"
#include "lib/snd.h"


//---------------------------------------------------------------------------
ST_BEEP Beep;

//---------------------------------------------------------------------------
void BeepInit(void)
{
	_Memset(&Beep, 0x00, sizeof(ST_BEEP));

	Beep.isSe = TRUE;
}
//---------------------------------------------------------------------------
void BeepBgmPlay(void)
{
	if(Beep.isBgm == FALSE)
	{
		return;
	}

	// The upstream background tune is MIDI data borrowed from a web page
	// without a licence, so it is not included here: effects only.
}
//---------------------------------------------------------------------------
void BeepBgmStop(void)
{
	SndStopScore();
}
//---------------------------------------------------------------------------
void BeepSePlay(u16 freq, u32 duration)
{
	if(Beep.isSe == FALSE)
	{
		return;
	}

	SndPlayTone(freq, duration);
}
//---------------------------------------------------------------------------
void BeepSeShipFire(void)
{
	BeepSePlay(330, 100);
}
//---------------------------------------------------------------------------
void BeepSeEnemyDestory(void)
{
	BeepSePlay(175, 100);
}
//---------------------------------------------------------------------------
void BeepSetBgm(bool is)
{
	Beep.isBgm = is;
}
//---------------------------------------------------------------------------
void BeepSetSe(bool is)
{
	Beep.isSe = is;
}
//---------------------------------------------------------------------------
bool BeepIsBgm(void)
{
	return Beep.isBgm;
}
//---------------------------------------------------------------------------
bool BeepIsSe(void)
{
	return Beep.isSe;
}
