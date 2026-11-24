#include "Game.h"
#include "Pokitto.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::flashHero(char sym){
  if(hero.hblnd==0) {
    drawMap();
    drawThing();
    drawMonst();
  }
  locate(hero.hx, hero.hy);
  PD::print(sym);
  // arduboy.display();
  // arduboy.digitalWriteRGB(RED_LED, RGB_ON);
  // arduboy.invert(true);
  // delay(70);
  // arduboy.invert(false);
  // arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
}

void Game::flashMonst(uint8_t num, char sym){
  if(hero.hblnd==0) {
    drawMap();
    drawThing();
    drawMonst();
  }

  locate(mx[num], my[num]);
  if(hero.hblnd == 0 && bitRead(m1[num],6)==0){
    PD::print(sym);
  } else {
    PD::print(' ');    
  }
  // arduboy.display();
  // arduboy.digitalWriteRGB(BLUE_LED, RGB_ON);
  // delay(70);
  // arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
}

void Game::initHero(){

  for(int i=0; i<IMAX; i++){
    deleteItem(i);
  }
  memset(&hero, 0, sizeof(hero));
    
  
  hero.im = 5, hero.dlv = 1, hero.st = 16, hero.stm = 16, hero.lv = 1;

  hero.hp = 16, hero.hpm = 16, hero.hh = 400, hero.nl = 10;

  inv[0].ii = 2 * 16 + 0; inv[0].i1 =  1; inv[0].i2 = 0; inv[0].i3 = 0; inv[0].i4 = 0b10000000;
  inv[1].ii = 3 * 16 + 0; inv[1].i1 =  1; inv[1].i2 = 1; inv[1].i3 = 1; inv[1].i4 = 0b00110000;
  inv[2].ii = 3 * 16 + 3; inv[2].i1 =  1; inv[2].i2 = 1; inv[2].i3 = 0; inv[2].i4 = 0b00100000;
  inv[3].ii = 3 * 16 + 5; inv[3].i1 = 30; inv[3].i2 = 0; inv[3].i3 = 0; inv[3].i4 = 0b10100000;
  inv[4].ii = 4 * 16 + 1; inv[4].i1 =  1; inv[4].i2 = 1; inv[4].i3 = 0; inv[4].i4 = 0b00110000;

}

void Game::initTable(){
  const uint8_t vari[4] = {POMAX, SCMAX, RGMAX, WDMAX};
  for (int v = 0; v < 4; v++) {
    for (int i = 0; i < 50; i++) {
      uint8_t r1 = random(vari[v]);
      uint8_t r2 = random(vari[v]);
      uint8_t tmp = ttab[v][r1];
      ttab[v][r1] = ttab[v][r2];
      ttab[v][r2] = tmp;
    }
  }
  for(int i=0; i<4; i++){
    tknow[i]=0;
  }
}
