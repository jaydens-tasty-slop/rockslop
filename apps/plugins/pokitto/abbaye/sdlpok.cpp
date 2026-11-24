#include <LibAudio>
#include "sdlpok.h"

//optimization by @FManga
void SDL_RenderCopyEx(rect *srcFull, rect *dstFull, uint8_t flip)
{
    rect srcClipped, dstClipped;
    srcClipped.x = srcFull->x;
    srcClipped.y = srcFull->y;
    srcClipped.w = srcFull->w;
    srcClipped.h = srcFull->h;
    dstClipped.x = dstFull->x;
    dstClipped.y = dstFull->y;

    if( dstFull->x < 0 )
    {
        if( !flip )
        {
            srcClipped.x -= dstFull->x;
            dstClipped.x = 0;
        }
        srcClipped.w += dstFull->x;
    }

    if( dstFull->y < 0 )
    {
        srcClipped.y -= dstFull->y;
        srcClipped.h += dstFull->y;
        dstClipped.y = 0;
    }

    if( dstClipped.x + srcClipped.w >= Pokitto::Display::width )
    {
        int16_t w = Pokitto::Display::width - dstClipped.x;
        if( w&1) w--;
        int16_t delta = srcClipped.w - w;
        if( flip )
        {
            srcClipped.x += delta;
            dstClipped.x -= delta+1;
            if( srcClipped.x&1 )
            {
                srcClipped.x++;
            }
        }
        srcClipped.w = w;
    }

    if( dstClipped.y + srcClipped.h >= Pokitto::Display::height )
    {
        srcClipped.h = Pokitto::Display::height - dstClipped.y;
    }

    if( srcClipped.x & 1 )
    {
        if( !flip )
        {
            srcClipped.x++;
            dstClipped.x++;
        }
        srcClipped.w--;
    }

    if( srcClipped.w <= 0 || srcClipped.h <= 0 )
        return;

    int pd1 = 0;
    int pd2 = 0;
    int pd = 0;
    int offsett = 0;
    int offsetty = 0;

    for (int y=0; y < srcClipped.h; y++)
    {
        offsetty = (((srcClipped.y + y) * 764)) + srcClipped.x;
        if( flip == 0 )
        {
            for (int x = 0; x < srcClipped.w-1; x+=2)
            {
                //calculate offset
                offsett = offsetty + x;
                pd = gfxtiles[2 + (offsett>>1)];

                if(pd!=0)
                {
                    pd1 = pd & (0x0F);
                    pd2 = pd >> 4;

                    Pokitto::Display::drawPixelRaw(dstClipped.x + x+1, dstClipped.y + y, pd1);
                    Pokitto::Display::drawPixelRaw(dstClipped.x + x, dstClipped.y + y, pd2);
                }
            }
        }
        else
        {
            for (int x = 0; x < srcClipped.w-1; x+=2)
            {
                //calculate offset
                offsett = offsetty + x;
                pd = gfxtiles[2 + (offsett>>1)];

                if(pd!=0)
                {
                    pd1 = pd & (0x0F);
                    pd2 = pd >> 4;

                    Pokitto::Display::drawPixelRaw(dstClipped.x + srcFull->w-x-1, dstClipped.y + y, pd1);
                    Pokitto::Display::drawPixelRaw(dstClipped.x + srcFull->w-x, dstClipped.y + y, pd2);
                }
            }
        }

    }

}

void SDL_RenderCopySolid(rect *srcFull, rect *dstFull)
{
    rect srcClipped, dstClipped;
    srcClipped.x = srcFull->x;
    srcClipped.y = srcFull->y;
    srcClipped.w = srcFull->w;
    srcClipped.h = srcFull->h;
    dstClipped.x = dstFull->x;
    dstClipped.y = dstFull->y;

    if( dstFull->x < 0 )
    {
	srcClipped.x -= dstFull->x;
	dstClipped.x = 0;
        srcClipped.w += dstFull->x;
    }

    if( dstFull->y < 0 )
    {
        srcClipped.y -= dstFull->y;
        srcClipped.h += dstFull->y;
        dstClipped.y = 0;
    }

    if( dstClipped.x + srcClipped.w >= Pokitto::Display::width )
    {
        int16_t w = Pokitto::Display::width - dstClipped.x;
        if( w&1) w--;
        int16_t delta = srcClipped.w - w;
        srcClipped.w = w;
    }

    if( dstClipped.y + srcClipped.h >= Pokitto::Display::height )
    {
        srcClipped.h = Pokitto::Display::height - dstClipped.y;
    }

    if( srcClipped.x & 1 )
    {
	srcClipped.x++;
	dstClipped.x++;
        srcClipped.w--;
    }

    if( srcClipped.w <= 0 || srcClipped.h <= 0 )
        return;

    int pd1 = 0;
    int pd2 = 0;
    const uint8_t *pd;
    uint8_t *out;
    int offsett = 0;
    int offsetty = 0;
    int dsty = dstClipped.y * (Pokitto::Display::width >> 1);
    offsetty = srcClipped.y * 764;

    for (int y=0; y < srcClipped.h; y++, offsetty += 764, dsty += Pokitto::Display::width >> 1 )
    {
        offsett = offsetty + srcClipped.x;
	pd = &gfxtiles[2 + (offsett>>1)];
	out = Pokitto::Display::screenbuffer + dsty + (dstClipped.x>>1);
	
        if( dstClipped.x & 1 )
        {
	    //calculate offset
            for (int x = 0; x < srcClipped.w-1; x+=2, out++, pd++)
            {
		pd2 = *pd >> 4;

		// Pokitto::Display::drawPixelRaw(dstClipped.x + x+1, dstClipped.y + y, pd1);
		out[1] = (out[1]&0x0F) | (*pd<<4);

		// Pokitto::Display::drawPixelRaw(dstClipped.x + x, dstClipped.y + y, pd2);
		out[0] = (out[0]&0xF0) | (pd2);
            }
        }
        else
        {
            for (int x = 0; x < srcClipped.w-1; x+=2 )
            {
		*out++ = *pd++;
            }
        }

    }

}

void SDL_RenderCopy(rect *src, rect *dst)
{
    SDL_RenderCopyEx(src, dst, 0);
}

/* Sound effects are raw files below the game's data dir (sfx/<n>.raw,
 * extracted by tools/abbaye_data.py) and music streams from music/. */
static const Audio::Clip sounds[] = {
    { "abbaye/sfx/0_shoot.raw" },
    { "abbaye/sfx/1_doorfx.raw" },
    { "abbaye/sfx/2_Item.raw" },
    { "abbaye/sfx/3_jump.raw" },
    { "abbaye/sfx/4_slash.raw" },
    { "abbaye/sfx/5_mechanismn.raw" },
    { "abbaye/sfx/6_thump.raw" },
    { "abbaye/sfx/7_thunder.raw" },
};

void Mix_PlaySound(uint8_t sound) {
    if( sound > 7 ) sound = 0;
    Audio::play<1>(sounds[sound]);
}

void Mix_HaltMusic() {
    Pokitto::Sound::pauseMusicStream();
}

const char *songs[] = {
    "0_PrayerofHopeN",
    "1_AreaIChurchN",
    "2_GameOverV2N",
    "3_HangmansTree",
    "4_AreaIICavesV2N",
    "5_EvilFightN",
    "6_AreaIIIHellN",
    "7_ManhuntwoodN",
    "8_MainTitleN",
    "9_ManhuntN"
};

void Mix_PlayMusic(uint8_t music) {
    char buf[40] = "abbaye/music/";
    strcat(buf, songs[music]);
    strcat(buf, ".raw");
    Pokitto::Sound::playMusicStream(buf, 0);
}

void Mix_PauseMusic() {
    Pokitto::Sound::pauseMusicStream();
}

void Mix_ResumeMusic() {
    Pokitto::Sound::playMusicStream();
}

void Pok_sleep(uint32_t ms)
{
 Pokitto::Core::wait(ms);
}
