#pragma once

#include <stdint.h>
#include "Utils.h"


enum class ScoreType : uint8_t {
  Fifteen,
  Pair,
  RoyalPair,
  DoubleRoyalPair,
  RunOf5,
  RunOf4,
  RunOf3,
  OneForNob
};

enum class CribState : uint8_t {
  Empty,
  Hidden,
  Visible
};

enum class TurnUpState : uint8_t {
  Empty,
  Hidden,
  Visible
};

enum class Suit : uint8_t {
  Hearts,
  Diamonds,
  Spades,
  Clubs,
};

enum class WhichPlayer : uint8_t {
  Player1,
  Player2
};

enum class GameStateType : uint8_t {
	SplashScreen,
	TitleScreen,
  PlayGame,
  GameOver
};

enum class BubbleAlignment : uint8_t {
	Normal_Player,
	Normal_Computer,
	Score_Player,
	Score_Computer,
  None
};


enum class DealerFace : uint8_t {
	Normal,
	Happy,
	Sad,
};
