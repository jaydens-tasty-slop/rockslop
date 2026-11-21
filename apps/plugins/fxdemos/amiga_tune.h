/* A short looping chiptune for the Amiga demo (the upstream demo is silent).
 * Original tune written for this port, released under CC0: eight bars in
 * A minor (Am F C G, twice) for a square lead, a quiet arpeggio and a
 * triangle bass, synthesized in integer maths on the mixer beep channel so
 * Rockbox's volume applies. The mixer callback runs in interrupt context:
 * keep this file free of floating point. Included once by amigademo.c. */
#ifndef AMIGA_TUNE_H
#define AMIGA_TUNE_H

#define TUNE_BPM 126
#define TUNE_BLOCK 256
#define TUNE_BARS 8
#define TUNE_LEAD_AMP 2200
#define TUNE_ARP_AMP 800
#define TUNE_BASS_AMP 2400

/* MIDI note numbers, 0 = rest; the lead has one entry per eighth note. */
static const uint8_t tune_lead[TUNE_BARS * 8] = {
  69, 0, 72, 0, 76, 0, 72, 69,   /* Am */
  69, 0, 72, 0, 77, 0, 76, 72,   /* F  */
  67, 0, 72, 0, 76, 0, 79, 76,   /* C  */
  74, 0, 71, 0, 67, 0, 71, 74,   /* G  */
  76, 0, 0, 72, 69, 0, 72, 76,   /* Am */
  77, 0, 76, 72, 69, 0, 72, 0,   /* F  */
  79, 0, 76, 0, 72, 0, 76, 79,   /* C  */
  74, 0, 71, 74, 79, 0, 0, 0,    /* G  */
};

static const uint8_t tune_root[4] = { 33, 29, 36, 31 };       /* A1 F1 C2 G1 */
static const uint8_t tune_chord[4][3] = {
  { 57, 60, 64 }, { 53, 57, 60 }, { 60, 64, 67 }, { 55, 59, 62 } };
static const uint8_t tune_arp_order[4] = { 0, 1, 2, 1 };

/* C4..B4 in 1/16 Hz */
static const uint16_t tune_hz16[12] = {
  4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 };

static int16_t tune_pcm[TUNE_BLOCK * 2];
static bool tune_open;
static uint32_t tune_rate, tune_step_len;
static volatile bool tune_hold;      /* silent while the demo is paused */
static int tune_pos, tune_count;     /* 16th note index, samples left in it */
static uint32_t tune_ph[3], tune_inc[3];
static int tune_env, tune_env_len;   /* lead decay over one eighth note */

static uint32_t tune_inc_for(int note)
{
  int rel = note - 60, oct = 0;

  while (rel < 0)
    rel += 12, oct--;
  while (rel >= 12)
    rel -= 12, oct++;
  uint64_t hz16 = tune_hz16[rel];
  hz16 = oct >= 0 ? hz16 << oct : hz16 >> -oct;
  return (uint32_t)((hz16 << 32) / ((uint64_t)tune_rate * 16));
}

static void tune_advance(void)
{
  int bar = tune_pos / 16, s = tune_pos % 16;
  int chord = bar % 4;      /* Am F C G, twice */
  int bassn;

  tune_inc[1] = tune_inc_for(tune_chord[chord][tune_arp_order[s % 4]] + 12);
  bassn = tune_root[chord];
  if (s == 4 || s == 12)
    bassn += 12;
  else if (s == 14)
    bassn += 7;
  tune_inc[2] = (s & 1) ? 0 : tune_inc_for(bassn + 12);
  if ((s & 1) == 0)
  {
    int n = tune_lead[bar * 8 + s / 2];
    tune_inc[0] = n ? tune_inc_for(n) : 0;
    tune_env = tune_env_len = tune_step_len * 2;
  }
  tune_count = tune_step_len;
  tune_pos = (tune_pos + 1) % (TUNE_BARS * 16);
}

static void tune_more(const void **start, size_t *size)
{
  for (int i = 0; i < TUNE_BLOCK; i++)
  {
    int out = 0;

    if (tune_hold)
    {
      tune_pcm[2 * i] = tune_pcm[2 * i + 1] = 0;
      continue;
    }
    if (tune_count <= 0)
      tune_advance();
    tune_count--;
    if (tune_env > 0)
      tune_env--;

    if (tune_inc[0])
    {
      tune_ph[0] += tune_inc[0];
      int a = TUNE_LEAD_AMP * tune_env / tune_env_len;
      out += (tune_ph[0] & 0x80000000u) ? a : -a;
    }
    if (tune_inc[1])
    {
      tune_ph[1] += tune_inc[1];
      out += (tune_ph[1] & 0xc0000000u) == 0xc0000000u ? TUNE_ARP_AMP : -TUNE_ARP_AMP;
    }
    if (tune_inc[2])
    {
      tune_ph[2] += tune_inc[2];
      int t = tune_ph[2] >> 16;          /* 0..65535 */
      t = t < 32768 ? t : 65535 - t;     /* triangle 0..32767 */
      out += (t - 16384) * TUNE_BASS_AMP / 16384;
    }
    tune_pcm[2 * i] = tune_pcm[2 * i + 1] = out;
  }
  *start = tune_pcm;
  *size = sizeof(tune_pcm);
}

/* Start (or continue) the tune, unless Rockbox is actually playing music
 * (a paused track still reports AUDIO_STATUS_PLAY). */
static void tune_start(void)
{
  static const struct mixer_play_cbs cbs = { .get_more = tune_more };

  int status = rb->audio_status();

  if (tune_open ||
      ((status & AUDIO_STATUS_PLAY) && !(status & AUDIO_STATUS_PAUSE)))
    return;
  tune_rate = rb->mixer_get_frequency();
  tune_step_len = tune_rate * 60 / (TUNE_BPM * 4);
  if (!tune_count)
  {
    rb->memset(tune_ph, 0, sizeof(tune_ph));
    rb->memset(tune_inc, 0, sizeof(tune_inc));
    tune_env = 0;
    tune_env_len = 1;
  }
  tune_hold = false;
  rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
  rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
  tune_open = true;
}

static void tune_stop(void)
{
  if (tune_open)
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
  tune_open = false;
}

#endif /* AMIGA_TUNE_H */
