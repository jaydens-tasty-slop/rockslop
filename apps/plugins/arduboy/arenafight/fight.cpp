#include <stdlib.h>
#include <math.h>
#include "Globals.h"
#include "actor.h"
#include "gameEngine.h"


void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  int  menuCase = 0;
  gameStatus = intro;
  arduboy.clear();
   
}

//Main loop
void loop() {
  if(!arduboy.nextFrame()){
    return;
  }
  arduboy.pollButtons();

  if( arduboy.justPressed(UP_BUTTON) == true){
      menuCase--;
    }
    if( arduboy.justPressed(DOWN_BUTTON) == true){
       menuCase++;
    }

  if( arduboy.notPressed(A_BUTTON) == true ) {
    abuff = 0;
  }

  switch (gameStatus) {
     case GameStatus::intro:
     Intro();
     break;
     case GameStatus::charSelect:
     gameStart();
     break;
     case GameStatus::menu:
     MainMenu();
     break;
     case GameStatus::battle:
     fight();
     break;
     case GameStatus::statScreen:
     StatMenu();
     break;
     case GameStatus::store:
     Store();
     break;
     case GameStatus::bossBattle:
     bossFight();
     break;
  }  
  arduboy.display();
}
