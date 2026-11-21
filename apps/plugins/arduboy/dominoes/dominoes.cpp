#include <Arduino.h>
#include <Arduboy2.h>

#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Enums.h"
#include "src/utils/Name.h"
#include "src/images/Images.h"
#include "src/fonts/Font4x6.h"
#include "src/utils/EEPROM_Utils.h"

#ifndef DEBUG
ARDUBOY_NO_USB
#endif

#ifdef SOUNDS
#include <ArduboyTones.h>
#endif

/* Arduino IDE auto-prototypes */
void playGame_Loop();
void swapPlayers(bool startOfGame);
void getNames_Loop();
void renderSideBones();
void drawHighScore_Loop(Name *name);
void renderScore(int16_t score, uint8_t x, uint8_t y);
void drawTalkBubble(uint8_t lines);
void drawMessageBox_WithHourglass(uint8_t boneIdx);
void drawMessageBox(String message, uint8_t lines, bool waitForButtonPress);
void drawMessageBox(String message, uint8_t lines, bool waitForButtonPress, Player renderPlayerScore);
void drawMessageBox(String message, uint8_t lines);
boolean exitGame();
void computersHand_Loop();
void playersHand_Loop(Player player, Player secondPlayer);
void highestPossibleScore(Player player, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_North(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_South(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_Center(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_CenterOnly(uint8_t boneIdx, uint8_t leftPips, uint8_t rightPips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_West(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_West_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_East(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void highestPossibleScore_East_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore(Player player, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_North(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_South(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_Center(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_CenterOnly(uint8_t boneIdx, uint8_t leftPips, uint8_t rightPips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_West(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_West_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void lowestPossibleScore_East(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection);
void playBone(GameMode gameMode, Player player, uint8_t index, Position position);
boolean canPlayBoard(uint8_t boneIdx, uint8_t position);
void renderPlayersScore(Player player, HandPosition handPosition);
void renderPlayersScore(GameMode gameMode, Player player);
void renderPlayerScore(HandPosition position, Player player, bool highlight);
void renderPlayersHand(Player player, uint8_t highlight, bool hideBones);
void renderGraveyard(Player currentPlayer, uint8_t highlight, uint8_t playersHandHighlight, bool hideBones);
void renderBoard(GameMode gameMode, Player player, Position highlight, bool hideBones);
void drawBone_Small(uint8_t boneValue, uint8_t x, uint8_t y, bool highlighted);
void drawBone_Small(uint8_t pipsTop, uint8_t pipsBottom, uint8_t x, uint8_t y, bool highlighted);
void drawBone_Rotated(uint8_t boneValue, uint8_t x, uint8_t y, bool highlighted);
void drawBone_Rotated(uint8_t pipsLeft, uint8_t pipsRight, uint8_t x, uint8_t y, bool highlighted);
void drawBone_Rotated_Erase(uint8_t boneValue, uint8_t x, uint8_t y);
void drawBone_Outline(uint8_t x, uint8_t y, bool highlighted);
void drawBone_Outline_Rotated(uint8_t x, uint8_t y, bool highlighted);
void drawBone_Hidden(uint8_t x, uint8_t y, bool highlighted);
void drawSplash_Loop();
void drawTitle_Loop();
Player getFirstPlayer(GameMode gameMode);
Player getSecondPlayer(GameMode gameMode);
Player getAltPlayer(GameMode gameMode, Player player);
void shuffleBones();
void dealBones(GameMode gameMode);
uint8_t getPipCount();
uint8_t getPipCount(uint8_t outerPips, uint8_t boneId);
bool isDouble(uint8_t boneId);
uint8_t leftPips_ByBoneValue(uint8_t boneValue);
uint8_t rightPips_ByBoneValue(uint8_t boneValue);
uint8_t leftPips_ByBoneId(uint8_t boneId);
uint8_t rightPips_ByBoneId(uint8_t boneId);
uint8_t playBone_NewPipValue(uint8_t existingBonePips, uint8_t newBoneId);
boolean hasXAxisBeenPlayed();
boolean hasADoubleBeenPlayed(uint8_t currentMode);
boolean canBlock(uint8_t pips);
void populateBoneCounts(Player player);
uint8_t handValue(Player player);
boolean canPlay(Player player);
boolean canPlayBone(uint8_t boneId);
boolean canEitherPlayerMove(GameMode gameMode);
void flashRedLED();
void flashGreenLED();
void DelayOrButtonPress(uint8_t delayVal);
uint8_t WaitForButtonPress();
void drawHorizontalDottedLine(uint8_t x1, uint8_t x2, uint8_t y);
boolean isTheGameOver(GameMode gameMode);
boolean isAnyoneOut(GameMode gameMode);
void printHand();

Arduboy2Ext arduboy;  
Font4x6 font4x6 = Font4x6();
GameMode gameMode;
TitleMode titleMode = TitleMode::PressA;

Name player0;
Name player1;
Name player2;

Name * players[3] = { &player0, &player1, &player2 };

#ifdef SOUNDS
  ArduboyTones sound(arduboy.audio.enabled);
#endif
                                                    // The 'bones_ref' array contains an element for each bone in a standard
                                                    // 28 bone set.  Arrays that represent players hands, bones available or 
                                                    // bones played all contain indexes that point to this array.

                            //    0     1     2     3     4     5     6
uint8_t bones_ref[BONES_COUNT] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                            //    7     8     9    10    11    12  
                               0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 
                            //   13    14    15    16    17
                               0x22, 0x23, 0x24, 0x25, 0x26, 
                            //   18    19    20    21
                               0x33, 0x34, 0x35, 0x36, 
                            //   22    23    24
                               0x44, 0x45, 0x46, 
                            //   25    26
                               0x55, 0x56,
                            //   27
                               0x66};
                               
uint8_t bones_available[BONES_COUNT] = {0};            // Array of bones that the players can draw from. Each array value 
uint8_t bones_available_idx = 0;                       // points to an entry in the 'bones_ref' array which contains the 
                                                       // actual bone details (left and right pip values). The next available
                                                       // bone is indicated by the 'bones_available_idx' variable.

uint8_t bones_played[BONES_COUNT] = {0};               // Array of bones played. Again, each array value points to an entry
uint8_t bones_played_idx = 0;                          // in the 'bones_ref' array.

uint8_t players_hand[3][BONES_COUNT] = {0};            // Multi-dimensional array that holds the two player's hand.  Bones 
uint8_t players_hand_idx[3] = {0};                     // will fill the lowest positions in the array - when a bone is played
                                                       // from a hand, the bones are shuffled to the 'left' of the array.
                                                       // The 'players_hand_idx[]' indicates the number of bones in each hand.  

uint8_t players_score[3] = {0};                        // Players score.

                                                       // When making a move, the player must select a bone from their hand
                                                       // followed by a position on the board.  Alternatively, they can select
                                                       // a bone from the graveyard to add to their hand (although in reality
                                                       // they will always get the next bone from the 'bones_available' array.

uint8_t players_hand_visible_idx = 0;                  // Only six player's bones are visible at one time and the player can
                                                       // scroll through these.  The 'players_hand_visible_idx' defines which
                                                       // should be displayed in the left most position on the display.
uint8_t players_hand_highlight_idx = 0;                // Used to store the currently selected bone.
uint8_t graveyard_highlight_idx = NOTHING;             // Used to store the currently selected graveyard bone.
Position board_highlighted_idx = Position::Nothing;    // Used to store the currently seelcted board location.  

uint8_t boneCounts_Overall[7] = {0};                   // Used to count how many of each bone is visible on the board.
uint8_t boneCounts_Inhand[7] = {0};                    // Used to count how many of each bone is visible in the players hand.

uint8_t bone_n = NOTHING;                              // Stores the index reference (to bones_ref) for each board position.
uint8_t bone_s = NOTHING;
uint8_t bone_e = NOTHING;
uint8_t bone_w = NOTHING;
uint8_t bone_c = NOTHING;

uint8_t bone_n_pips_inner = NOTHING;                    // Stores the inner and outer PIP count for each board position.
uint8_t bone_s_pips_inner = NOTHING;
uint8_t bone_e_pips_inner = NOTHING;
uint8_t bone_w_pips_inner = NOTHING;
uint8_t bone_n_pips_outer = NOTHING;
uint8_t bone_s_pips_outer = NOTHING;
uint8_t bone_e_pips_outer = NOTHING;
uint8_t bone_w_pips_outer = NOTHING;
uint8_t bone_c_pips_inner = NOTHING;
uint8_t bone_c_pips_outer = NOTHING;

uint8_t boardMode = BOARD_MODE_NO_BONES_PLAYED;        // Stores the current mode of the board which is used to determine what
                                                       // board positions should be rendered and selectable when playing a bone.
                                                       // For example, when the boardMode is equal to 'NO_BONES_PLAYED' only the
                                                       // center bone of the board is rendered and selectable.

uint8_t frame = 0;
Player playersTurn = Player::Human_1;                  // Used to retain the players turn at the end of each round.

GameState gameState = GameState::SplashScreen;    
uint8_t playerBeingEdited = 1;

/* ----------------------------------------------------------------------------
 *  Initialise the Arduboy and get ready ..
 */
void setup() {

	arduboy.boot();
	arduboy.flashlight();
	arduboy.systemButtons();
	arduboy.initRandomSeed();

  EEPROM_Utils::initEEPROM(false);

  player0.setChar(0, 'C');
  player0.setChar(1, 'o');
  player0.setChar(2, 'm');
  player0.setChar(3, 'p');
  player0.setChar(4, 'u');
  player0.setChar(5, 't');
  player0.setChar(6, 'e');
  player0.setChar(7, 'r');
  player0.setChar(8, 0);

#ifdef SOUNDS
  arduboy.audio.begin();
#endif

}
    

/* ----------------------------------------------------------------------------
 *  Play dominoes !
 */
void loop() {

  if (!(arduboy.nextFrame())) return;
  
  arduboy.pollButtons();

  switch (gameState) {

    case GameState::SplashScreen:
      drawSplash_Loop();
      break;

    case GameState::TitleScreen:
      drawTitle_Loop();
      break;

    case GameState::HighScore:
      drawHighScore_Loop(nullptr);
      break;

    case GameState::GetNames:
      if (gameMode == GameMode::SinglePlayer) {
        playersTurn = Player::Human_1; 
      }
      else {
        playersTurn = Player::Human_2; 
      }
      playerBeingEdited = 1;
      getNames_Loop();
      break;

    case GameState::PlayGame:
      playGame_Loop();
      break;

    case GameState::PlayComputer:
      computersHand_Loop();
      break;

    case GameState::SwapPlayers:
      swapPlayers(false);
      break;

    case GameState::GameOver:
      gameState = GameState::HighScore;
      titleMode = TitleMode::PressA;
      if (gameMode == GameMode::SinglePlayer){
        drawHighScore_Loop(players[getAltPlayer(gameMode, playersTurn)]);
      }
      else {
        drawHighScore_Loop(players[playersTurn]);
      }

      break;

    default:
      playersHand_Loop(playersTurn, getAltPlayer(gameMode, playersTurn));
      break;

  }
      
}


/* ----------------------------------------------------------------------------
 *  Outer game loop.
 *  
 *  Game play continues until one player reaches 100 points.  
 */
void playGame_Loop() {

  while (gameState == GameState::PlayGame) {

    
    // Clear and reset the players hands, the available bones and the board ..
    
    memset(bones_available, 0, (sizeof(bones_available) / sizeof(bones_available[0])));
    memset(bones_played, 0, (sizeof(bones_played) / sizeof(bones_played[0])));

    boardMode = BOARD_MODE_NO_BONES_PLAYED;
    bones_available_idx = 0;
    bones_played_idx = 0; 
    players_hand_visible_idx = 0;
    
    memset(players_hand[0], 0, (sizeof(players_hand[0]) / sizeof(players_hand[0][0])));
    memset(players_hand_idx, 0, (sizeof(players_hand_idx) / sizeof(players_hand_idx[0])));
    
    players_hand_highlight_idx = 0;
    board_highlighted_idx = Position::Nothing;

    bone_n = NOTHING;
    bone_s = NOTHING;
    bone_e = NOTHING;
    bone_w = NOTHING;
    bone_c = NOTHING;

    bone_n_pips_inner = NOTHING;
    bone_s_pips_inner = NOTHING;
    bone_e_pips_inner = NOTHING;
    bone_w_pips_inner = NOTHING;
    bone_n_pips_outer = NOTHING;
    bone_s_pips_outer = NOTHING;
    bone_e_pips_outer = NOTHING;
    bone_w_pips_outer = NOTHING;
    bone_c_pips_inner = NOTHING;
    bone_c_pips_outer = NOTHING;
           

    // Shuffle and deal the bones ..

#ifdef DEBUG  
    randomSeed(3);          // Seed with a constant to enable debugging with a repeatable bone set. 
#else
    arduboy.initRandomSeed();
#endif

    shuffleBones();
    dealBones(gameMode);

#ifdef DEBUG_PLAYER_CANNOT_PLAY
    Serial.println("DEBUG_PLAYER_CANNOT_PLAY");
    bones_available_idx = 0;
    players_hand[static_cast<uint8_t>(Player::Computer)][0] = 26;  // 5:6
    players_hand[static_cast<uint8_t>(Player::Computer)][1] = 27;  // 6:6
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 25;     // 5:5 
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 0;      // 0:0
    players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 2;
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    printHand();
#endif

#ifdef DEBUG_PLAYER_CAN_FINISH_GAME
    Serial.println("DEBUG_PLAYER_CAN_FINISH_GAME");
    bones_available_idx = 5;
    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 26;  // 5:6
      players_hand[static_cast<uint8_t>(Player::Computer)][1] = 27;  // 6:6
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 2;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 26;  // 5:6
      players_hand[static_cast<uint8_t>(Player::Human_2)][1] = 27;  // 6:6
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 2;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 25;     // 5:5 
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 1;
    playBone(gameMode, gameMode == GameMode::SinglePlayer ? Player::Computer : Player::Human_2, 0, Position::Center);
    printHand();
#endif

#ifdef DEBUG_PLAYER_A_FEW_TILES_EACH
    Serial.println("DEBUG_PLAYER_A_FEW_TILES_EACH");
    bones_available_idx = 5;
    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 26;  // 5:6
      players_hand[static_cast<uint8_t>(Player::Computer)][1] = 27;  // 6:6
      players_hand[static_cast<uint8_t>(Player::Computer)][2] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 3;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 26;  // 5:6
      players_hand[static_cast<uint8_t>(Player::Human_2)][1] = 27;  // 6:6
      players_hand[static_cast<uint8_t>(Player::Human_2)][2] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 3;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 25;     // 5:5 
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 22;     // 4:$
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    playBone(gameMode, gameMode == GameMode::SinglePlayer ? Player::Computer : Player::Human_2, 0, Position::Center);
    printHand();
#endif
    
#ifdef DEBUG_COMPUTER_CAN_FINISH_GAME
    Serial.println("DEBUG_COMPUTER_CAN_FINISH_GAME");
    bones_available_idx = 5;
    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 25;  // 5:5
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 1;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 25;  // 5:5
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 1;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 5;     // 5:0 
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 27;     // 6:6 
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    printHand();
#endif
    
#ifdef DEBUG_NO_ONE_CAN_PLAY_1
    // first player plays 4:6 and then stalemate.
    Serial.println("DEBUG_NO_ONE_CAN_PLAY_1");
    bones_available_idx = 0;
    boardMode = BOARD_MODE_ONE_BONE_PLAYED;

    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 25;  // 5:5
      players_hand[static_cast<uint8_t>(Player::Computer)][1] = 20;  // 5:3
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 2;
      players_score[static_cast<uint8_t>(Player::Computer)] = 30;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 25;  // 5:5
      players_hand[static_cast<uint8_t>(Player::Human_2)][1] = 20;  // 5:3
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 2;
      players_score[static_cast<uint8_t>(Player::Human_2)] = 30;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 9;     // 1:3 
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 24;    // 4:6 
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    players_score[static_cast<uint8_t>(Player::Human_1)] = 50;
    bone_c = 4;
    bone_c_pips_inner = 0;
    bone_c_pips_outer = 4;
    printHand();
#endif
    
#ifdef DEBUG_NO_ONE_CAN_PLAY_2
    // first player has to pass, second player plays 4:6 and then stalemate.
    Serial.println("DEBUG_NO_ONE_CAN_PLAY_2");
    bones_available_idx = 0;
    boardMode = BOARD_MODE_ONE_BONE_PLAYED;

    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 9;  // 1:3
      players_hand[static_cast<uint8_t>(Player::Computer)][1] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 2;
      players_score[static_cast<uint8_t>(Player::Computer)] = 50;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 9;  // 1:2
      players_hand[static_cast<uint8_t>(Player::Human_2)][1] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 2;
      players_score[static_cast<uint8_t>(Player::Human_2)] = 50;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 25;     // 5:5 
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 20;    // 5:3
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    players_score[static_cast<uint8_t>(Player::Human_1)] = 30;
    bone_c = 4;
    bone_c_pips_inner = 0;
    bone_c_pips_outer = 4;
    printHand();
#endif
   
#ifdef DEBUG_CORRUPTED_GRAPHICS
    // first player has to pass, second player plays 4:6 and then stalemate.
    Serial.println("DEBUG_CORRUPTED_GRAPHICS");

    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 11;  // 5:1
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 11;  // 5:1
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 25;     // 5:5 
    printHand();
#endif
 
#ifdef DEBUG_ALLSIDES_CANNOT_PLAY
    // first player has to pass, second player plays 4:6 and then stalemate.
    // first player has to pass, second player plays 4:6 and then stalemate.
    Serial.println("DEBUG_ALLSIDES_CANNOT_PLAY");
    bones_available_idx = 0;
    boardMode = BOARD_MODE_X_AXIS_PLAYED;

    if (gameMode == GameMode::SinglePlayer) {
      players_hand[static_cast<uint8_t>(Player::Computer)][0] = 9;  // 1:3
      players_hand[static_cast<uint8_t>(Player::Computer)][1] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Computer)] = 2;
      players_score[static_cast<uint8_t>(Player::Computer)] = 50;
    }
    else {
      players_hand[static_cast<uint8_t>(Player::Human_2)][0] = 9;  // 1:2
      players_hand[static_cast<uint8_t>(Player::Human_2)][1] = 24;  // 4:6
      players_hand_idx[static_cast<uint8_t>(Player::Human_2)] = 2;
      players_score[static_cast<uint8_t>(Player::Human_2)] = 50;
    }
    players_hand[static_cast<uint8_t>(Player::Human_1)][0] = 4;     // 0:4
    players_hand[static_cast<uint8_t>(Player::Human_1)][1] = 1;    // 0:1
    players_hand_idx[static_cast<uint8_t>(Player::Human_1)] = 2;
    players_score[static_cast<uint8_t>(Player::Human_1)] = 30;
    bone_c = 22;
    bone_c_pips_inner = 4;
    bone_c_pips_outer = 4;
    bone_n = 21;
    bone_n_pips_inner = 3;
    bone_n_pips_outer = 6;
    bone_e = 23;
    bone_e_pips_inner = 4;
    bone_e_pips_outer = 5;
    bone_s = 16;
    bone_s_pips_inner = 5;
    bone_s_pips_outer = 2;
    bone_w = 2;
    bone_w_pips_inner = 0;
    bone_w_pips_outer = 2;
    printHand();
#endif

//                             //    0     1     2     3     4     5     6
// uint8_t bones_ref[BONES_COUNT] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
//                             //    7     8     9    10    11    12  
//                                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 
//                             //   13    14    15    16    17
//                                0x22, 0x23, 0x24, 0x25, 0x26, 
//                             //   18    19    20    21
//                                0x33, 0x34, 0x35, 0x36, 
//                             //   22    23    24
//                                0x44, 0x45, 0x46, 
//                             //   25    26
//                                0x55, 0x56,
//                             //   27
//                                0x66};

    // Render the screen ..
    
    arduboy.clear();
    renderGraveyard(playersTurn, NOTHING, players_hand_highlight_idx, true);
    gameState = (playersTurn == Player::Computer ? GameState::PlayComputer : GameState::PlayHuman_HandSel);

    if (gameMode == GameMode::SinglePlayer) {
      renderBoard(gameMode, playersTurn, Position::Nothing, false);
    }
    else {

      swapPlayers(true);

    }

  }

}

/* ----------------------------------------------------------------------------
 *  Swap players.
 */
void swapPlayers(bool startOfGame) {

  playersTurn = (playersTurn == Player::Human_1 ? Player::Human_2 : Player::Human_1);
  
  Player altPlayer = getAltPlayer(gameMode, playersTurn);

  renderPlayerScore(HandPosition::Top, altPlayer, false);
  renderPlayerScore(HandPosition::Bottom, playersTurn, false);
  renderPlayersHand(playersTurn, NOTHING, true);

  if (startOfGame) {
    drawMessageBox(F("You~can~start\nthe~new~game,"), 3);
    font4x6.setCursor(63,31);
  }
  else {
    drawMessageBox(F("Its~your~turn"), 2);
    font4x6.setCursor(63,23);
  }

  font4x6.setTextColor(BLACK);
  font4x6.print(players[static_cast<uint8_t>(playersTurn)]->getString());
  font4x6.print("!");
  font4x6.setTextColor(WHITE);
  arduboy.display();

  arduboy.pollButtons();
  DelayOrButtonPress(200);

  gameState = GameState::PlayHuman_HandSel;

}
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Enums.h"
#include "src/utils/Name.h"

   
Name *name;

void getNames_Loop() {

  players[1]->setStartLoc(EEPROM_PLAYER2);
  players[2]->setStartLoc(EEPROM_PLAYER3);

  name = players[1];

  while (gameState == GameState::GetNames) {

    if (!(arduboy.nextFrame())) {
      delay(5);
    };
  
    arduboy.pollButtons();

    auto justPressed = arduboy.justPressedButtons();

    uint8_t charIndex = name->getCharIndex();

    if (justPressed & LEFT_BUTTON) 						 	{ name->decCharIndex(); } 
    if (justPressed & RIGHT_BUTTON) 					 	{ name->incCharIndex(); } 

    if (justPressed & UP_BUTTON) { 

      name->incChar(charIndex); 

    }
    else if (justPressed & DOWN_BUTTON) { 

      name->decChar(charIndex); 

    }
    


    // Go back a player or to number of players screen ..

    if (justPressed & B_BUTTON) { 

      if (playerBeingEdited > 1) {
      
        playerBeingEdited--;
        name = players[playerBeingEdited];

      }
      else {

        gameState = GameState::TitleScreen;

      }

    }


    // Go forward to next player or start game ..

    if (justPressed & A_BUTTON) { 

      bool allSpaces = true;
      
      for (uint8_t x = 0; x < NAME_LENGTH; x++) {

        if (static_cast<uint8_t>(name->getChar(x)) != 32 && static_cast<uint8_t>(name->getChar(x)) != 0) allSpaces = false;

      }			

      if (!allSpaces) {

        if (playerBeingEdited == 1 && gameMode == GameMode::TwoPlayer)  {
          
            playerBeingEdited++;
            name = players[playerBeingEdited];
        
        }
        else {

          gameState = GameState::PlayGame;

        }

      }

    }


    // -----------------------------------------------------------------------------------------
    //  Render details ..
    // -----------------------------------------------------------------------------------------
    
    const bool flash = arduboy.getFrameCountHalf(FLASH_FRAME_COUNT_2);

    renderSideBones();
    // Sprites::drawOverwrite(1, 0, Images::HighScore_LHS, 0);
    // Sprites::drawOverwrite(101, 0, Images::HighScore_RHS, 0);
    Sprites::drawOverwrite(33, 5, Images::Players, 0);

    font4x6.setCursor(45, 30);
    font4x6.print(F("Player "));
    font4x6.print(playerBeingEdited);


    arduboy.drawHorizontalDottedLine(42, 84, 54);

    for (uint8_t x = 0; x < NAME_LENGTH; x++) {

      font4x6.setCursor(42 + (x * NAME_CHARACTER_SPACING), 46);

      if (flash && name->getCharIndex() == x) {
        arduboy.drawFastHLine(42 + (x * NAME_CHARACTER_SPACING), 54, 4, WHITE);
      }

      font4x6.print(name->getChar(x));

    }

    arduboy.display(true);

  }

}

void renderSideBones() {
  Sprites::drawOverwrite(1, 0, Images::HighScore_LHS, 0);
  Sprites::drawOverwrite(101, 0, Images::HighScore_RHS, 0);
}
#include "src/utils/Arduboy2Ext.h"

// ----------------------------------------------------------------------------
//  Initialise state ..
//
void drawHighScore_Loop(Name *name) {

	arduboy.setRGBled(0, 0, 0);
    
  uint8_t winnerIdx = NO_WINNER;
  uint8_t clearScores = 0;
  uint8_t flashCount = 255;

  char player1[NAME_LENGTH + 1];
  char player2[NAME_LENGTH + 1];
  char player3[NAME_LENGTH + 1];
  char player4[NAME_LENGTH + 1];
  char player5[NAME_LENGTH + 1];

  uint16_t score1 = 0;
  uint16_t score2 = 0;
  uint16_t score3 = 0;
  uint16_t score4 = 0;
  uint16_t score5 = 0;

  if (name != nullptr) winnerIdx = EEPROM_Utils::saveScore(name->getString());

  EEPROM_Utils::getName(player1, EEPROM_HS_NAME_1);
  EEPROM_Utils::getName(player2, EEPROM_HS_NAME_2);
  EEPROM_Utils::getName(player3, EEPROM_HS_NAME_3);
  EEPROM_Utils::getName(player4, EEPROM_HS_NAME_4);
  EEPROM_Utils::getName(player5, EEPROM_HS_NAME_5);

  score1 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_1);
  score2 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_2);
  score3 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_3);
  score4 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_4);
  score5 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_5);

  while (gameState == GameState::HighScore){

    if (!(arduboy.nextFrame())) { delay(20); };
    arduboy.pollButtons();

    auto justPressed = arduboy.justPressedButtons();
    auto pressed = arduboy.pressedButtons();



    // Clear scores ..

    if ((pressed & LEFT_BUTTON) && (pressed & RIGHT_BUTTON)) {

      clearScores++;

      switch (clearScores) {

        case 21 ... 60:

          arduboy.setRGBled(128 - (clearScores * 2), 0, 0);
          break;

        case 61:

          clearScores = 0;
          arduboy.setRGBled(0, 0, 0);
          EEPROM_Utils::initEEPROM(true);

          EEPROM_Utils::getName(player1, EEPROM_HS_NAME_1);
          EEPROM_Utils::getName(player2, EEPROM_HS_NAME_2);
          EEPROM_Utils::getName(player3, EEPROM_HS_NAME_3);
          EEPROM_Utils::getName(player4, EEPROM_HS_NAME_4);
          EEPROM_Utils::getName(player5, EEPROM_HS_NAME_5);

          score1 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_1);
          score2 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_2);
          score3 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_3);
          score4 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_4);
          score5 = EEPROM_Utils::getHighScore(EEPROM_HS_SCORE_5);

          winnerIdx = NO_WINNER;

          break;

      }

    }
    else {

      if (clearScores > 0) {
      
        arduboy.setRGBled(0, 0, 0);
        clearScores = 0;

      }
      
    }


    // Handle other input ..

    if (justPressed & A_BUTTON) {
      gameState = GameState::TitleScreen; 
    }



    const bool flash = arduboy.getFrameCountHalf(FLASH_FRAME_COUNT_2);
    if (flashCount > 0 && arduboy.getFrameCount(FLASH_FRAME_COUNT_2)) flashCount--;
    if (flashCount == 0) winnerIdx = NO_WINNER;

    renderSideBones();
    // Sprites::drawOverwrite(1, 0, Images::HighScore_LHS, 0);
    // Sprites::drawOverwrite(101, 0, Images::HighScore_RHS, 0);
    Sprites::drawOverwrite(31, 3, Images::HighScore_Top, 0);

    if ((flash && winnerIdx == 0) || winnerIdx != 0) {

      font4x6.setCursor(32, 20);
      font4x6.print(player1);
      renderScore(score1, 82, 20);

    }

    if ((flash && winnerIdx == 1) || winnerIdx != 1) {

      font4x6.setCursor(32, 29);
      font4x6.print(player2);
      renderScore(score2, 82, 29);

    }

    if ((flash && winnerIdx == 2) || winnerIdx != 2) {

      font4x6.setCursor(32, 38);
      font4x6.print(player3);
      renderScore(score3, 82, 38);

    }

    if ((flash && winnerIdx == 3) || winnerIdx != 3) {

      font4x6.setCursor(32, 47);
      font4x6.print(player4);
      renderScore(score4, 82, 47);

    }

    if ((flash && winnerIdx == 4) || winnerIdx != 4) {

      font4x6.setCursor(32, 56);
      font4x6.print(player5);
      renderScore(score5, 82, 56);

    }

    arduboy.display(true);

  }

}


void renderScore(int16_t score, uint8_t x, uint8_t y) {
    
  uint8_t digits[3] = {};
  extractDigits(digits, static_cast<uint16_t>(absT(score)));

  for (uint8_t j = 3, x2 = x; j > 0; --j, x2 += 5) {

    font4x6.setCursor(x2, y);
    font4x6.print(digits[j - 1]);
  }

}
#include "src/utils/Arduboy2Ext.h"

#define MESSAGE_BOX_DELAY_INIT      500
#define MESSAGE_BOX_DELAY           20
#define MESSAGE_BOX_DELAY_SHORT     5

#define MESSAGE_BOX_X               BONES_GRAVEYARD_X
#define MESSAGE_BOX_X_MAX           BONES_GRAVEYARD_X_MAX
#define MESSAGE_BOX_Y               BONES_GRAVEYARD_Y
#define MESSAGE_BOX_Y_MAX           BONES_GRAVEYARD_Y_MAX - 4
#define MESSAGE_BOX_Y_2_LINE        MESSAGE_BOX_Y + 10
#define MESSAGE_BOX_Y_3_LINE        MESSAGE_BOX_Y + 7

#define MESSAGE_BOX_EXIT_CIRCLE_X   115
#define MESSAGE_BOX_EXIT_CIRCLE_Y   38
#define MESSAGE_BOX_EXIT_CIRCLE_RAD 5


void drawTalkBubble(uint8_t lines) {

  arduboy.fillRect(BONES_GRAVEYARD_X, BONES_GRAVEYARD_Y, BONES_GRAVEYARD_X_MAX, BONES_GRAVEYARD_Y_MAX, BLACK);

  uint8_t y1 = (lines == 2 ? 3 : 2);
  uint8_t y2 = (lines == 2 ? 23 : 26);

  Sprites::drawSelfMasked(MESSAGE_BOX_X, MESSAGE_BOX_Y + y1, Images::Talk, 0);
  Sprites::drawSelfMasked(MESSAGE_BOX_X, MESSAGE_BOX_Y + y2, Images::Talk, 1);

}

/* ----------------------------------------------------------------------------
    Render a message box with an animated hourglass.

    Render a message box with an animated hourglass when the computer is about
    to play a bone.  The animation can get quite annoying - especially when
    debugging so it can conditionally be suppressed by removing the definition
    ANIMATIONS.  The user can skip the animation by pressing the A or B button.
    Once the animation is complete, a confirmation of the bone being played 
    is shown.

*/
void drawMessageBox_WithHourglass(uint8_t boneIdx) {

#ifdef ANIMATIONS
  uint8_t rotations = 2;
#endif

  arduboy.fillRect(BONES_GRAVEYARD_X, BONES_GRAVEYARD_Y, BONES_GRAVEYARD_X_MAX, BONES_GRAVEYARD_Y_MAX, BLACK);

#ifdef ANIMATIONS

  while (rotations > 0) {

    if ((arduboy.nextFrame())) {

      drawTalkBubble(3);
      Sprites::drawErase(MESSAGE_BOX_X + 23, MESSAGE_BOX_Y + 7, Images::hourglass, frame);

      if (arduboy.everyXFrames(3)) frame++;

      if (frame > 11 || (rotations == 1 && frame == 9)) {

        rotations--;
        frame = 0;

      }

      arduboy.display();

    }

    if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) {
      break;
    }

  }

#endif

  frame = 0;

  drawTalkBubble(3);
  font4x6.setTextColor(BLACK);
  font4x6.setCursor(MESSAGE_BOX_X + 4, MESSAGE_BOX_Y_2_LINE);
  font4x6.print(F("I will play\nthe    ~bone."));
  drawBone_Rotated_Erase(bones_ref[boneIdx], MESSAGE_BOX_X + 22, 21);
  font4x6.setTextColor(WHITE);

  arduboy.display();
  DelayOrButtonPress(MESSAGE_BOX_DELAY);

}


/* ----------------------------------------------------------------------------
 *  Render a simple message.
 */
void drawMessageBox(String message, uint8_t lines, bool waitForButtonPress) {

  drawMessageBox(message, lines, waitForButtonPress, Player::None);

}

void drawMessageBox(String message, uint8_t lines, bool waitForButtonPress, Player renderPlayerScore) {

  arduboy.delayShort(MESSAGE_BOX_DELAY_INIT);

  drawTalkBubble(lines);

  font4x6.setTextColor(BLACK);
  font4x6.setCursor(MESSAGE_BOX_X + 4, (lines == 2 ? MESSAGE_BOX_Y_2_LINE : MESSAGE_BOX_Y_3_LINE));
  font4x6.print(message);
  font4x6.setTextColor(WHITE);

  arduboy.display();


  // Should a players score be updated?

  if (renderPlayerScore != Player::None) {
    renderPlayersScore(gameMode, renderPlayerScore);
  }


  // Should the message box be dismissed after a delay or wait until a button is pressed?

  if (waitForButtonPress) {

    WaitForButtonPress();

  }
  else {

    DelayOrButtonPress((renderPlayerScore == Player::None ? MESSAGE_BOX_DELAY : MESSAGE_BOX_DELAY_SHORT));

  }

}


void drawMessageBox(String message, uint8_t lines) {

  drawTalkBubble(lines);
  font4x6.setTextColor(BLACK);
  font4x6.setCursor(MESSAGE_BOX_X + 4, (lines == 2 ? MESSAGE_BOX_Y_2_LINE : MESSAGE_BOX_Y_3_LINE));
  font4x6.print(message);
  font4x6.setTextColor(WHITE);

}

/* ----------------------------------------------------------------------------
 *  Render an exit message box.
 */
boolean exitGame() {

  drawTalkBubble(3);
  font4x6.setTextColor(BLACK);
  font4x6.setCursor(MESSAGE_BOX_X + 4, MESSAGE_BOX_Y_3_LINE);
  font4x6.print(F("~Abandon~game\n~and~return?"));
    
  font4x6.setCursor(MESSAGE_BOX_X + 4, MESSAGE_BOX_Y_3_LINE + 22);
  font4x6.print(F("~Confirm"));
  Sprites::drawErase(113, 27, Images::BOnly, 0);
  font4x6.setTextColor(WHITE);
  arduboy.display();

  arduboy.delayShort(250);

  return (WaitForButtonPress() == B_BUTTON ? true : false);

}


#include "src/utils/Arduboy2Ext.h"

/* ----------------------------------------------------------------------------
 *  Play the computer's hand.
 *  
 */
void computersHand_Loop() {

  uint8_t boneIdx = NOTHING;
  Position bonePosition = Position::Nothing;
  
  players_hand_highlight_idx = 0;
  players_hand_visible_idx = 0;
  board_highlighted_idx = Position::Nothing;

  while (gameState == GameState::PlayComputer) {

    bool drawBone = false;

    
    // Can a scoring play be made ?
    
    highestPossibleScore(Player::Computer, &boneIdx, &bonePosition);
    
    if (boneIdx != NOTHING) {
    
      drawMessageBox_WithHourglass(players_hand[static_cast<uint8_t>(Player::Computer)][boneIdx]);
      playBone(gameMode, Player::Computer, boneIdx, bonePosition);
       
    }
    else {


      // If not, what move will drive the PIP count down?
      
      lowestPossibleScore(Player::Computer, &boneIdx, &bonePosition);
        
      if (boneIdx != NOTHING) {

        drawMessageBox_WithHourglass(players_hand[static_cast<uint8_t>(Player::Computer)][boneIdx]);
        playBone(gameMode, Player::Computer, boneIdx, bonePosition);
        gameState == GameState::PlayHuman_HandSel;
           
      }
      else {

        
        // No bones can be played so draw ..

        if (bones_available_idx > 0) {

          players_hand[static_cast<uint8_t>(Player::Computer)][players_hand_idx[static_cast<uint8_t>(Player::Computer)]] = bones_available[bones_available_idx];
          drawMessageBox(F("~I~can't~go.~I\n~will~draw~an\n~extra~bone."), 3, false);

          players_hand_idx[static_cast<uint8_t>(Player::Computer)] = players_hand_idx[static_cast<uint8_t>(Player::Computer)] + 1;
          bones_available_idx--;
          renderPlayerScore(HandPosition::Top, Player::Computer, false);
          renderGraveyard(Player::Computer, NOTHING, NOTHING, false);
          drawBone = true;
        
        }
        else {  // No bones to draw so pass ..

          gameState = (canPlay(Player::Human_1) ? GameState::PlayComputer : GameState::PlayGame);

          if (gameState == GameState::PlayComputer) {

            drawMessageBox(F("~I~can't~go.~I\n~will~have~to\n~pass."), 3, true);
            gameState == GameState::PlayHuman_HandSel;

          }
          else {

            char message[] = "No~one~can~go\nand~this~game\nis~a~draw!";

            uint8_t player1HandVal = handValue(Player::Human_1);
            uint8_t player2HandVal = handValue(Player::Computer);

            if (player1HandVal < player2HandVal) {
              players_score[static_cast<uint8_t>(Player::Human_1)] = players_score[static_cast<uint8_t>(Player::Human_1)] + (player2HandVal - player1HandVal);
              drawMessageBox(message, 3, false);
              renderPlayersScore(Player::Human_1, HandPosition::Bottom);
              WaitForButtonPress();
            }
            else if (player1HandVal < player2HandVal) {
              players_score[static_cast<uint8_t>(Player::Computer)] = players_score[static_cast<uint8_t>(Player::Computer)] + (player1HandVal - player2HandVal);
              drawMessageBox(message, 3, false);
              renderPlayersScore(Player::Computer, HandPosition::Top);
              WaitForButtonPress();
            }
            else {
              drawMessageBox(message, 3, true);
            }

          }

        }
        
      }
    
    }

    boneIdx = NOTHING;
    bonePosition = bonePosition;

    
    // Pass control to the human if the game is still in play or finish the game ..
    
    if (!drawBone) {

      gameState = (isAnyoneOut(gameMode) ? GameState::PlayGame : gameState);
      gameState = (isTheGameOver(gameMode) ? GameState::GameOver : gameState);

      if (gameState == GameState::PlayComputer) {
        gameState = (canEitherPlayerMove(gameMode) ? GameState::PlayHuman_HandSel: GameState::PlayComputer);
      }
      
    }
    
    arduboy.delayShort(100);
  
  }


  // The playersTurn variable is used to record whose turn it is between rounds ..
  
  playersTurn = Player::Human_1;

}

#include "src/utils/Arduboy2Ext.h"


/* ----------------------------------------------------------------------------
 *  Play the human's hand.
 *  
 */
void playersHand_Loop(Player player, Player secondPlayer) {

  uint8_t playerIdx = static_cast<uint8_t>(player);

  players_hand_highlight_idx = 0;
  players_hand_visible_idx = 0;
  renderGraveyard(player, NOTHING, players_hand_highlight_idx, false);

  while (gameState >= GameState::PlayHuman_HandSel) {

    arduboy.pollButtons();
    uint8_t justPressed = arduboy.justPressedButtons();


    
    // If the human cannot play a bone or draw one from the graveyard, the hand is over ..
    
    if (!canPlay(player) && bones_available_idx == 0) {

      gameState = (canPlay(secondPlayer) && !canPlay(player) ? (gameMode == GameMode::SinglePlayer ? GameState::PlayComputer : GameState::SwapPlayers) : GameState::PlayGame);

      if (gameState == GameState::PlayGame) {

        char message[] = ("Stalemate!~No\none~can~play\na~bone.");

        uint8_t player1HandVal = handValue(player);
        uint8_t player2HandVal = handValue(secondPlayer);

        if (player1HandVal < player2HandVal) {
          players_score[static_cast<uint8_t>(player)] = players_score[static_cast<uint8_t>(player)] + (player2HandVal - player1HandVal);
          drawMessageBox(message, 3, false);
          renderPlayersScore(player, HandPosition::Bottom);
          WaitForButtonPress();
        }
        else if (player1HandVal > player2HandVal) {
          players_score[static_cast<uint8_t>(secondPlayer)] = players_score[static_cast<uint8_t>(secondPlayer)] + (player1HandVal - player2HandVal);
          drawMessageBox(message, 3, false);
          renderPlayersScore(secondPlayer, HandPosition::Top);
          WaitForButtonPress();
        }
        else {
          drawMessageBox(message, 3, true);
        }

      }
      else {

//      drawMessageBox(F("You~can't~play~a\nbone~and~have\nto~pass."), 3, true);
        drawMessageBox(F(" You~can~not\n play~a~bone\n and~pass."), 3, true);

      }
      
    }

  
    // The human can play or draw so handle the actions ..
          
    if (gameState >= GameState::PlayHuman_HandSel) { 

      switch (gameState) {

        case GameState::PlayHuman_HandSel:
  
          if ((justPressed &LEFT_BUTTON) && players_hand_highlight_idx > 0) {
  
            players_hand_highlight_idx--;
            if (players_hand_highlight_idx < players_hand_visible_idx) {
              players_hand_visible_idx--;
            }

            renderPlayersHand(player, players_hand_highlight_idx, false);
            
          }

          if ((justPressed & RIGHT_BUTTON) && players_hand_highlight_idx < players_hand_idx[playerIdx] - 1) {
          
            players_hand_highlight_idx++;
            if (players_hand_highlight_idx >= players_hand_visible_idx + PLAYER_BOTTOM_VISIBLE) {
              players_hand_visible_idx++;
            }

            renderPlayersHand(player, players_hand_highlight_idx, false);
            
          }
              
          if ((justPressed & UP_BUTTON) && bones_available_idx > 0) { 

            graveyard_highlight_idx = bones_available_idx - 1;
            gameState = GameState::PlayHuman_GraveSel;
            renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);
  
          }
  
          if ((justPressed & DOWN_BUTTON) && bones_available_idx > 0) { 
  
            graveyard_highlight_idx = 0;
            gameState = GameState::PlayHuman_GraveSel;
            renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);
  
          }
          
          if (justPressed & A_BUTTON) { 

            if (!canPlayBone(players_hand[playerIdx][players_hand_highlight_idx])) {

              flashRedLED();
              
            }
            else {

              flashGreenLED();
             
              gameState = GameState::PlayHuman_BoardSel;
              board_highlighted_idx = Position::Center;
              renderBoard(gameMode, player, board_highlighted_idx, false);

            }
            
          }
          
          if (justPressed & B_BUTTON) { 

            if (exitGame()) {
              gameState = GameState::TitleScreen;
            }
            else {
              renderGraveyard(player, NOTHING, players_hand_highlight_idx, false);
            }
            
          }

          break;

        case GameState::PlayHuman_GraveSel:
           
          if ((justPressed & LEFT_BUTTON) && graveyard_highlight_idx > 0) {
  
            graveyard_highlight_idx--;
            renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);
            
          }

          if (justPressed & RIGHT_BUTTON) {

            if (graveyard_highlight_idx < bones_available_idx - 1) {

              graveyard_highlight_idx++;
              renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);

            }
         
          }
           
          if (justPressed & UP_BUTTON) {
            
            if (graveyard_highlight_idx > BONES_GRAVEYARD_BONES_PER_ROW) {

              graveyard_highlight_idx = graveyard_highlight_idx - BONES_GRAVEYARD_BONES_PER_ROW;
              renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);
              
            }
            else {

              graveyard_highlight_idx = NOTHING;
              gameState = GameState::PlayHuman_HandSel;
              renderGraveyard(player, graveyard_highlight_idx, players_hand_highlight_idx, false);

            }
            
          }
           
          if (justPressed & DOWN_BUTTON) {
            
            if (graveyard_highlight_idx < bones_available_idx - BONES_GRAVEYARD_BONES_PER_ROW) {

              graveyard_highlight_idx = graveyard_highlight_idx + BONES_GRAVEYARD_BONES_PER_ROW;
              renderGraveyard(player, graveyard_highlight_idx, NOTHING, false);
              
            }
            else {

              graveyard_highlight_idx = NOTHING;
              gameState = GameState::PlayHuman_HandSel;
              renderGraveyard(player, graveyard_highlight_idx, players_hand_highlight_idx, false);

            }
            
          }

          if (justPressed & A_BUTTON) {

            players_hand[playerIdx][players_hand_idx[playerIdx]] = bones_available[bones_available_idx];
            players_hand_idx[playerIdx] = players_hand_idx[playerIdx] + 1;
            bones_available_idx--;

            players_hand_highlight_idx = players_hand_idx[playerIdx] - 1;
            players_hand_visible_idx = (players_hand_highlight_idx >= PLAYER_BOTTOM_VISIBLE ? players_hand_highlight_idx - PLAYER_BOTTOM_VISIBLE + 1: 0);
        
            board_highlighted_idx = Position::Nothing;
            graveyard_highlight_idx = NOTHING;

            gameState = GameState::PlayHuman_HandSel;
            renderGraveyard(player, NOTHING, players_hand_highlight_idx, false);
              
          }
          
          break;

        case GameState::PlayHuman_BoardSel:

          if ((justPressed & LEFT_BUTTON) && board_highlighted_idx == Position::Center && boardMode != BOARD_MODE_NO_BONES_PLAYED) {

            board_highlighted_idx = Position::West;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & LEFT_BUTTON) && board_highlighted_idx == Position::East) {

            board_highlighted_idx = Position::Center;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & RIGHT_BUTTON) && board_highlighted_idx == Position::Center && boardMode == BOARD_MODE_ONE_BONE_PLAYED) {

            board_highlighted_idx = Position::East;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & RIGHT_BUTTON) && board_highlighted_idx == Position::Center && boardMode > BOARD_MODE_TWO_BONES_PLAYED) {

            board_highlighted_idx = Position::East;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }
          
          if ((justPressed & RIGHT_BUTTON) && board_highlighted_idx == Position::West) {

            board_highlighted_idx = Position::Center;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & UP_BUTTON) && board_highlighted_idx == Position::Center && boardMode == BOARD_MODE_X_AXIS_PLAYED) {

            board_highlighted_idx = Position::North;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & UP_BUTTON) && board_highlighted_idx == Position::South && boardMode == BOARD_MODE_X_AXIS_PLAYED) {

            board_highlighted_idx = Position::Center;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & DOWN_BUTTON) && board_highlighted_idx == Position::Center && boardMode == BOARD_MODE_X_AXIS_PLAYED) {

            board_highlighted_idx = Position::South;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if ((justPressed & DOWN_BUTTON) && board_highlighted_idx == Position::North && boardMode == BOARD_MODE_X_AXIS_PLAYED) {

            board_highlighted_idx = Position::Center;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            
          }

          if (justPressed & A_BUTTON) {

            if (!canPlayBoard(players_hand[playerIdx][players_hand_highlight_idx], board_highlighted_idx)) {

              flashRedLED();
              
            }
            else {

              flashGreenLED();
              playBone(gameMode, player, players_hand_highlight_idx, board_highlighted_idx);
     
              players_hand_highlight_idx = NOTHING;
              players_hand_visible_idx = 0;
              board_highlighted_idx = Position::Nothing;

              if (gameMode == GameMode::SinglePlayer) {
                gameState = GameState::PlayComputer;
              }
              else {
                gameState = GameState::SwapPlayers;
              }

              renderPlayersHand(player, NOTHING, false);

            }
          
          }

          if (justPressed & B_BUTTON) {
          
            board_highlighted_idx = Position::Nothing;
            renderBoard(gameMode, player, board_highlighted_idx, false);
            gameState = GameState::PlayHuman_HandSel;
          
          }
          
          break;
          
        default: break;
      
      }

    }    


    // Pass control to the computer if the game is still in play or finish the game ..
      
    gameState = (isAnyoneOut(gameMode) ? GameState::PlayGame : gameState);
    gameState = (isTheGameOver(gameMode) ? GameState::GameOver : gameState);

    if (gameState >= GameState::PlayHuman_HandSel) {
      gameState = (canEitherPlayerMove(gameMode) ? gameState : GameState::PlayGame);
    }

    arduboy.delayShort(100);
  
  }


  // The playersTurn variable is used to record whose turn it is between rounds ..
  
  if (gameMode == GameMode::SinglePlayer) {

    playersTurn = Player::Computer;

  }

}

#include "src/utils/Arduboy2Ext.h"


/* ----------------------------------------------------------------------------
 *  The computer will attempt to:
 *  
 *  1)  Play a domino that scores.
 *  2)  Play a domino that blocks an end of the play using a bone whose pips are 
 *      fully visible in the his hand or the 'used' boness. 
 *  3)  Play a domino that reduces the pip count thus preventing other players
 *      scoring highly. 
 *  
 */

/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand.
 *  
 *  Iterate through the player's hand and return the best score-producing hand
 *  if one exists.  Returns the cards index (in the hand, zero-based) and the 
 *  cardinal location on the playing field to place it.
 *    
 */
void highestPossibleScore(Player player, uint8_t *retIndex, Position *retDirection) {

  uint8_t retValue = 0;
  uint8_t leftPips = NOTHING;
  uint8_t rightPips = NOTHING;
  bool isDouble = false;
    
  for (uint8_t x = 0; x < players_hand_idx[static_cast<uint8_t>(player)]; x++) {

      leftPips = leftPips;
      rightPips = rightPips;
      isDouble = (leftPips == rightPips);
      
      switch (boardMode) {
      
        case BOARD_MODE_NO_BONES_PLAYED:
          highestPossibleScore_CenterOnly(x, leftPips, rightPips, &retValue, retIndex, retDirection);
          break;
          
        case BOARD_MODE_ONE_BONE_PLAYED:

          if (bone_c_pips_outer == leftPips)    { highestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { highestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_inner == leftPips)    { highestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_inner == rightPips)   { highestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
                    
          break;

        case BOARD_MODE_TWO_BONES_PLAYED:
          
          if (bone_c_pips_outer == leftPips)    { highestPossibleScore_Center(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { highestPossibleScore_Center(x, leftPips, &retValue, retIndex, retDirection); }
          if (bone_w_pips_outer == leftPips)    { highestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_w_pips_outer == rightPips)   { highestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
          
          break;

      case BOARD_MODE_DOUBLE_BONE_PLAYED:

        if (bone_e != NOTHING) {                         // Center is occupied and east is played ..
          
          if (bone_e_pips_outer == leftPips) {  

            if (bone_w != NOTHING)              { highestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
            if (bone_w == NOTHING)              { highestPossibleScore_East_CenterOnly(x, rightPips, &retValue, retIndex, retDirection); }
          
          }
          
          if (bone_e_pips_outer == rightPips) { 

            if (bone_w != NOTHING)              { highestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
            if (bone_w == NOTHING)              { highestPossibleScore_East_CenterOnly(x, leftPips, &retValue, retIndex, retDirection); }
          
          }
         
        }

        if (bone_w != NOTHING) {                         // Center is occupied and west is played ..
          
          if (bone_w_pips_outer == leftPips) { 

            if (bone_e != NOTHING)              { highestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
            if (bone_e == NOTHING)              { highestPossibleScore_West_CenterOnly(x, rightPips, &retValue, retIndex, retDirection); }
          
          }
          
          if (bone_w_pips_outer == rightPips) { 

            if (bone_e != NOTHING)              { highestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
            if (bone_e == NOTHING)              { highestPossibleScore_West_CenterOnly(x, leftPips, &retValue, retIndex, retDirection); }
          
          }
         
        }

        if (bone_w == NOTHING && bone_e == NOTHING) {    // Center is occupied but the east and the west are empty ..

          if (bone_c_pips_outer == leftPips)    { highestPossibleScore_West_CenterOnly(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { highestPossibleScore_West_CenterOnly(x, leftPips, &retValue, retIndex, retDirection); }

        }          

        break;
    
      case BOARD_MODE_X_AXIS_PLAYED:

        if (bone_e_pips_outer == leftPips)      { highestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_e_pips_outer == rightPips)     { highestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == leftPips)      { highestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == rightPips)     { highestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }

        if (bone_n != NOTHING) {                         
          
          if (bone_n_pips_outer == leftPips)    { highestPossibleScore_North(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_n_pips_outer == rightPips)   { highestPossibleScore_North(x, leftPips, &retValue, retIndex, retDirection); }
          
        }
        else {
          
          if (bone_c_pips_outer == leftPips)    { highestPossibleScore_North(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { highestPossibleScore_North(x, leftPips, &retValue, retIndex, retDirection); }
        
        }

        if (bone_s != NOTHING) { 
          
          if (bone_s_pips_outer == leftPips)    { highestPossibleScore_South(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_s_pips_outer == rightPips)   { highestPossibleScore_South(x, leftPips, &retValue, retIndex, retDirection); }
          
        }
        else {
          
          if (bone_c_pips_outer == leftPips)    { highestPossibleScore_South(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { highestPossibleScore_South(x, rightPips, &retValue, retIndex, retDirection); }
        
        }

        break;
     
    }

  }  

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (North).
 *  
 *  Can the nominated bone be played in the North position?  If so, will it
 *  result in a score that is lower than all previously calculated ones ??
 *    
 */
void highestPossibleScore_North(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = bone_w_pips_outer + bone_e_pips_outer + (bone_s != NOTHING ? bone_s_pips_outer : 0) + (bone_n != NOTHING ? bone_n_pips_outer : 0) + pips;

  if (val % 5 == 0 && val > *retValue) { 
    *retValue = val; 
    *retDirection = Position::North;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (South).
 *  
 *  Can the nominated bone be played in the South position?  If so, will it
 *  result in a score that is lower than all previously calculated ones ??
 *    
 */
void highestPossibleScore_South(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = bone_w_pips_outer + bone_e_pips_outer + (bone_n != NOTHING ? bone_n_pips_outer : 0) + pips;

  if (val % 5 == 0 && val > *retValue) {  
    *retValue = val; 
    *retDirection = Position::South;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (Center).
 *  
 *  Can the nominated bone be played in the Center position?  If so, will it
 *  result in a score that is lower than all previously calculated ones ??
 *    
 */
void highestPossibleScore_Center(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = bone_w_pips_outer + bone_e_pips_outer + (bone_n != NOTHING ? bone_n_pips_outer : 0) + pips;

  if (val % 5 == 0 && val > *retValue) {  
    *retValue = val; 
    *retDirection = Position::South;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (Center Only).
 *  
 *  Can the nominated bone be played in the West position against the centre 
 *  bone? If so, will it result in a score that is lower than all previously 
 *  calculated ones ??  
 *    
 */
void highestPossibleScore_CenterOnly(uint8_t boneIdx, uint8_t leftPips, uint8_t rightPips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = leftPips + rightPips;

  if (val % 5 == 0 && val > *retValue) { 
    *retValue = val; 
    *retDirection = Position::Center;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (West).
 *  
 *  Can the nominated bone be played in the West position?  If so, will it
 *  result in a score that is lower than all previously calculated ones ??
 *    
 */
void highestPossibleScore_West(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  switch (boardMode) {
  
    case BOARD_MODE_ONE_BONE_PLAYED:
      {
        uint8_t val = bone_c_pips_outer + (pips * (isDouble ? 2 : 1));
      
        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }
      
      break;
      
    case BOARD_MODE_TWO_BONES_PLAYED:
      {
        uint8_t val = bone_c_pips_outer + pips;

        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }
      
      break;

    case BOARD_MODE_DOUBLE_BONE_PLAYED:
      {
        uint8_t val = bone_e_pips_outer + pips;
        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }      
      break;

    case BOARD_MODE_X_AXIS_PLAYED:
      {
        uint8_t val = (bone_n != NOTHING ? bone_n_pips_outer : 0) + (bone_s != NOTHING ? bone_s_pips_outer : 0) + bone_e_pips_outer + pips;

        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }
        
      }
      break;
      
  }
  
}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (West, Center Only).
 *  
 *  Can the nominated bone be played in the West position against the centre 
 *  bone? If so, will it result in a score that is lower than all previously 
 *  calculated ones ??  
 *    
 */
void highestPossibleScore_West_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = bone_c_pips_outer + bone_c_pips_inner + pips;

  if (val % 5 == 0 && val > *retValue) { 
    *retValue = (bone_c_pips_outer + bone_c_pips_inner + pips); 
    *retDirection = Position::West;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (East).
 *  
 *  Can the nominated bone be played in the East position?  If so, will it
 *  result in a score that is lower than all previously calculated ones ??
 *    
 */
void highestPossibleScore_East(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  switch (boardMode) {
  
    case BOARD_MODE_ONE_BONE_PLAYED:
      {
        uint8_t val = bone_c_pips_inner + (pips * (isDouble ? 2 : 1));

        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }

      }
      
      break;
      
    case BOARD_MODE_DOUBLE_BONE_PLAYED:     
      {
        uint8_t val = (bone_w_pips_outer + pips);

        if (val % 5 == 0 && val > *retValue) { 
          *retValue = (bone_w_pips_outer + pips); 
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }

      }
      
      break;
      
    case BOARD_MODE_X_AXIS_PLAYED:      
      {
        uint8_t val = (bone_n != NOTHING ? bone_n_pips_outer : 0) + (bone_s != NOTHING ? bone_s_pips_outer : 0) + bone_w_pips_outer + pips;
        
        if (val % 5 == 0 && val > *retValue) { 
          *retValue = val; 
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }

      }
      
      break;
      
  }
  
}


/* ----------------------------------------------------------------------------
 *  Calculate the best score-producing hand (East, Center Only).
 *  
 *  Can the nominated bone be played in the East position against the centre 
 *  bone? If so, will it result in a score that is lower than all previously 
 *  calculated ones ??  
 *    
 */
void highestPossibleScore_East_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {
  
  uint8_t val = bone_c_pips_outer + bone_c_pips_inner + pips;

  if (val % 5 == 0 && val > *retValue) { 
    *retValue = val; 
    *retDirection = Position::East;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score.
 *  
 *  If a scoring hand cannot be played, look for a bone that reduces the overall
 *  pip score as much as possible.  A play that results in a block is given more
 *  weight than other moves.
 *  
 */
void lowestPossibleScore(Player player, uint8_t *retIndex, Position *retDirection) {

  uint8_t retValue = NOTHING;

  
  // Work out which bones are in play or in the computer's hand.  These are then 
  // used to work out whether a blocking play can be made ..
  
  populateBoneCounts(player);
  
  
  // Cycle through all bones in the hand ..
  
  for (uint8_t x = 0; x < players_hand_idx[static_cast<uint8_t>(player)]; x++) {

    uint8_t boneIdx = players_hand[static_cast<uint8_t>(player)][x]; 
    uint8_t leftPips = leftPips_ByBoneId(boneIdx);
    uint8_t rightPips = rightPips_ByBoneId(boneIdx);
    bool isDouble = (leftPips == rightPips);
    
    switch (boardMode) {
      
      case BOARD_MODE_NO_BONES_PLAYED:
        lowestPossibleScore_CenterOnly(x, leftPips, rightPips, &retValue, retIndex, retDirection);
        break;
          
      case BOARD_MODE_ONE_BONE_PLAYED:

        if (bone_c_pips_outer == leftPips)      { lowestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_c_pips_outer == rightPips)     { lowestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
        if (bone_c_pips_inner == leftPips)      { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_c_pips_inner == rightPips)     { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
          
        break;

      case BOARD_MODE_TWO_BONES_PLAYED:
         
        if (bone_c_pips_outer == leftPips)      { lowestPossibleScore_Center(x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_c_pips_outer == rightPips)     { lowestPossibleScore_Center(x, leftPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == leftPips)      { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == rightPips)     { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
          
        break;
        
      case BOARD_MODE_DOUBLE_BONE_PLAYED:
       
        if (bone_e != NOTHING) { 

          if (bone_e_pips_outer == leftPips)    { lowestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_e_pips_outer == rightPips)   { lowestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
         
        }
        else {

          if (bone_c_pips_outer == leftPips)    { lowestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { lowestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }

        }

        if (bone_w != NOTHING) { 

          if (bone_w_pips_outer == leftPips)    { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_w_pips_outer == rightPips)   { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
         
        }
        else {

          if (bone_c_pips_outer == leftPips)    { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }

        }
        
        if (bone_w == NOTHING && bone_e == NOTHING) { // Center is occupied but the east and the west are empty ..

          if (bone_c_pips_outer == leftPips)    { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }

        }          

        break;
    
      case BOARD_MODE_X_AXIS_PLAYED:
       
        if (bone_e_pips_outer == leftPips)      { lowestPossibleScore_East(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_e_pips_outer == rightPips)     { lowestPossibleScore_East(isDouble, x, leftPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == leftPips)      { lowestPossibleScore_West(isDouble, x, rightPips, &retValue, retIndex, retDirection); }
        if (bone_w_pips_outer == rightPips)     { lowestPossibleScore_West(isDouble, x, leftPips, &retValue, retIndex, retDirection); }

        if (bone_n != NOTHING) { // North ..
         
          if (bone_n_pips_outer == leftPips)    { lowestPossibleScore_North(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_n_pips_outer == rightPips)   { lowestPossibleScore_North(x, leftPips, &retValue, retIndex, retDirection); }
          
        }
        else {
         
          if (bone_c_pips_outer == leftPips)    { lowestPossibleScore_North(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { lowestPossibleScore_North(x, leftPips, &retValue, retIndex, retDirection); }
        
        }

        if (bone_s != NOTHING) { // South ..
          
          if (bone_s_pips_outer == leftPips)    { lowestPossibleScore_South(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_s_pips_outer == rightPips)   { lowestPossibleScore_South(x, leftPips, &retValue, retIndex, retDirection); }
          
        }
        else {
          
          if (bone_c_pips_outer == leftPips)    { lowestPossibleScore_North(x, rightPips, &retValue, retIndex, retDirection); }
          if (bone_c_pips_outer == rightPips)   { lowestPossibleScore_North(x, leftPips, &retValue, retIndex, retDirection); }
        
        }

        break;
     
    }

  }  

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (North).
 *  
 *  Can the nominated bone be played in the North position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_North(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {
    
  uint8_t val = canBlock(pips) ? 0 : (bone_w_pips_outer + bone_e_pips_outer + (bone_s != NOTHING ? bone_s_pips_outer : 0) + pips);

  if (val < *retValue) { 
    *retValue = val; 
    *retDirection = Position::North;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (South).
 *  
 *  Can the nominated bone be played in the South position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_South(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = canBlock(pips) ? 0 : (bone_w_pips_outer + bone_e_pips_outer + (bone_n != NOTHING ? bone_n_pips_outer : 0) + pips);

  if (val < *retValue) {  
    *retValue = val; 
    *retDirection = Position::South;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (Center).
 *  
 *  Can the nominated bone be played in the Center position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_Center(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = (canBlock(pips) ? 0 : bone_w_pips_outer + pips);

  if (val < *retValue) { 
    *retValue = val;
    *retDirection = Position::Center;
    *retIndex = boneIdx;
  }
  
}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (Center Only).
 *  
 *  Can the nominated bone be played in the Center position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_CenterOnly(uint8_t boneIdx, uint8_t leftPips, uint8_t rightPips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = leftPips + rightPips;

  if (val < *retValue) { 
    *retValue = val;
    *retDirection = Position::Center;
    *retIndex = boneIdx;
  }
  
}

/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (West).
 *  
 *  Can the nominated bone be played in the West position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_West(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  switch (boardMode) {
    
    case BOARD_MODE_TWO_BONES_PLAYED:
      {
        uint8_t val = (canBlock(pips) ? 0 : bone_c_pips_outer + pips);

        if (val < *retValue) { 
          *retValue = val;
          *retDirection = Position::West;
          *retIndex = boneIdx;
        } 
        
      }

      break;
  
    case BOARD_MODE_ONE_BONE_PLAYED:
      {
        uint8_t val = (canBlock(pips) ? 0 : bone_c_pips_outer + (pips * (isDouble ? 2 : 1)));

        if (val < *retValue) { 
          *retValue = val;
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }

      break;
     
    case BOARD_MODE_DOUBLE_BONE_PLAYED:
      {
        uint8_t val = canBlock(pips) ? 0 : ((bone_e != NOTHING ? bone_e_pips_outer : bone_c_pips_outer + bone_c_pips_inner) + pips);

        if (val < *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }

      break;

    case BOARD_MODE_X_AXIS_PLAYED:
      {
        uint8_t val = canBlock(pips) ? 0 : ((bone_n != NOTHING ? bone_n_pips_outer : 0) + (bone_s != NOTHING ? bone_s_pips_outer : 0) + bone_e_pips_outer + pips);

        if (val < *retValue) { 
          *retValue = val; 
          *retDirection = Position::West;
          *retIndex = boneIdx;
        }

      }
      
      break;
        
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (West, Center Only).
 *  
 *  Can the nominated bone be played in the West position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_West_CenterOnly(uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  uint8_t val = canBlock(pips) ? 0 : (bone_c_pips_inner + bone_c_pips_outer + pips);

  if (val < *retValue) { 
    *retValue = val; 
    *retDirection = Position::West;
    *retIndex = boneIdx;
  }

}


/* ----------------------------------------------------------------------------
 *  Calculate the lowest possible score (East).
 *  
 *  Can the nominated bone be played in the East position?  If so, could it 
 *  block the other player or drive the pip count down to a value that is lower 
 *  than all previously calculated ones ??  Moves that block the other player 
 *  are preferred to non-blocking moves.
 *    
 */
void lowestPossibleScore_East(boolean isDouble, uint8_t boneIdx, uint8_t pips, uint8_t *retValue, uint8_t *retIndex, Position *retDirection) {

  switch (boardMode) {
    
    case BOARD_MODE_ONE_BONE_PLAYED:
      {
        uint8_t val = (canBlock(pips) ? 0 : bone_c_pips_inner + (pips * (isDouble ? 2 : 1)));

        if (val < *retValue) {
          *retValue = val;
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }

      }          
      break;
      
    case BOARD_MODE_DOUBLE_BONE_PLAYED:
      {
        uint8_t val = canBlock(pips) ? 0 :((bone_w != NOTHING ? bone_w_pips_outer : bone_c_pips_outer + bone_c_pips_inner) + pips);

        if (val < *retValue) { 
          *retValue = val; 
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }

      }     
      break;

    case BOARD_MODE_X_AXIS_PLAYED:
      {
        uint8_t val = canBlock(pips) ? 0 : ((bone_n != NOTHING ? bone_n_pips_outer : 0) + (bone_s != NOTHING ? bone_s_pips_outer : 0) + bone_w_pips_outer + pips);

        if (val < *retValue) { 
          *retValue = val; 
          *retDirection = Position::East;
          *retIndex = boneIdx;
        }
      }
      break;
      
  }
  
}


/* ----------------------------------------------------------------------------
 *  Play a bone.
 *  
 *  Play the bone at 'index' in the hand of the 'player' in cardinal location 
 *  'position'.
 *  
 */
void playBone(GameMode gameMode, Player player, uint8_t index, Position position) {

  printHand();
  
  // Update the board's end bones ..
  
  switch (position) {
      
    case Position::Center:
    
      if (bone_c == NOTHING) {
        bone_c = players_hand[static_cast<uint8_t>(player)][index];
        bone_c_pips_inner = leftPips_ByBoneId(bone_c);
        bone_c_pips_outer = rightPips_ByBoneId(bone_c);
      }
      else {
        bones_played[bones_played_idx] = bone_c;
        bones_played_idx++;
        bone_c = players_hand[static_cast<uint8_t>(player)][index];
        bone_c_pips_inner = bone_c_pips_outer;
        bone_c_pips_outer = playBone_NewPipValue(bone_c_pips_inner, bone_c);
      }
      break;
        
    case Position::North:
    
      if (bone_n == NOTHING) {
        bone_n = players_hand[static_cast<uint8_t>(player)][index];
        bone_n_pips_inner = bone_c_pips_outer;
        bone_n_pips_outer = playBone_NewPipValue(bone_n_pips_inner, bone_n);
      }
      else {
        bones_played[bones_played_idx] = bone_n;
        bones_played_idx++;
        bone_n = players_hand[static_cast<uint8_t>(player)][index];
        bone_n_pips_inner = bone_n_pips_outer;
        bone_n_pips_outer = playBone_NewPipValue(bone_n_pips_inner, bone_n);
      }
      break;
      
    case Position::South:
    
      if (bone_s == NOTHING) {
        bone_s = players_hand[static_cast<uint8_t>(player)][index];
        bone_s_pips_inner = bone_c_pips_outer;
        bone_s_pips_outer = playBone_NewPipValue(bone_s_pips_inner, bone_s);
      }
      else {
        bones_played[bones_played_idx] = bone_s;
        bones_played_idx++;
        bone_s = players_hand[static_cast<uint8_t>(player)][index];
        bone_s_pips_inner = bone_s_pips_outer;
        bone_s_pips_outer = playBone_NewPipValue(bone_s_pips_inner, bone_s);
      }
      break;      

    case Position::East:
    
      if (bone_e == NOTHING) {
        bone_e = players_hand[static_cast<uint8_t>(player)][index];
        if (bone_c == NOTHING) {
          bone_e_pips_inner = bone_e_pips_inner;
          bone_e_pips_outer = playBone_NewPipValue(bone_e_pips_inner, bone_e);
        }
        else {
          bone_e_pips_inner = bone_c_pips_outer;
          bone_e_pips_outer = playBone_NewPipValue(bone_e_pips_inner, bone_e);
        }
      }
      else {        
        bones_played[bones_played_idx] = bone_e;
        bones_played_idx++;
        bone_e = players_hand[static_cast<uint8_t>(player)][index];
        bone_e_pips_inner = bone_e_pips_outer;
        bone_e_pips_outer = playBone_NewPipValue(bone_e_pips_inner, bone_e);
      }
      break;
      
    case Position::West:

      if (bone_w == NOTHING) {
        bone_w = players_hand[static_cast<uint8_t>(player)][index];
        if (bone_c == NOTHING) {
          bone_w_pips_inner = bone_w_pips_inner;
          bone_w_pips_outer = playBone_NewPipValue(bone_w_pips_inner, bone_w);
        }
        else {
          bone_w_pips_inner = bone_c_pips_inner;
          bone_w_pips_outer = playBone_NewPipValue(bone_w_pips_inner, bone_w);
        }
      }
      else {
        bones_played[bones_played_idx] = bone_w;
        bones_played_idx++;
        bone_w = players_hand[static_cast<uint8_t>(player)][index];
        bone_w_pips_inner = bone_w_pips_outer;
        bone_w_pips_outer = playBone_NewPipValue(bone_w_pips_inner, bone_w);

      }
      break;

    default: break;
                  
  }
 
  
  // Shuffle the bones along in the players hand to the left ..

  for (uint8_t x = index; x < players_hand_idx[static_cast<uint8_t>(player)] - 1; x++) {
            
      players_hand[player][x] = players_hand[static_cast<uint8_t>(player)][x + 1]; 
      
  }

  players_hand[static_cast<uint8_t>(player)][players_hand_idx[static_cast<uint8_t>(player)] - 1] = 0;
  players_hand_idx[static_cast<uint8_t>(player)]--;

  

  // Update screen ..

  switch (boardMode) {
  
    case BOARD_MODE_NO_BONES_PLAYED:
      boardMode = (hasADoubleBeenPlayed(BOARD_MODE_NO_BONES_PLAYED) ? BOARD_MODE_DOUBLE_BONE_PLAYED : BOARD_MODE_ONE_BONE_PLAYED);
      break;
  
    case BOARD_MODE_ONE_BONE_PLAYED:
      boardMode = (hasADoubleBeenPlayed(BOARD_MODE_ONE_BONE_PLAYED) ? BOARD_MODE_DOUBLE_BONE_PLAYED : BOARD_MODE_TWO_BONES_PLAYED);
      break;
  
    case BOARD_MODE_TWO_BONES_PLAYED:
      boardMode = (hasADoubleBeenPlayed(BOARD_MODE_TWO_BONES_PLAYED) ? BOARD_MODE_DOUBLE_BONE_PLAYED : BOARD_MODE_TWO_BONES_PLAYED);
      break;
      
    case BOARD_MODE_DOUBLE_BONE_PLAYED:  
      boardMode = (hasXAxisBeenPlayed() ? BOARD_MODE_X_AXIS_PLAYED : BOARD_MODE_DOUBLE_BONE_PLAYED);
      break;
      
    case BOARD_MODE_X_AXIS_PLAYED:  
      break;

  }

  renderBoard(gameMode, player, Position::Nothing, false);


  // Update score ..

  if (getPipCount() % 5 == 0 && getPipCount() != 0) {

    renderGraveyard(player, NOTHING, players_hand_highlight_idx, false);
    players_score[static_cast<uint8_t>(player)] = players_score[static_cast<uint8_t>(player)] + getPipCount();
    renderPlayersScore(gameMode, player);
    renderPlayersHand(player, players_hand_highlight_idx, false);
  
  }
  else {
    renderGraveyard(player, NOTHING, players_hand_highlight_idx, false);
  }

}


/* ----------------------------------------------------------------------------
 *  Can the nominated bone be played in the position?
 *  
 */
boolean canPlayBoard(uint8_t boneIdx, uint8_t position) {

  uint8_t leftPips = leftPips_ByBoneId( boneIdx );
  uint8_t rightPips = rightPips_ByBoneId( boneIdx );
  
  switch (boardMode) {
  
    case BOARD_MODE_NO_BONES_PLAYED:

      return true;

    case BOARD_MODE_ONE_BONE_PLAYED:

      if (position == Position::East) {
        
        if (bone_e == NOTHING && bone_c != NOTHING && ( bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips)) {
          return true;
        }
  
        if (bone_e != NOTHING && ( bone_e_pips_inner == leftPips || bone_e_pips_inner == rightPips)) {
          return true;
        }

      }
      
      if (position == Position::West) {

        if (bone_w == NOTHING && bone_c != NOTHING && ( bone_c_pips_inner == leftPips || bone_c_pips_inner == rightPips)) {
          return true;
        }

        if (bone_w != NOTHING && ( bone_w_pips_outer == leftPips || bone_w_pips_outer == rightPips)) {
          return true;
        }
            
      }
      
      // if (position == Position::Center) {
          
      //   if (bone_c_pips_outer == NOTHING || bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips) {
      //     return true;
      //   }
      
      // }
      
      break;
      
    case BOARD_MODE_TWO_BONES_PLAYED:

      if (position == Position::West && (bone_w_pips_outer == leftPips || bone_w_pips_outer == rightPips)) {
        return true;
      }
    
      if (position == Position::Center && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips)) {
        return true;
      }

      break;
      
    case BOARD_MODE_DOUBLE_BONE_PLAYED:
    
      if (position == Position::East) {
        
        if (bone_e_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips)) {
          return true;
        }
    
        if (bone_e_pips_outer != NOTHING && (bone_e_pips_outer == leftPips || bone_e_pips_outer == rightPips)) {
          return true;
        }

      }
   
      if (position == Position::West) {
    
        if (bone_w_pips_outer == NOTHING && (bone_c_pips_inner == leftPips || bone_c_pips_inner == rightPips)) {
          return true;
        }
    
        if (bone_w_pips_outer != NOTHING && (bone_w_pips_outer == leftPips || bone_w_pips_outer == rightPips)) {
          return true;
        }

      }
      
      break;

    case BOARD_MODE_X_AXIS_PLAYED:

      if (position == Position::North) {
      
        if (bone_n_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips)) {
          return true;
        }
    
        if (bone_n_pips_outer != NOTHING && (bone_n_pips_outer == leftPips || bone_n_pips_outer == rightPips)) {
          return true;
        }
      
      }

      if (position == Position::South) {
      
        if (bone_s_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips)) {
          return true;
        }
    
        if (bone_s_pips_outer != NOTHING && (bone_s_pips_outer == leftPips || bone_s_pips_outer == rightPips)) {
          return true;
        }

      }

      if (position == Position::East && (bone_e_pips_outer == leftPips || bone_e_pips_outer == rightPips)) {
        return true;
      }
    
      if (position == Position::West && (bone_w_pips_outer == leftPips || bone_w_pips_outer == rightPips)) {
        return true;
      }

      break;
      
  }

  return false;
  
}


#include "src/utils/Arduboy2Ext.h"


/* ----------------------------------------------------------------------------
 *  Render player's score.
 *  
 *  If ANIMATIONS is defined, flash the score three times.  Disable ANIMATIONS 
 *  to speed up testing in development mode.
 *  
 */
void renderPlayersScore(Player player, HandPosition handPosition) { 

  switch (handPosition) {
    
    case HandPosition::Top:

      renderPlayerScore(HandPosition::Top, player, false);
        
      for (uint8_t y = 0; y < 6; y++) {

        renderPlayerScore(HandPosition::Top, player, (y % 2 == 0));
        arduboy.delayShort(250);

      }
      break;

    case HandPosition::Bottom:

      renderPlayerScore(HandPosition::Bottom, player, false);
        
      for (uint8_t y = 0; y < 6; y++) {

        renderPlayerScore(HandPosition::Bottom, player, (y % 2 == 0));
        arduboy.delayShort(250);

      }
      break;

  }

}


/* ----------------------------------------------------------------------------
 *  Render player's score.
 *  
 *  If ANIMATIONS is defined, flash the score three times.  Disable ANIMATIONS 
 *  to speed up testing in development mode.
 *  
 */
void renderPlayersScore(GameMode gameMode, Player player) { 

  Player altPlayer = getAltPlayer(gameMode, player);

  if (player == Player::Computer) {

    renderPlayerScore(HandPosition::Top, player, false);
    renderPlayerScore(HandPosition::Bottom, altPlayer, false);
      
    for (uint8_t y = 0; y < 6; y++) {

      if (player == Player::Computer) {
        renderPlayerScore(HandPosition::Top, player, (y % 2 == 0));
      }
      else {
        renderPlayerScore(HandPosition::Bottom, altPlayer, (y % 2 == 0));
      }

      arduboy.delayShort(250);

    }

  }
  else {

    renderPlayerScore(HandPosition::Top, altPlayer, false);
    renderPlayerScore(HandPosition::Bottom, player, false);
      
    for (uint8_t y = 0; y < 6; y++) {

//      if (player == Player::Computer) {
//        renderPlayerScore(HandPosition::Top, altPlayer, (y % 2 == 0));
//      }
//      else {
        renderPlayerScore(HandPosition::Bottom, player, (y % 2 == 0));
//      }

      arduboy.delayShort(250);

    }

  }

}

/* ----------------------------------------------------------------------------
 *  Render a single player's score.
 *  
 */
void renderPlayerScore(HandPosition position, Player player, bool highlight) {

  uint8_t player_score = players_score[static_cast<uint8_t>(player)];
  uint8_t player_hand_bones = players_hand_idx[player];
  
  if (position == HandPosition::Top) {

    if (!highlight) {
      
      arduboy.fillRect(SCORE_COMPUTER_LEFT, SCORE_COMPUTER_TOP, SCORE_COMPUTER_WIDTH, SCORE_COMPUTER_HEIGHT + 8, BLACK);
      Sprites::drawOverwrite(SCORE_COMPUTER_LEFT + 1, SCORE_COMPUTER_TOP, Images::digits, player_score / 100);
      player_score = player_score - (player_score / 100) * 100;
      Sprites::drawOverwrite(SCORE_COMPUTER_LEFT + 6, SCORE_COMPUTER_TOP, Images::digits, player_score / 10);
      Sprites::drawOverwrite(SCORE_COMPUTER_LEFT + 11, SCORE_COMPUTER_TOP, Images::digits, player_score % 10);
      
      Sprites::drawOverwrite(SCORE_COMPUTER_LEFT + 1, SCORE_COMPUTER_TOP + 8, Images::digits, player_hand_bones / 10);
      Sprites::drawOverwrite(SCORE_COMPUTER_LEFT + 6, SCORE_COMPUTER_TOP + 8, Images::digits, player_hand_bones % 10);
      
    }
    else {
      
#ifdef SOUNDS
  
      sound.tone(NOTE_C3,50, NOTE_D3,50, NOTE_E3,50);

#endif 
      
      arduboy.fillRect(SCORE_COMPUTER_LEFT, SCORE_COMPUTER_TOP, SCORE_COMPUTER_WIDTH, SCORE_COMPUTER_HEIGHT, WHITE);
      Sprites::drawErase(SCORE_COMPUTER_LEFT + 1, SCORE_COMPUTER_TOP, Images::digits, player_score / 100);
      player_score = player_score - (player_score / 100) * 100;
      Sprites::drawErase(SCORE_COMPUTER_LEFT + 6, SCORE_COMPUTER_TOP, Images::digits, player_score / 10);
      Sprites::drawErase(SCORE_COMPUTER_LEFT + 11, SCORE_COMPUTER_TOP, Images::digits, player_score % 10);
      
    }
    
  }
 
  if (position == HandPosition::Bottom) {

    if (!highlight) {
      arduboy.fillRect(SCORE_PLAYER_LEFT, SCORE_PLAYER_TOP, SCORE_PLAYER_WIDTH, SCORE_PLAYER_HEIGHT, BLACK);
      Sprites::drawOverwrite(SCORE_PLAYER_LEFT + 1, SCORE_PLAYER_TOP, Images::digits, player_score / 100);
      player_score = player_score - (player_score / 100) * 100;
      Sprites::drawOverwrite(SCORE_PLAYER_LEFT + 6, SCORE_PLAYER_TOP, Images::digits, player_score / 10);
      Sprites::drawOverwrite(SCORE_PLAYER_LEFT + 11, SCORE_PLAYER_TOP, Images::digits, player_score % 10); 
      
    }
    else {
      
#ifdef SOUNDS
  
      sound.tone(NOTE_C4,30, NOTE_D4,30, NOTE_E4,30);

#endif 

      arduboy.fillRect(SCORE_PLAYER_LEFT, SCORE_PLAYER_TOP, SCORE_PLAYER_WIDTH, SCORE_PLAYER_HEIGHT, WHITE);
      Sprites::drawErase(SCORE_PLAYER_LEFT + 1, SCORE_PLAYER_TOP, Images::digits, player_score / 100);
      player_score = player_score - (player_score / 100) * 100;
      Sprites::drawErase(SCORE_PLAYER_LEFT + 6, SCORE_PLAYER_TOP, Images::digits, player_score / 10);
      Sprites::drawErase(SCORE_PLAYER_LEFT + 11, SCORE_PLAYER_TOP, Images::digits, player_score % 10);

    }
    
  }

  arduboy.display();

}


/* ----------------------------------------------------------------------------
 *  Render the player's hand.
 *  
 *  Only six bones can be rendered across the screen however the players hand
 *  may contain many more.  Render the bones from index 'players_hand_visible_idx'
 *  to 'players_hand_visible_idx' plus six.
 *  
 */
void renderPlayersHand(Player player, uint8_t highlight, bool hideBones) { 

  if (player == Player::Computer) player = getAltPlayer(gameMode, player);

  uint8_t x = 0;

  arduboy.fillRect(PLAYER_BOTTOM_LEFT_ARROW_X, PLAYER_BOTTOM_HAND_Y, WIDTH, HEIGHT, BLACK);
 
  for (uint8_t y = players_hand_visible_idx; y < players_hand_idx[player]; y++) {

    if (!hideBones) {
      drawBone_Rotated(bones_ref[players_hand[static_cast<uint8_t>(player)][y]], PLAYER_BOTTOM_HAND_X + (x * BONES_SMALL_X_SPACING_LANDSCAPE), PLAYER_BOTTOM_HAND_Y, y == highlight);
    }
    else {
      Sprites::drawOverwrite(PLAYER_BOTTOM_HAND_X + (x * BONES_SMALL_X_SPACING_LANDSCAPE), PLAYER_BOTTOM_HAND_Y, Images::bone_landscape_hidden, 0);
    }

    x++;
    if (y - players_hand_visible_idx + 1 >= PLAYER_BOTTOM_VISIBLE) break;

  }

  Sprites::drawOverwrite(PLAYER_BOTTOM_LEFT_ARROW_X, PLAYER_BOTTOM_LEFT_ARROW_Y, Images::leftArrow, (players_hand_visible_idx != 0 ? 1 : 0));
  Sprites::drawOverwrite(PLAYER_BOTTOM_RIGHT_ARROW_X, PLAYER_BOTTOM_RIGHT_ARROW_Y, Images::rightArrow, (players_hand_idx[1] - players_hand_visible_idx) > PLAYER_HAND_MAX_VISIBLE ? 1 : 0);
  arduboy.display();

}


/* ----------------------------------------------------------------------------
 *  Render the graveyard.
 *  
 *  The graveyard includes available bones and those already played.  As the 
 *  bones are 17 pixels high, they overwrite the player's hand and this must
 *  be redrawn.
 *  
 *  The graveyard can only 'accommodate' 21 bones, as defined by the constants
 *  BONES_GRAVEYARD_BONES_PER_ROW and BONES_GRAVEYARD_BONES_ROWS.  Therefore
 *  rendering stops after the third row.  This gives the computer an advantage
 *  when getting towards the end of a long game as he can see every bone that
 *  has been played whereas the human may not be able to.  Tough luck!
 *  
 */
void renderGraveyard(Player currentPlayer, uint8_t highlight, uint8_t playersHandHighlight, bool hideBones) { 

  arduboy.fillRect(BONES_GRAVEYARD_X, BONES_GRAVEYARD_Y, BONES_GRAVEYARD_X_MAX, BONES_GRAVEYARD_Y_MAX, BLACK); 

  uint8_t x = BONES_GRAVEYARD_X;
  uint8_t y = BONES_GRAVEYARD_Y;
  uint8_t rows = 1;
  
  
  // Draw the available bones ..
  
  for (uint8_t z = 0; z < bones_available_idx; z++) {
    
    drawBone_Hidden(x, y, z == highlight);
    x = x + BONES_SMALL_X_SPACING_PORTRAIT;
    
    if (x > (WIDTH - BONES_SMALL_X_SPACING_PORTRAIT)) { 
      x = BONES_GRAVEYARD_X; 
      y = y + BONES_SMALL_Y_SPACING_PORTRAIT;
      rows++;
    }
    
  }

  
  // Draw the played bones ..

  for (uint8_t z = 0; z < bones_played_idx; z++) {
    
    drawBone_Small(bones_ref[bones_played[z]], x, y, false);
    x = x + BONES_SMALL_X_SPACING_PORTRAIT;
    
    if (x > (WIDTH - BONES_SMALL_X_SPACING_PORTRAIT)) { 
      x = BONES_GRAVEYARD_X; 
      y = y + BONES_SMALL_Y_SPACING_PORTRAIT;
      rows++;
    }
    
    
    // Do not render a fourth row as there is not enough real-estate ..
    
    if (rows > BONES_GRAVEYARD_BONES_ROWS) { break; }
    
  }

  renderPlayersHand(currentPlayer, playersHandHighlight, hideBones);
  arduboy.display();    
  
}


/* ----------------------------------------------------------------------------
 *  Render the board.
 *  
 *  Render the playing board including played bones, available positions where
 *  bones can be played and the player's scores.  As the rendering of the board
 *  overwrites the player's hand this is also re-rendered.
 *  
 */
void renderBoard(GameMode gameMode, Player player, Position highlight, bool hideBones) {


  // Clear background ..
  
  arduboy.fillRect(0, 0, 55, 50, BLACK);
  

  // Render players scores ..
  
  if (player == Player::Computer) {
    renderPlayerScore(HandPosition::Top, player, false);
    renderPlayerScore(HandPosition::Bottom, getAltPlayer(gameMode, player), false);
  }
  else {
    renderPlayerScore(HandPosition::Top, getAltPlayer(gameMode, player), false);
    renderPlayerScore(HandPosition::Bottom, player, false);
  }


  // Render board ..
 
  if (boardMode == BOARD_MODE_NO_BONES_PLAYED) {

    drawBone_Outline_Rotated(BONE_C_POS_X_NO_BONES_PLAYED, BONE_C_POS_Y_NO_BONES_PLAYED, highlight == Position::Center);

  } else if (boardMode == BOARD_MODE_TWO_BONES_PLAYED) {

    drawBone_Rotated(bone_w_pips_outer, bone_w_pips_inner, BONE_W_POS_X_TWO_BONES_PLAYED, BONE_W_POS_Y_TWO_BONES_PLAYED, highlight == Position::West);
    drawBone_Rotated(bone_c_pips_inner, bone_c_pips_outer, BONE_C_POS_X_TWO_BONES_PLAYED, BONE_C_POS_Y_TWO_BONES_PLAYED, highlight == Position::Center);

  } else {

    if (boardMode == BOARD_MODE_X_AXIS_PLAYED){
      
      if (bone_n == NOTHING)    drawBone_Outline(BONE_N_POS_X_AXIS_PLAYED, BONE_N_POS_Y_AXIS_PLAYED, highlight == Position::North);
      if (bone_n != NOTHING)    {
        if (isDouble(bone_n))   {
          drawBone_Rotated(bone_n_pips_outer, bone_n_pips_inner, BONE_N_POS_X_DOUBLE_BONE_PLAYED - 4, BONE_N_POS_Y_DOUBLE_BONE_PLAYED + 1, highlight == Position::North);
          Sprites::drawSelfMasked(BONE_N_POS_X_DOUBLE_BONE_PLAYED, BONE_N_POS_Y_DOUBLE_BONE_PLAYED + 11, Images::Half_Top, 0);
        }
        else {
          drawBone_Small(bone_n_pips_outer, bone_n_pips_inner, BONE_N_POS_X_DOUBLE_BONE_PLAYED, BONE_N_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::North);
        }
      }        
    }
    
    if (bone_w == NOTHING)      drawBone_Outline_Rotated(BONE_W_POS_X_DOUBLE_BONE_PLAYED, BONE_W_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::West);
    if (bone_w != NOTHING)      {
      if (isDouble(bone_w))     {
        drawBone_Small(bone_w_pips_outer, bone_w_pips_inner, BONE_W_POS_X_DOUBLE_BONE_PLAYED, BONE_W_POS_Y_DOUBLE_BONE_PLAYED - 4, highlight == Position::West);
        Sprites::drawSelfMasked(BONE_W_POS_X_DOUBLE_BONE_PLAYED + 10, BONE_W_POS_Y_DOUBLE_BONE_PLAYED, Images::Half_Left, 0);
      }
      else {
        drawBone_Rotated(bone_w_pips_outer, bone_w_pips_inner, BONE_W_POS_X_DOUBLE_BONE_PLAYED, BONE_W_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::West);
      }
    }
    
    drawBone_Rotated(bone_c_pips_inner, bone_c_pips_outer, BONE_C_POS_X_DOUBLE_BONE_PLAYED, BONE_C_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::Center);
    
    if (bone_e == NOTHING)      drawBone_Outline_Rotated(BONE_E_POS_X_DOUBLE_BONE_PLAYED, BONE_E_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::East);
    if (bone_e != NOTHING)      {
      if (isDouble(bone_e))     {
        drawBone_Small(bone_e_pips_inner, bone_e_pips_outer, BONE_E_POS_X_DOUBLE_BONE_PLAYED + 7, BONE_E_POS_Y_DOUBLE_BONE_PLAYED - 4, highlight == Position::East);
        Sprites::drawSelfMasked(BONE_E_POS_X_DOUBLE_BONE_PLAYED, BONE_E_POS_Y_DOUBLE_BONE_PLAYED, Images::Half_Right, 0);
      }
      else {
        drawBone_Rotated(bone_e_pips_inner, bone_e_pips_outer, BONE_E_POS_X_DOUBLE_BONE_PLAYED, BONE_E_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::East);
      }
    }
            
    if (boardMode == BOARD_MODE_X_AXIS_PLAYED){
      
      if (bone_s == NOTHING)    drawBone_Outline(BONE_S_POS_X_AXIS_PLAYED, BONE_S_POS_Y_AXIS_PLAYED, highlight == Position::South);
      if (bone_s != NOTHING)    {
        if (isDouble(bone_s))   {
          drawBone_Rotated(bone_s_pips_inner, bone_s_pips_outer, BONE_S_POS_X_DOUBLE_BONE_PLAYED - 4, BONE_S_POS_Y_DOUBLE_BONE_PLAYED + 7, highlight == Position::South);
          Sprites::drawSelfMasked(BONE_S_POS_X_DOUBLE_BONE_PLAYED, BONE_S_POS_Y_DOUBLE_BONE_PLAYED, Images::Half_Bottom, 0);
        }
        else {          
          drawBone_Small(bone_s_pips_inner, bone_s_pips_outer, BONE_S_POS_X_DOUBLE_BONE_PLAYED, BONE_S_POS_Y_DOUBLE_BONE_PLAYED, highlight == Position::South);
        }
      }

    }
      
  }

  renderPlayersHand(player, players_hand_highlight_idx, hideBones);
  arduboy.display();
  
}

#include "src/utils/Arduboy2Ext.h"


/* ----------------------------------------------------------------------------
 *  Draw a bone at location x, y.  
 *  
 *  This function accepts a bone value (left and righ pip value in a single hex
 *  number) and renders it without regard to orientation.  This is used when
 *  rendering the played bones in the graveyard. 
 *  
 */
void drawBone_Small(uint8_t boneValue, uint8_t x, uint8_t y, bool highlighted) {
  
  drawBone_Small(leftPips_ByBoneValue(boneValue), rightPips_ByBoneValue(boneValue), x, y, highlighted);
  
}


/* ----------------------------------------------------------------------------
 *  Draw a bone at location x, y.  
 *  
 *  This function accepts two parameters - left / top pips and right / bottom
 *  pips and renders a bone in this orientation.  This is used when rendering
 *  the board where orientation matters. 
 *  
 */
void drawBone_Small(uint8_t pipsTop, uint8_t pipsBottom, uint8_t x, uint8_t y, bool highlighted) {

  if (highlighted) {
    
    Sprites::drawSelfMasked(x, y, Images::bone_portrait_highlighted, 0);
    Sprites::drawErase(x + 2, y + 2, Images::bone_pips, pipsTop);
    Sprites::drawErase(x + 2, y + 10, Images::bone_pips, pipsBottom);
    
  }
  else {

    Sprites::drawSelfMasked(x, y, Images::bone_portrait, 0);
    Sprites::drawOverwrite(x + 2, y + 2, Images::bone_pips_highlight, pipsTop);
    Sprites::drawOverwrite(x + 2, y + 10, Images::bone_pips_highlight, pipsBottom);
  
  }
  
}


/* ----------------------------------------------------------------------------
 *  Draw a bone at location x, y in the landscape orientation.  
 *  
 *  This function accepts a bone value (left and righ pip value in a single hex
 *  number) and renders it without regard to orientation.  This is used when
 *  rendering the played bones in the graveyard. 
 *  
 */
void drawBone_Rotated(uint8_t boneValue, uint8_t x, uint8_t y, bool highlighted) {
  
  drawBone_Rotated(leftPips_ByBoneValue(boneValue), rightPips_ByBoneValue(boneValue), x, y, highlighted);
  
}


/* ----------------------------------------------------------------------------
 *  Draw a bone at location x, y in the landscape orientation.  
 *  
 *  This function accepts a bone value (left and righ pip value in a single hex
 *  number) and renders it without regard to orientation.  This is used when
 *  rendering the played bones in the graveyard. 
 *  
 */
void drawBone_Rotated(uint8_t pipsLeft, uint8_t pipsRight, uint8_t x, uint8_t y, bool highlighted) {

  if (highlighted) {
    
    Sprites::drawSelfMasked(x, y, Images::bone_landscape_highlighted, 0);
    Sprites::drawErase(x + 2, y + 2, Images::bone_pips, pipsLeft);
    Sprites::drawErase(x + 10, y + 2, Images::bone_pips, pipsRight);
    
  }
  else {

    Sprites::drawSelfMasked(x, y, Images::bone_landscape, 0);
    Sprites::drawOverwrite(x + 2, y + 2, Images::bone_pips_highlight, pipsLeft);
    Sprites::drawOverwrite(x + 10, y + 2, Images::bone_pips_highlight, pipsRight);
  
  }

}


void drawBone_Rotated_Erase(uint8_t boneValue, uint8_t x, uint8_t y) {

  Sprites::drawErase(x, y, Images::bone_landscape, 0);
  Sprites::drawErase(x + 2, y + 2, Images::bone_pips_highlight, leftPips_ByBoneValue(boneValue));
  Sprites::drawErase(x + 10, y + 2, Images::bone_pips_highlight, rightPips_ByBoneValue(boneValue));

}

/* ----------------------------------------------------------------------------
 *  Draw a bone outline only at location X, Y.
 *  
 */
void drawBone_Outline(uint8_t x, uint8_t y, bool highlighted) {

  if (highlighted) {
    
    Sprites::drawOverwrite(x, y, Images::bone_portrait_outline_highlighted, 0);
    
  }
  else {
    
    Sprites::drawOverwrite(x, y, Images::bone_portrait_outline, 0);
    
  }

}


/* ----------------------------------------------------------------------------
 *  Draw a bone outline only at location X, Y in the landscape orientation. 
 *  
 */
void drawBone_Outline_Rotated(uint8_t x, uint8_t y, bool highlighted) {

  if (highlighted) {
    
    Sprites::drawOverwrite(x, y, Images::bone_landscape_outline_highlighted, 0);
    
  }
  else {
    
    Sprites::drawOverwrite(x, y, Images::bone_landscape_outline, 0);
    
  }

}


/* ----------------------------------------------------------------------------
 *  Draw a hidden bone at location X, Y in the landscape orientation. 
 *  
 */
void drawBone_Hidden(uint8_t x, uint8_t y, bool highlighted) {

  if (highlighted) {

    Sprites::drawOverwrite(x, y, Images::bone_portrait_hidden_highlight, 0);
  
  }
  else {
    
    Sprites::drawOverwrite(x, y, Images::bone_portrait_hidden, 0);
  
  }

}

#include "src/utils/Arduboy2Ext.h"
#include "src/images/Images.h"
#include "src/utils/Enums.h"

static constexpr uint8_t ppotFrames = (1.6 *60); // 1.6s animation loop


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void drawSplash_Loop() {

  uint8_t counter = 0;

  while (gameState == GameState::SplashScreen) {

    if (!(arduboy.nextFrame())) { delay(5); };
    arduboy.pollButtons();
      
    auto justPressed = arduboy.justPressedButtons();
      

    if (justPressed > 0 && counter > 0) {

      counter = 20;

    }

    if (justPressed > 0 && counter == 0) {

  #ifdef SOUNDS    
      sound.tone(NOTE_A2, 30);
      sound.tone(NOTE_F1, 30);
  #endif

      counter = 1;

    }

    if (counter > 0) {

      if (arduboy.everyXFrames(2)) {
        counter++;
        if (counter == 21) gameState = GameState::TitleScreen; 
      }

    }


// ----------------------------------------------------------------------------

    Sprites::drawOverwrite(32, 16, Images::PPOT, 0);

    uint8_t p = 0x11; // Default pixel position 0 is hidden in the image

    switch (arduboy.getFrameCount(ppotFrames)) {

      // 2nd quarter of frames
      case (ppotFrames*1/4) ... (ppotFrames*2/4)-1:
        p = 0x31; // Move pixel down to position 1
        /*-fallthrough*/

      // 1st quarter of frames
      case 0 ... (ppotFrames*1/4)-1:
        // Flash 'Play' arrow by clearing the image
        // Overwrite directly to the screen buffer 
        arduboy.sBuffer[91 +(3*WIDTH)] = 0x00;
        arduboy.sBuffer[92 +(3*WIDTH)] = 0x00;
        arduboy.sBuffer[93 +(3*WIDTH)] = 0x00;
        arduboy.sBuffer[94 +(3*WIDTH)] = 0x00;
        arduboy.sBuffer[95 +(3*WIDTH)] = 0x00;
        break;

      // 3rd quarter of frames
      case (ppotFrames*2/4) ... (ppotFrames*3/4)-1:
        p = 0x51; // Move pixel down to position 2
        break;

      // 4th quarter of frames
      default:
        p = 0x91; // Move pixel down to position 3
        break;

    }

    // Draw pixel to represent the tape spooling
    // Render directly to the screen buffer 
    arduboy.sBuffer[52 +(3*WIDTH)] = p; // Values 0x11, 0x31, 0x51, 0x91

    // On even steps of pixel movement, update the spindle image
    if ((p & 0xA0) == 0) {
      // Render directly to the screen buffer 
      arduboy.sBuffer[45 +(3*WIDTH)] = 0xA5;
      arduboy.sBuffer[46 +(3*WIDTH)] = 0x95;
    }

    arduboy.display(true);

  }
}
#include "src/utils/Arduboy2Ext.h"
#include "src/utils/Enums.h"

#define TITLE_DELAY_BEFORE_PROMPT      20
#define TITLE_PORTRAIT_BONE_X          1
#define TITLE_PORTRAIT_BONE_Y          1
#define TITLE_LANDSCAPE_BONE_X         18
#define TITLE_LANDSCAPE_BONE_Y         36
#define TITLE_TITLE_X                  19
#define TITLE_TITLE_Y                  2
#define TITLE_PRESSA_X                 49
#define TITLE_PRESSA_Y                 52

#define TITLE_UPPER_LINE_X             70
#define TITLE_UPPER_LINE_X_MAX         127
#define TITLE_UPPER_LINE_Y             8

#define TITLE_LOWER_LINE_X             TITLE_UPPER_LINE_X
#define TITLE_LOWER_LINE_X_MAX         TITLE_UPPER_LINE_X_MAX
#define TITLE_LOWER_LINE_Y             28

#define TITLE_HEADLINE_X               75
#define TITLE_HEADLINE_Y               15
#define TITLE_PROMPT_X                 79
#define TITLE_PROMPT_Y                 45
#define TITLE_PROMPT_CIRCLE_X          117
#define TITLE_PROMPT_CIRCLE_Y          49
#define TITLE_PROMPT_CIRCLE_RAD        5


/* ----------------------------------------------------------------------------
 *  Handle the title screen.
 */
void drawTitle_Loop() {

  // uint8_t restart = 0;
  uint8_t delayA = 0;
  uint8_t topArrowOffset = 0;
  uint8_t bottomArrowOffset = 0;
  uint8_t arrowOffsetCount = 0;


  // Reset the scores ..
  memset(players_score, 0, (sizeof(players_score) / sizeof(players_score[0])));

  while (gameState == GameState::TitleScreen) {

    if (!(arduboy.nextFrame())) { delay(20); };
    arduboy.pollButtons();

    uint8_t justPressed = arduboy.justPressedButtons();


    // Clear the screen and render the splash ..
    
    arduboy.clear();
    Sprites::drawSelfMasked(TITLE_TITLE_X, TITLE_TITLE_Y, Images::Title, 0);

    switch (titleMode) {

      case TitleMode::PressA:

        if (delayA >= 55) {
          Sprites::drawSelfMasked(35, 42, Images::Bullet, 0);
          Sprites::drawSelfMasked(90, 42, Images::Bullet, 0);
        }

        if (delayA >= 110) {
          Sprites::drawSelfMasked(27, 42, Images::Bullet, 0);
          Sprites::drawSelfMasked(98, 42, Images::Bullet, 0);
        }

        if (delayA >= 165) {
          Sprites::drawSelfMasked(19, 42, Images::Bullet, 0);
          Sprites::drawSelfMasked(106, 42, Images::Bullet, 0);
        }

        if (delayA == 175) {
          Sprites::drawSelfMasked(TITLE_PRESSA_X, TITLE_PRESSA_Y, Images::PressA, 0);
        }
        else {
          delayA++;
        }

        if (justPressed & A_BUTTON) { titleMode = TitleMode::EnterNumberOfPlayers; }
        if (justPressed & B_BUTTON) { gameState = GameState::HighScore; }

        break;

      case TitleMode::EnterNumberOfPlayers:

        font4x6.setCursor(16,52);
        font4x6.print(F("Number~of~players~?"));

        switch (gameMode) {

          case GameMode::SinglePlayer:
            font4x6.setCursor(108, 52);
            font4x6.print(F("1"));
            break;

          case GameMode::TwoPlayer:
            font4x6.setCursor(108, 52);
            font4x6.print(F("2"));
            break;

        }

        Sprites::drawSelfMasked(107, 49 - topArrowOffset, Images::ArrowUp, 0);
        Sprites::drawSelfMasked(107, 60 + bottomArrowOffset, Images::ArrowDown, 0);

        if (justPressed & A_BUTTON) { gameState = GameState::GetNames; }
        if (justPressed & B_BUTTON) { titleMode = TitleMode::PressA; }
        if ((justPressed & UP_BUTTON) && gameMode == GameMode::SinglePlayer)   { gameMode = GameMode::TwoPlayer; topArrowOffset = 2; arrowOffsetCount = 10; }
        if ((justPressed & DOWN_BUTTON) && gameMode == GameMode::TwoPlayer)    { gameMode = GameMode::SinglePlayer; bottomArrowOffset = 2; arrowOffsetCount = 10; arduboy.clearButtonState(); }

        break;

    }


    arduboy.display();

    if (arrowOffsetCount > 0) {
      arrowOffsetCount--;
      if (arrowOffsetCount == 0) {
        topArrowOffset = 0;
        bottomArrowOffset = 0;
      }
    }


    // Restart ?

    // if (arduboy.pressed(DOWN_BUTTON)) {

    //   if (restart < UPLOAD_DELAY) {
    //     restart++;
    //   }
    //   else {
    //     arduboy.exitToBootloader();
    //   }

    // }
    // else {
    //   restart = 0;
    // }

  }
  
}

#include "src/utils/Arduboy2Ext.h"

Player getFirstPlayer(GameMode gameMode) {

  if (gameMode == GameMode::SinglePlayer) {

    return Player::Computer;

  }
  else {

    return Player::Human_1;

  }

}


Player getSecondPlayer(GameMode gameMode) {

  if (gameMode == GameMode::SinglePlayer) {

    return Player::Human_1;

  }
  else {

    return Player::Human_2;

  }

}

Player getAltPlayer(GameMode gameMode, Player player) {

  if (gameMode == GameMode::SinglePlayer) {

    if (player == Player::Computer) {
      return Player::Human_1;
    }
    else {
      return Player::Computer;
    }

  }
  else {

    if (player == Player::Human_2) {
      return Player::Human_1;
    }
    else {
      return Player::Human_2;
    }

  }

}


/* ----------------------------------------------------------------------------
 *  Shuffle the bones ..
 *  
 */
void shuffleBones() {

  
  // Clear the bones_played array and populate the bones_available array with 
  // a sequential number between 0 and BONES_COUNT (26).

  for (uint8_t x = 0; x < BONES_COUNT; x++) {
      
    bones_available[x] = x;
    bones_played[x] = 0;
            
  }
    
  
  // Randomly switch bones in the array with each other ..
  
  for (uint8_t x = BONES_COUNT - 1; x > 0; x--) {
      
    uint8_t y = random(x + 1);

    uint8_t z = bones_available[y];
    bones_available[y] = bones_available[x];
    bones_available[x] = z;

  }
  
  bones_available_idx = BONES_COUNT - 1;
  bones_played_idx = 0;
   
}


/* ----------------------------------------------------------------------------
 *  Deal the bones to the players ..
 *  
 */
void dealBones(GameMode gameMode) {
  
  Player player1 = getFirstPlayer(gameMode);
  Player player2 = getSecondPlayer(gameMode);
  
  memset(players_hand[0], 0, (sizeof(players_hand[0]) / sizeof(players_hand[0][0])));
  memset(players_hand_idx, 0, (sizeof(players_hand_idx) / sizeof(players_hand_idx[0])));
  
  for (uint8_t x = static_cast<uint8_t>(player1); x <= static_cast<uint8_t>(player2); x++) {

      for (uint8_t y = 0; y < BONES_INITIAL_COUNT; y++) {
      
      players_hand[x][y] = bones_available[bones_available_idx];
      
      bones_available_idx--;
      players_hand_idx[x]++;
    
    }
            
  }
    
}


/* ----------------------------------------------------------------------------
 *  Get the pip count of the board ..
 *  
 */
uint8_t getPipCount() {

  if (bone_c == NOTHING && bone_n == NOTHING && bone_s == NOTHING && bone_e == NOTHING && bone_w == NOTHING) {
      
    return NOTHING;
            
  }

  if (bone_c != NOTHING && bone_n == NOTHING && bone_s == NOTHING && bone_e == NOTHING && bone_w == NOTHING) {
      
    return bone_c_pips_inner + bone_c_pips_outer;
            
  }

  if (bone_e != NOTHING && bone_w != NOTHING) {
    
    uint8_t wPips = getPipCount(bone_w_pips_outer, bone_w);
    uint8_t ePips = getPipCount(bone_e_pips_outer, bone_e);
    uint8_t nPips = 0;
    uint8_t sPips = 0;

    if (bone_n != NOTHING) nPips = getPipCount(bone_n_pips_outer, bone_n);
    if (bone_s != NOTHING) sPips = getPipCount(bone_s_pips_outer, bone_s);
    
    return wPips + ePips + nPips + sPips;
          
  }
  else {

    uint8_t wPips = 0;
    uint8_t ePips = 0;

    if (bone_w != NOTHING) wPips = getPipCount(bone_w_pips_outer, bone_w);
    if (bone_e != NOTHING) ePips = getPipCount(bone_e_pips_outer, bone_e);
   
    return (bone_c_pips_inner == bone_c_pips_outer ? bone_c_pips_inner + bone_c_pips_outer : bone_c_pips_outer) + wPips + ePips;
          
  }
  
}

uint8_t getPipCount(uint8_t outerPips, uint8_t boneId) {

  return outerPips + (leftPips_ByBoneId(boneId) == rightPips_ByBoneId(boneId) ? outerPips : 0);

}

bool isDouble(uint8_t boneId) {

  return (leftPips_ByBoneId(boneId) == rightPips_ByBoneId(boneId));
  
}

/* ----------------------------------------------------------------------------
 *  Get the left-hand pip count of the bone by value.
 *  
 *  Bone values are a literal representation of the bone represented as a two-digit
 *  number such as 13 where the 1 is the right hand pip value and 3 is the left 
 *  hand pip value.
 *  
 */
uint8_t leftPips_ByBoneValue(uint8_t boneValue) {

  return boneValue >> 4; 
      
}


/* ----------------------------------------------------------------------------
 *  Get the right-hand pip count of the bone by value.
 *  
 *  Bone values are a literal representation of the bone represented as a two-digit
 *  number such as 13 where the 1 is the right hand pip value and 3 is the left 
 *  hand pip value.
 *  
 */
uint8_t rightPips_ByBoneValue(uint8_t boneValue) {

  return boneValue & 0x0F; 
      
}


/* ----------------------------------------------------------------------------
 *  Get the left-hand pip count of the bone by id.
 *  
 *  The bone value (left and right hand pip values as a two-digit value) must 
 *  be looked up via the bones-ref array.
 *  
 */
uint8_t leftPips_ByBoneId(uint8_t boneId) {

  return bones_ref[boneId] >> 4; 
      
}


/* ----------------------------------------------------------------------------
 *  Get the right-hand pip count of the bone by id.
 *  
 *  The bone value (left and right hand pip values as a two-digit value) must 
 *  be looked up via the bones-ref array.
 *  
 */
uint8_t rightPips_ByBoneId(uint8_t boneId) {

  return bones_ref[boneId] & 0x0F; 
      
}


/* ----------------------------------------------------------------------------
 *  Calculate the board's pip value if the nominated bone is played.
 *  
 */
uint8_t playBone_NewPipValue(uint8_t existingBonePips, uint8_t newBoneId) {

  if (existingBonePips == rightPips_ByBoneId(newBoneId)) {
  
    return leftPips_ByBoneId(newBoneId);
    
  }
  else {
  
    return rightPips_ByBoneId(newBoneId);
    
  }
     
} 


/* ----------------------------------------------------------------------------
 *  Have all 3 bones on the X-axis of play been played?
 *  
 */
boolean hasXAxisBeenPlayed() {

  return (bone_w != NOTHING && bone_c != NOTHING && bone_e != NOTHING);
  
}


/* ----------------------------------------------------------------------------
 *  Has a double bone been played?
 *  
 */
boolean hasADoubleBeenPlayed(uint8_t currentMode) {
 
  // Has a double been played ?

  switch (currentMode) {
     
    case BOARD_MODE_NO_BONES_PLAYED:

      return (bone_c_pips_inner == bone_c_pips_outer);
    
    case BOARD_MODE_ONE_BONE_PLAYED:
    case BOARD_MODE_TWO_BONES_PLAYED:

      if (bone_c_pips_inner == bone_c_pips_outer) {
        return true;
      }

      if (bone_w_pips_inner == bone_w_pips_outer && bone_e == NOTHING) {
        bone_e = bone_c;
        bone_e_pips_inner = bone_c_pips_inner;
        bone_e_pips_outer = bone_c_pips_outer;
        bone_c = bone_w;
        bone_c_pips_inner = bone_w_pips_inner;
        bone_c_pips_outer = bone_w_pips_outer;
        bone_w = NOTHING;
        bone_w_pips_inner = NOTHING;
        bone_w_pips_outer = NOTHING;
        return true;
      }
      
      if (bone_e_pips_inner == bone_e_pips_outer && bone_w == NOTHING) {
        bone_w = bone_c;
        bone_w_pips_inner = bone_c_pips_outer;
        bone_w_pips_outer = bone_c_pips_inner;
        bone_c = bone_e;
        bone_c_pips_inner = bone_e_pips_inner;
        bone_c_pips_outer = bone_e_pips_outer;
        bone_e = NOTHING;
        bone_e_pips_inner = NOTHING;
        bone_e_pips_outer = NOTHING;
        return true;
      }
           
      if (bone_e_pips_inner != bone_e_pips_outer && bone_w == NOTHING) {
        bone_w = bone_c;
        bone_w_pips_outer = bone_c_pips_inner;
        bone_w_pips_inner = bone_c_pips_outer;
        bone_c = bone_e;
        bone_c_pips_inner = bone_e_pips_inner;
        bone_c_pips_outer = bone_e_pips_outer;
        bone_e = NOTHING;
        bone_e_pips_inner = NOTHING;
        bone_e_pips_outer = NOTHING;
        return false;
      }

      return false;

  }
  
  return true;
  
}


/* ----------------------------------------------------------------------------
 *  Will the nominated bone block play?
 *  
 */
boolean canBlock(uint8_t pips) {

  return (boneCounts_Overall[pips] + boneCounts_Inhand[pips] == 7);

}


/* ----------------------------------------------------------------------------
 *  Populate bone counts.
 *  
 *  When determining whether a bone could block play, a count of each pip value
 *  (from one to six) that is visible is determined by inspecting the players 
 *  hand and the played bones.   The bone counts per pip are recorded into two
 *  arrays, boneCounts_Overall and boneCounts_Inhand.
 *  
 */
void populateBoneCounts(Player player) {


  // Clear array before processing ..
  
  memset(boneCounts_Overall, 0, sizeof(boneCounts_Overall));
  memset(boneCounts_Inhand, 0, sizeof(boneCounts_Inhand));


  // Populate bone count from players hand ..

  for (uint8_t x = 0; x < players_hand_idx[static_cast<uint8_t>(player)]; x++) {
    
    boneCounts_Inhand[leftPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x])]++;
    
    if (leftPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x]) != rightPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x])) {
        boneCounts_Inhand[rightPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x])]++;
      }
    
  }


  // Populate visible bones in grave yard ..

  for (uint8_t x = 0; x < bones_played_idx; x++) {
    
    boneCounts_Overall[leftPips_ByBoneId(bones_played[x])]++;
    
    if (leftPips_ByBoneId(bones_played[x]) != rightPips_ByBoneId(bones_played[x])) {
      boneCounts_Overall[rightPips_ByBoneId(bones_played[x])]++;
    }
    
  }


  // Populate visible bones on board ..
  
  if (bone_c != NOTHING) {
    boneCounts_Overall[bone_c_pips_inner]++;
    if (bone_c_pips_inner != bone_c_pips_outer) { boneCounts_Overall[bone_c_pips_outer]++; }
  }

  if (bone_n != NOTHING) {
    boneCounts_Overall[bone_n_pips_inner]++;
    if (bone_n_pips_inner != bone_n_pips_outer) { boneCounts_Overall[bone_n_pips_outer]++; }
  }

  if (bone_s != NOTHING) {
    boneCounts_Overall[bone_s_pips_inner]++;
    if (bone_s_pips_inner != bone_s_pips_outer) { boneCounts_Overall[bone_s_pips_outer]++; }
  }

  if (bone_e != NOTHING) {
    boneCounts_Overall[bone_e_pips_inner]++;
    if (bone_e_pips_inner != bone_e_pips_outer) { boneCounts_Overall[bone_e_pips_outer]++; }
  }

  if (bone_w != NOTHING) {
    boneCounts_Overall[bone_w_pips_inner]++;
    if (bone_w_pips_inner != bone_w_pips_outer) { boneCounts_Overall[bone_w_pips_outer]++; }
  }

}


/* ----------------------------------------------------------------------------
 *  What is the value of the pips in a player's hand ? (rounded to nearest 5).
 *  
 */
uint8_t handValue(Player player) {

  uint8_t score = 0;
  
  for (uint8_t x = 0; x < players_hand_idx[static_cast<uint8_t>(player)]; x++) {
    score = score + leftPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x]);
    score = score + rightPips_ByBoneId(players_hand[static_cast<uint8_t>(player)][x]);
  }

  if (score % 5 <= 2) {

    return (score / 5) * 5;

  }
  else {

    return ((score / 5) + 1) * 5;

  }

}


/* ----------------------------------------------------------------------------
 *  Can the player play a bone?
 *  
 */
boolean canPlay(Player player) {

  for (uint8_t x = 0; x < players_hand_idx[player]; x++) {

    if (canPlayBone(players_hand[static_cast<uint8_t>(player)][x])) {

        return true;
        
    }

  }

  return false;
  
}


/* ----------------------------------------------------------------------------
 *  Can the bone be played?
 *  
 */
boolean canPlayBone(uint8_t boneId) {

  if (boardMode == BOARD_MODE_NO_BONES_PLAYED) {
    
      return true;

  }
  else {

    uint8_t leftPips = leftPips_ByBoneId(boneId);
    uint8_t rightPips = rightPips_ByBoneId(boneId);
    
    if (bone_c_pips_outer == bone_c_pips_inner) {

      if ((bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips) && (bone_e_pips_outer == NOTHING || bone_w_pips_outer == NOTHING || bone_n_pips_outer == NOTHING || bone_s_pips_outer == NOTHING)) {

        return true;

      }

    }
    else {

      if ((bone_c_pips_inner == leftPips || bone_c_pips_inner == rightPips) && bone_w_pips_outer == NOTHING) {

        return true;

      }

      if ((bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips) && bone_e_pips_outer == NOTHING) {

        return true;
      }

    }

    if (bone_e_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips))     { return true; }  
    if (bone_e_pips_outer != NOTHING && (bone_e_pips_outer == leftPips || bone_e_pips_outer == rightPips))     { return true; }
    if (bone_w_pips_outer == NOTHING && (bone_c_pips_inner == leftPips || bone_c_pips_inner == rightPips))     { return true; }
    if (bone_w_pips_outer != NOTHING && (bone_w_pips_outer == leftPips || bone_w_pips_outer == rightPips))     { return true; }

    if (boardMode == BOARD_MODE_X_AXIS_PLAYED) {

      if (bone_n_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips))    { return true; }
      if (bone_n_pips_outer != NOTHING && (bone_n_pips_outer == leftPips || bone_n_pips_outer == rightPips))    { return true; }
      if (bone_s_pips_outer == NOTHING && (bone_c_pips_outer == leftPips || bone_c_pips_outer == rightPips))    { return true; }
      if (bone_s_pips_outer != NOTHING && (bone_s_pips_outer == leftPips || bone_s_pips_outer == rightPips))    { return true; }

    }
      
  }
           
  return false;

}


/* ----------------------------------------------------------------------------
 *  Can either player make a move or draw a bone?
 *  
 */
boolean canEitherPlayerMove(GameMode gameMode) {
  
  Player player1 = getFirstPlayer(gameMode);
  Player player2 = getSecondPlayer(gameMode);
  

  // Check to see if a move exists for any player ..

  if (bones_available_idx == 0 && !canPlay(player1) && !canPlay(player2)) {

    uint8_t hand0 = handValue(player1);
    uint8_t hand1 = handValue(player2);

    char message[] = (" ~No~one~can\n~play~a~bone!");

    if (hand0 == hand1) {
      
      drawMessageBox(message, 2, true);
      return false;
    }

    if (hand0 > hand1) {
      
      players_score[static_cast<uint8_t>(player2)] = players_score[static_cast<uint8_t>(player2)] + (hand0 - hand1);
      drawMessageBox(message, 2, true, player2);
      return false;         

    }

    if (hand0 < hand1) {
      
      players_score[static_cast<uint8_t>(player1)] = players_score[static_cast<uint8_t>(player1)] + (hand1 - hand0);
      drawMessageBox(message, 2, true, player1);
      return false;         

    }

  }

  return true;
  
 }



/* ----------------------------------------------------------------------------
 *  Flash the red LED.
 *  
 */
void flashRedLED() {

  arduboy.setRGBled(25, 0, 0);
  arduboy.delayShort(100);
  arduboy.setRGBled(0, 0, 0);

}


/* ----------------------------------------------------------------------------
 *  Flash the green LED.
 *  
 */
void flashGreenLED() {
 
  arduboy.setRGBled(0, 25, 0);
  arduboy.delayShort(100);
  arduboy.setRGBled(0, 0, 0);

}


/* ----------------------------------------------------------------------------
 *  Delay before proceeding.  If the user clicks the A or B button, then abort. 
 *  
 */
void DelayOrButtonPress(uint8_t delayVal) {

  uint8_t delayCnt = 0;
  
  while (delayCnt < delayVal) {
    if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)) { break; }
    arduboy.delayShort(100);
    delayCnt++;
  }

}



/* ----------------------------------------------------------------------------
 *  Wait for a button press. 
 *  
 *  Returns the pressed button to the caller.
 *  
 */
uint8_t WaitForButtonPress() {
  
  arduboy.pollButtons();
  
  while (true) {

    if (arduboy.pressed(A_BUTTON)) { 
      return A_BUTTON;
    }
    
    if (arduboy.pressed(B_BUTTON)) { 
      return B_BUTTON;
    }
    
    arduboy.delayShort(100);
    
  }

}


/* ----------------------------------------------------------------------------
 *  Draw a horizontal dotted line. 
 *  
 *  So much nicer than a solid line!
 *  
 */
void drawHorizontalDottedLine(uint8_t x1, uint8_t x2, uint8_t y) {

  for (uint8_t x3 = x1; x3 <= x2; x3+=2) {
    arduboy.drawPixel(x3, y, WHITE);
  }
  
}


/* ----------------------------------------------------------------------------
 *  Is the game over yet?
 *  
 */
boolean isTheGameOver(GameMode gameMode) {
  
  Player player1 = getFirstPlayer(gameMode);
  Player player2 = getSecondPlayer(gameMode);

  if (players_score[static_cast<uint8_t>(player1)] > GAME_FIRST_TO_REACH_SCORE || players_score[static_cast<uint8_t>(player2)] > GAME_FIRST_TO_REACH_SCORE) {

    if (players_score[static_cast<uint8_t>(player1)] > GAME_FIRST_TO_REACH_SCORE && player1 == Player::Computer) {
      
      drawMessageBox(F("\n  I won !"), 3, true);

    }
    else {
      
      drawMessageBox(F("Great~work"), 3);
      font4x6.setCursor(63,20);
      font4x6.setTextColor(BLACK);
      font4x6.print(players[static_cast<uint8_t>(playersTurn)]->getString());
      font4x6.print(",\nyou~won~!");
      font4x6.setTextColor(WHITE);
      arduboy.display();
    
    }

    arduboy.pollButtons();
    DelayOrButtonPress(200);
    return true;

  }
  else {
    
    return false;
    
  }

}


/* ----------------------------------------------------------------------------
 *  Has either player played all their bones?
 *  
 */
boolean isAnyoneOut(GameMode gameMode) {
  
  Player player1 = getFirstPlayer(gameMode);
  Player player2 = getSecondPlayer(gameMode);


  // Game still in play?

  for (uint8_t x = static_cast<uint8_t>(player1); x <= static_cast<uint8_t>(player2); x++) {


    // Has the player run out of bones?
    
    if (players_hand_idx[x] == 0) {

//    char message[] = "You~scored~an\nan extra~   \npoints!";
      char message[] = "You~scored~a \nbonus of~   \npoints!";
      printHand();

      //renderPlayersHand(player1, players_hand_highlight_idx, false);

      if (x == static_cast<uint8_t>(Player::Computer)) {

        players_score[static_cast<uint8_t>(player1)] = players_score[static_cast<uint8_t>(player1)] + handValue(player2);
        drawMessageBox(F("~I~have~used\n~up~all~of~my\n~bones!"), 3, true, Player::Computer);

      }

      if (x == static_cast<uint8_t>(Player::Human_1)) {

        uint16_t extraPoints = handValue(x == player2 ? player1 : player2);
        drawMessageBox(F("You~have~used\nup~all~of~your\nbones!"), 3, true);

        if (extraPoints < 100) {
          message[23] = (extraPoints / 10) + 48;
          message[24] = (extraPoints % 10) + 48;
          message[25] = ' ';
        }
        else {
          message[23] = (extraPoints / 100) + 48; extraPoints = extraPoints % 100;
          message[24] = ((extraPoints / 10) % 10) + 48;
          message[25] = (extraPoints % 10) + 48;
        }
        
        players_score[static_cast<uint8_t>(x == player1 ? player1 : player2)] = players_score[static_cast<uint8_t>(x == player1 ? player1 : player2)] + extraPoints;
        drawMessageBox(message, 3, true, Player::Human_1);

      }

      if (x == static_cast<uint8_t>(Player::Human_2)) {

        uint16_t extraPoints = handValue(player1);
        drawMessageBox(F("You~have~used\nup~all~of~your\nbones!"), 3, true, Player::Human_2);

        if (extraPoints < 100) {
          message[25] = (extraPoints / 10) + 48;
          message[26] = (extraPoints % 10) + 48;
          message[27] = ' ';
        }
        else {
          message[25] = (extraPoints / 100) + 48; extraPoints = extraPoints % 100;
          message[26] = ((extraPoints / 10) % 10) + 48;
          message[27] = (extraPoints % 10) + 48;
        }

        players_score[static_cast<uint8_t>(player2)] = players_score[static_cast<uint8_t>(player2)] + extraPoints;
        drawMessageBox(message, 3, true, Player::Human_2);

      }

      gameState = GameState::PlayGame;
      return true;
      
    }

  }

  return false;
  
}


void printHand() {

  #ifdef DEBUGs

  Serial.println("--------------------------------------------");
  Serial.print("PlayersTurn :");
  Serial.println(playersTurn);
  
  if (gameMode == GameMode::SinglePlayer) {
    Serial.print("Comp:  ");
    for (uint8_t x = 0; x < players_hand_idx[0]; x++) {
      uint8_t b = bones_ref[players_hand[0][x]];
      Serial.print( b >> 4 );
      Serial.print( b & 0x0f );
      Serial.print(" ");
    }  
    Serial.println(" ");
  }

  Serial.print("Play1: ");
  for (uint8_t x = 0; x < players_hand_idx[1]; x++) {
    uint8_t b = bones_ref[players_hand[1][x]];
    Serial.print( b >> 4 );
    Serial.print( b & 0x0f );
    Serial.print(" ");
  }  
  Serial.println(" ");

  if (gameMode == GameMode::TwoPlayer) {
    Serial.print("Play2: ");
    for (uint8_t x = 0; x < players_hand_idx[2]; x++) {
      uint8_t b = bones_ref[players_hand[2][x]];
      Serial.print( b >> 4 );
      Serial.print( b & 0x0f );
      Serial.print(" ");
    }  
    Serial.println(" ");
  }

  #endif

}
