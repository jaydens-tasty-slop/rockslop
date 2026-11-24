#include "Game.h"
#include "Pokitto.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::saveHiScore(){
  // for(int i=0; i<5; i++){
  //   EEPROM.put(100+i*5, glory[i]);
  // }
}

void Game::loadHiScore(){
  // for(int i=0; i<5; i++){
  //   EEPROM.get(100+i*5, glory[i]);
  // }
}

void Game::clearHiScore(){
  // for(int i=0; i<25; i++){
  //   EEPROM.write(100+i, 0);
  // }  
}

void Game::saveStatus(){
  // EEPROM.put(200, hero);
  // for(int i=0; i<26; i++){
  //   EEPROM.put(300+i*5, inv[i]);
  // }
  // for(int i=0; i<4; i++){
  //   for(int j=0; j<14; j++){
  //     EEPROM.put(500+i*14+j, ttab[i][j]);
  //   }
  // }
  // for(int i=0; i<4; i++){
  //   EEPROM.put(600+i*2, tknow[i]);
  // }
}

void Game::loadStatus(){
  // EEPROM.get(200, hero);
  // for(int i=0; i<26; i++){
  //   EEPROM.get(300+i*5, inv[i]);
  // }
  // for(int i=0; i<4; i++){
  //   for(int j=0; j<14; j++){
  //     EEPROM.get(500+i*14+j, ttab[i][j]);
  //   }
  // }
  // for(int i=0; i<4; i++){
  //   EEPROM.get(600+i*2, tknow[i]);
  // }
}

