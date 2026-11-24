#include "Game.h"
#include "Pokitto.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

uint8_t Game::inputWait() {
  uint8_t result = 0;
  do {
    if (PC::buttons.pressed(BTN_LEFT)) result = 1;
    if (PC::buttons.pressed(BTN_UP)) result = 2;
    if (PC::buttons.pressed(BTN_RIGHT)) result = 3;
    if (PC::buttons.pressed(BTN_DOWN)) result = 4;
    if (PC::buttons.pressed(BTN_A)) result = 5;
    if (PC::buttons.pressed(BTN_B)) result = 6;
  } while (result == 0);
  setActiveMessage(29);
//  clearBuf();
  return result;
}

void Game::drawInventry(uint8_t st, uint8_t mode) {
  uint8_t ed;
//  arduboy.clear();
  locate(0, 0);
  if(mode==1){
    PD::print(" which}");
  }
  locate(0, 1 - !mode );
  PD::print('>');

  if (st + 8 - mode > hero.im) {
    ed = hero.im;
  } else {
    ed = st + 8 - mode;
  }
  for (int i = st; i < ed; i++) {
    locate(1, i - st + 1 - !mode );
    char buff;
    if(bitRead(inv[i].i4,3)==1){
      buff='{';
    } else {
      buff = tsym[inv[i].ii / 16 - 1];
    }
    PD::print(buff);
    PD::print(' ');

    if(bitRead(inv[i].i4,7)==1){
        PD::print(inv[i].i1);
        PD::print(' ');      
    }
    if(inv[i].ii/16<5){
      itmToGitm(inv[i].ii/16,inv[i].ii%16,0);
    } else {
      itmToGitm(inv[i].ii/16,inv[i].ii%16,bitRead(tknow[inv[i].ii/16-5],inv[i].ii%16));
    }
    PD::print(activeMessage);
    if(bitRead(inv[i].i4,5)==1){
      if(inv[i].ii/16 == 3){           //weapon
        PD::print('^');
        PD::print((int)inv[i].i2);
        PD::print('<');
        PD::print((int)inv[i].i3);
        PD::print(']');      
      } else if(inv[i].ii/16 == 4 || inv[i].ii/16 == 8){      //armor or ring
        PD::print('^');
        PD::print((int)inv[i].i2);
        PD::print(']');
      } else if(inv[i].ii/16 == 7){
        if(inv[i].i2 != 0){
          PD::print('^');
          PD::print((int)inv[i].i2);
          PD::print(']');
        }
      }
    }
    if(bitRead(inv[i].i4,4)==1){
      PD::print(" E");
    }
    if(bitRead(inv[i].i4,6)==1 && bitRead(inv[i].i4,1)==1){
      PD::print(" C");
    }
  }
//  arduboy.display();
}

uint8_t Game::inventry(uint8_t mode) {
  uint8_t st = 0;
  uint8_t ex = 0;
  do {
    drawInventry(st, mode);
    uint8_t a = inputWait();
    if (a == 2) {
      if (st > 0) st--;
    }
    if (a == 4) {
      if (st < hero.im - 1) st++;
    }
    if (a == 5) {
      if(mode==0){
        ex = action(st);
      } else {
        ex=1;
      }
    }
    if (a == 6) {
      ex = 1;
    }
  } while (ex == 0);
  return st;
}

uint8_t Game::action(uint8_t st) {
  uint8_t curs = 0;
  PD::setColor(0);
  PD::fillRect(17, 3, 57, 35);
  PD::setColor(1);
  PD::fillRect(18, 4, 55, 33);
  PD::setColor(0);
  PD::fillRect(19, 5, 53, 31);
  locate(5, 1);
  PD::print(" use");
  locate(5, 2);
  PD::print(" throw");
  locate(5, 3);
  PD::print(" drop");
  locate(4, curs + 1);
  PD::print('>');

//  arduboy.display();
  uint8_t ex = 0;
  do {
    uint8_t a = inputWait();
    switch (a) {
      case 2:
        if (curs > 0) {
          locate(4, curs + 1);
          PD::print(' ');
          curs--;
          locate(4, curs + 1);
          PD::print('>');
//          arduboy.display();
        }
        break;
      case 4:
        if (curs < 2) {
          locate(4, curs + 1);
          PD::print(' ');
          curs++;
          locate(4, curs + 1);
          PD::print('>');
//          arduboy.display();
        }
        break;
      case 5:
        if(curs==0){
          if(hero.hslep == 0) {
            useItem(st);
            sortItem();
            moveMonst();
            tweatHero();
          } else {
            setActiveMessage(22);
          }
          ex=1;
        } else if(curs==1){
          if(hero.hslep == 0) {
            throwItem(st);
            sortItem();
          } else {
            setActiveMessage(22);
          }  
          ex=1;
        } else if(curs==2){
          if(hero.hslep == 0){
            dropItem(hero.hx, hero.hy, st);
            sortItem();
            moveMonst();
            tweatHero();
          } else {
            setActiveMessage(22);
          }
          ex=1;
//        } else if(curs==3){
//          saveStatus();
//          loadStatus();
//          gstate=2;
//          ex=1;
        }
        break;
      case 6:
        ex = 1;
        break;
    }
  } while (ex == 0);
  return 1;
}

void Game::showStatus()
  {
  uint8_t h = 0;
  uint8_t menu = 0;
  if (hero.hx < 10)
  {
    h = 11;
  }
  
  if(hero.hblnd==0) drawMap();
  drawHero();
  if(hero.hblnd==0) drawThing();
  if(hero.hblnd==0) drawMonst();

    if (hero.hx < 10)
    {
      PD::setColor(0);
      PD::fillRect(62, -1, 66, 66);
      PD::setColor(0);
      PD::fillRect(63, -1, 64, 66);
      PD::setColor(0);
      PD::fillRect(64, -1, 63, 66);
    }
    else
    {
      PD::setColor(0);
      PD::fillRect(0, -1, 65, 66);
      PD::setColor(0);
      PD::fillRect(0, -1, 64, 66);
      PD::setColor(0);
      PD::fillRect(0, -1, 63, 66);
    }
    
  locate(h, 0);
  PD::print("Lv;");
  PD::print(hero.dlv);
  locate(h, 1);
  PD::print("Au;");
  PD::print(hero.au);
  locate(h, 2);
  PD::print("Hp;");
  PD::print(hero.hp);
  PD::print('~');
  PD::print(hero.hpm);
  locate(h, 3);
  PD::print("Hg;");
  PD::print(hero.hh);
  locate(h, 4);
  PD::print("St;");
  PD::print(hero.st);
  PD::print('~');
  PD::print(hero.stm);
  locate(h, 5);
  PD::print("AC;");
  uint8_t a=equip(4,1);
  uint8_t ac=0;
  if(a==0){
    ac=0;
  } else {
    ac=(astat[ inv[a-1].ii % 16] ) + inv[a-1].i2;
  }
  PD::print((int)ac);
  locate(h, 6);
  PD::print("Ex;");
  PD::print(hero.lv);
  PD::print('~');
  locate(h + 3, 7);
  PD::print(hero.ex);
}

void Game::search() {
  for (int i = hero.hx - 1; i <= hero.hx + 1; i++) {
    for (int j = hero.hy - 1; j <= hero.hy + 1; j++) {
      if (dungeon[i][j] >= 31 && dungeon[i][j] <= 106) {
        dungeon[i][j] = dungeon[i][j] + 80;
      } else if (dungeon[i][j] >= 221 && dungeon[i][j] <= 226) {
        dungeon[i][j] = dungeon[i][j] - 200;
      } else if (dungeon[i][j] == 228) {
        dungeon[i][j] = dungeon[i][j] - 220;
      } else if (dungeon[i][j] >= 231 && dungeon[i][j] <= 236) {
        dungeon[i][j] = dungeon[i][j] - 210;
      }
    }
  }
}
