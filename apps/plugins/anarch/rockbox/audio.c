/* Original six samples and procedural soundtrack; no allocation/SD in IRQ.
 * CC0-1.0. The callback produces one short stereo block at the mixer rate. */
#include "port.h"
#if ANARCH_SOUND
#ifndef SFG_PROGRAM_MEMORY
#define SFG_PROGRAM_MEMORY static const
#define SFG_PROGRAM_MEMORY_U8(p) (*(p))
#endif
#include "../sounds.h"
static int16_t pcm[512*2];
static struct { unsigned pos; uint8_t sound, volume; } voices[2];
static unsigned phase;
static int sample;
static bool music_on;
static void more(const void **start, size_t *size)
{
    for(unsigned i=0;i<512;++i) {
        phase+=8000;
        if(phase>=HW_SAMPR_DEFAULT) {
            phase-=HW_SAMPR_DEFAULT;
            int mixed=0;
            if(music_on) {
                int music=SFG_getNextMusicSample();
                mixed=(music-SFG_musicTrackAverages[SFG_MusicState.track])*32;
            }
            for(unsigned v=0;v<2;++v)if(voices[v].pos<SFG_SFX_SAMPLE_COUNT) {
                mixed+=((int)SFG_GET_SFX_SAMPLE(voices[v].sound,voices[v].pos)-128)*
                    (1<<(voices[v].volume/37));
                voices[v].pos++;
            }
            sample=MAX(-32768,MIN(32767,mixed));
        }
        pcm[2*i]=pcm[2*i+1]=sample;
    }
    *start=pcm;*size=sizeof(pcm);
}
void anarch_audio_init(void)
{
    rb->audio_stop();
#if INPUT_SRC_CAPS != 0
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK,SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    phase=sample=0;music_on=false;
    rb->memset(&SFG_MusicState,0,sizeof(SFG_MusicState));
    voices[0].pos=voices[1].pos=SFG_SFX_SAMPLE_COUNT;
    rb->mixer_set_frequency(HW_SAMPR_DEFAULT);
    rb->pcmbuf_fade(false,true);
    static const struct mixer_play_cbs cbs={.get_more=more};
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK,&cbs,NULL,0);
}
void anarch_audio_close(void)
{
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
    rb->pcmbuf_fade(false,false);
    rb->mixer_set_frequency(HW_SAMPR_DEFAULT);
}
void anarch_audio_sound(uint8_t sound,uint8_t volume)
{
    if(sound>=6)return;
    rb->pcm_play_lock();
    unsigned v=voices[0].pos>=voices[1].pos ? 0 : 1;
    voices[v].sound=sound;voices[v].volume=volume;voices[v].pos=0;
    rb->pcm_play_unlock();
}
void anarch_audio_music(uint8_t command)
{
    rb->pcm_play_lock();
    if(command==2) {
        SFG_MusicState.track=(SFG_MusicState.track+1)%SFG_TRACK_COUNT;
        SFG_MusicState.t=SFG_MusicState.t2=SFG_MusicState.n11t=0;
    } else music_on=command==1;
    rb->pcm_play_unlock();
}
#else
void anarch_audio_init(void) {}
void anarch_audio_close(void) {}
void anarch_audio_sound(uint8_t sound,uint8_t volume) {(void)sound;(void)volume;}
void anarch_audio_music(uint8_t command) {(void)command;}
#endif
