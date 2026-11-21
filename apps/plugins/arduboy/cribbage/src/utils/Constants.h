#pragma once

#define _DEBUG

#define _DEBUG_PRINT
#ifdef DEBUG_PRINT
  #define DEBUG_PRINT_CARDS
  #define DEBUG_PRINT_HANDS
  #define _DEBUG_PRINT_DISCARDS
  #define _DEBUG_PRINT_COMPUTER_DISCARDS
#endif

#define EEPROM_START                  100
#define EEPROM_START_C1               101
#define EEPROM_START_C2               102
#define EEPROM_PLAYER1                103
#define EEPROM_PLAYER2                113
#define EEPROM_PLAYER3                123
#define EEPROM_PLAYER4                133
#define EEPROM_HS_NAME_1              143
#define EEPROM_HS_NAME_2              153
#define EEPROM_HS_NAME_3              163
#define EEPROM_HS_SCORE_1             173
#define EEPROM_HS_SCORE_2             175
#define EEPROM_HS_SCORE_3             177
#define EEPROM_END                    179

namespace Constants {

  constexpr uint8_t Board_Tile_Width = 18;
  constexpr uint8_t Board_Start_Width = 18;
  constexpr uint8_t Board_Row_1_Y = 0;
  constexpr uint8_t Board_Row_2_Y = 18;
  constexpr uint8_t Board_Row_3_Y = 37;

  constexpr uint8_t NoCard = 255;
  constexpr uint8_t NumberOfCards = 52;

  constexpr uint8_t PlayerHandScores = 12;


}