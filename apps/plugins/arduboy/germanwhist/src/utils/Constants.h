#pragma once

#define _DEBUG

#define DEBUG_PRINT
#ifdef DEBUG_PRINT
  #define DEBUG_PRINT_CARDS
  #define DEBUG_PRINT_HANDS
  #define _DEBUG_PRINT_DISCARDS
  #define _DEBUG_PRINT_COMPUTER_DISCARDS
#endif

namespace Constants {

  constexpr uint8_t Board_Tile_Width = 18;
  constexpr uint8_t Board_Start_Width = 18;
  constexpr uint8_t Board_Row_1_Y = 0;
  constexpr uint8_t Board_Row_2_Y = 18;
  constexpr uint8_t Board_Row_3_Y = 37;

  constexpr uint8_t NoCard = 255;
  constexpr uint8_t NumberOfCards = 52;

  constexpr uint8_t PlayerHandScores = 12;
  constexpr uint8_t GameOverScore = 50;
  constexpr uint8_t OverallScore_X = 10;
  constexpr uint8_t OverallScore_Y = 23;

}