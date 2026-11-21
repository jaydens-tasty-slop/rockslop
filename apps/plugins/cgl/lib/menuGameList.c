/* Each game rock links exactly one game; the rest resolve to NULL. */
#include "menu.h"

void addGamePakuPaku(void) __attribute__((weak));
void addGameThunder(void) __attribute__((weak));
void addGameBallTour(void) __attribute__((weak));
void addGameHexmin(void) __attribute__((weak));
void addGameSurvivor(void) __attribute__((weak));
void addGamePinCLimb(void) __attribute__((weak));
void addGameColorRoll(void) __attribute__((weak));
void addGameCastN(void) __attribute__((weak));
void addGameReflector(void) __attribute__((weak));
void addGameFroooog(void) __attribute__((weak));
void addGameRWheel(void) __attribute__((weak));
void addGameLadderDrop(void) __attribute__((weak));
void addGameGrapplingH(void) __attribute__((weak));
void addGameInTow(void) __attribute__((weak));
void addGameTimberTest(void) __attribute__((weak));
void addGameFracave(void) __attribute__((weak));

void addGames(void) {
  if (addGamePakuPaku) addGamePakuPaku();
  if (addGameThunder) addGameThunder();
  if (addGameBallTour) addGameBallTour();
  if (addGameHexmin) addGameHexmin();
  if (addGameSurvivor) addGameSurvivor();
  if (addGamePinCLimb) addGamePinCLimb();
  if (addGameColorRoll) addGameColorRoll();
  if (addGameCastN) addGameCastN();
  if (addGameReflector) addGameReflector();
  if (addGameFroooog) addGameFroooog();
  if (addGameRWheel) addGameRWheel();
  if (addGameLadderDrop) addGameLadderDrop();
  if (addGameGrapplingH) addGameGrapplingH();
  if (addGameInTow) addGameInTow();
  if (addGameTimberTest) addGameTimberTest();
  if (addGameFracave) addGameFracave();
}
