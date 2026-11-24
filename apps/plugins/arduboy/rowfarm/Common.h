#ifndef COMMON_H
#define COMMON_H

// System global variables
Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
// BeepPin1 is silent on Rockbox: same interface, played through ArduboyTones.
struct {
  void begin() {}
  void timer() {}
  uint16_t freq(float hz) { return (uint16_t)hz; }
  // ArduboyTones treats anything under 250 Hz as silence; keep low beeps audible
  void tone(uint16_t hz, uint8_t frames) { sound.tone(hz < 250 ? 250 : hz, frames * 1000 / frameRate); }
} beep;
SpritesB sprites;

enum class GameState { Menu,
                       InGame,
                       InGamePause,
                       GameOver,
                       ScoringTable };

// Globals variables used everywhere
extern GameState currentGameState = GameState::Menu;
extern bool soundEnabled = true;
extern bool firstGameStateDraw = true;
extern uint16_t highScores[highScoreSize] = {0};

#endif