/* Audio half of OBONO's MyArduboy2 (originally MyArduboyPlaytune.cpp: AVR
 * timers and an ISR) on top of ArduboyPlaytune (score voices + one tone
 * voice). A game's MyArduboyPlaytune.cpp is replaced by:
 *     #include "MyArduboy2.h"
 *     #include "../lib/ArduboyPlaytune.h"
 *     #include "../lib/MyArduboy2Audio.inc"
 * Lower priority numbers win, as upstream. The duty cycle is ignored. A game
 * that has playWave()/stopTone() in MyArduboy2.h defines MYAB2_PLAYWAVE /
 * MYAB2_STOPTONE before the include. */

static ArduboyPlaytune ab_pt(NULL);
static uint8_t ab_chans;
static uint8_t ab_prio;
static bool ab_score_on;
static bool ab_tone_prio_valid;
static uint8_t ab_tone_prio;

void MyArduboy2::initAudio(uint8_t chans)
{
    ab_chans = chans;
    ab_score_on = false;
}

void MyArduboy2::closeAudio(void)
{
    ab_pt.stopScore();
    ab_pt.stopTone();
    ab_chans = 0;
    ab_score_on = false;
}

bool MyArduboy2::isAudioEnabled(void) { return audio.enabled(); }
void MyArduboy2::setAudioEnabled(bool on) { (on) ? audio.on() : audio.off(); }
void MyArduboy2::toggleAudioEnabled(void) { audio.toggle(); }
void MyArduboy2::saveAudioOnOff(void) { audio.saveOnOff(); }

void MyArduboy2::playTone(uint16_t frequency, uint16_t duration,
        uint8_t priority, uint8_t dutyCycle)
{
    (void)dutyCycle;
    if (ab_chans == 0) return;
    ab_score_on = ab_score_on && ab_pt.playing();
    if (ab_chans == 1 && ab_score_on) {
        if (priority > ab_prio) return;
        stopScore();
    }
    if (ab_score_on)
        ab_pt.muteScore(priority <= ab_prio);
    ab_tone_prio = priority;
    ab_tone_prio_valid = true;
    ab_pt.playTone(frequency, duration);
}

void MyArduboy2::playScore(const byte *score, uint8_t priority, int8_t pitch)
{
    ab_score_on = ab_score_on && ab_pt.playing();
    if (ab_score_on) {
        if (priority > ab_prio) return;
        stopScore();
    } else if (ab_pt.tonePlaying() && ab_tone_prio_valid) {
        ab_pt.muteScore(priority > ab_tone_prio);
    }
    ab_prio = priority;
    ab_score_on = true;
    ab_pt.playScore(score, pitch);
}

void MyArduboy2::stopScore(void)
{
    ab_pt.stopScore();
    ab_pt.stopWave();
    ab_score_on = false;
}

#ifdef MYAB2_STOPTONE
void MyArduboy2::stopTone(void)
{
    ab_pt.stopTone();
}
#endif

#ifdef MYAB2_PLAYWAVE
void MyArduboy2::playWave(uint16_t frequency, const byte *wave, uint16_t samples,
        uint8_t priority)
{
    ab_score_on = ab_score_on && ab_pt.playing();
    if (ab_score_on || ab_pt.wavePlaying()) {
        if (priority > ab_prio) return;
        stopScore();
    }
    ab_prio = priority;
    ab_pt.playWave(frequency, wave, samples);
}
#endif
