#include "Game.h"
#include "Pokitto.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

void Game::setup(GameCookie *cookie) {
  uint8_t r;
  this->cookie = cookie;

  loadHiScore();
  sortItem();
}

void Game::loop() {

  PC::buttons.pollButtons();
  PD::clear();
  switch (gstate) {
    case 0:
      title();
      break;
    case 1:
      gameloop();
      break;
    case 2:
      gameover();
      break;
    case 3:
      winner();
      break;
    case 4:
      score();
      break;
    case 5:
      landing();
      break;
  }
  
}



void Game::title() {
  locate(6, 2);
  PD::print("@ Rogue");
  locate(5, 5);
  PD::print("A] New Game");
  locate(5, 6);
  PD::print("B] Continue");
  //SJH
  // if ((millis() / 300) % 2 == 0) {
  //   locate(13, 2);
  //   PD::print('"');
  // } else {
  //   locate(13, 2);
  //   PD::print(' ');
  // }
  
  if (wiz == 1) {
    locate(0, 7);
    PD::print(adepth);
  }

  if (PC::buttons.pressed(BTN_UP)) {
    wiz = 1;
    if (adepth >= 2) {
      adepth--;
    }
  }
  if (PC::buttons.pressed(BTN_DOWN)) {
    wiz = 1;
    if (adepth <= 25) {
      adepth++;
    }
  }
  if (PC::buttons.pressed(BTN_A)) {
    initTable();
    initHero();
    clearKnown();
    buildDungeon();
    setActiveMessage(0);
    gstate = 1;
  }
  if (PC::buttons.pressed(BTN_B) ){
    printf("Btn B 1\n");
    if(EEPROM.read(20)==1){
      EEPROM.update(20,0);
      loadStatus();
      gstate = 5;
    }
  }
}

void Game::landing(){
  locate(5,2);
  PD::print("Next floor ");
  PD::print(hero.dlv);
  locate(5,5);
  PD::print("A] Go Next");
  locate(5,6);
  PD::print("B] Save~quit");

  if (PC::buttons.pressed(BTN_A) ) {
    setActiveMessage(29);
    buildDungeon();
    gstate = 1;
  }

  if (PC::buttons.pressed(BTN_B) ) {
    printf("Btn B 2\n");
    EEPROM.update(20,1);
    saveStatus();
    wiz=0;
    adepth=26;
    gstate = 0;
  }
}

void Game::gameover() {
  
  wiz = 0;
  adepth = 26;
//SJH  EEPROM.update(20,0);

  locate(2, 1);
  PD::print("( REST IN PEACE (");
  locate(5, 3);
  PD::print("_  Hero  _");

  if (death == 0) {
    locate(7, 5);
    PD::print("Starve");
  } else if (death == 1) {
    locate(7, 5);
    PD::print("Frozen");
  } else if(death==2){
    locate(7, 4);
    PD::print("lack of");
    locate(8, 5);
    PD::print("luck");
  } else if(death==3){
    locate(7, 4);    
    PD::print("hellfire");
  } else {
    locate(7, 4);
    PD::print("Killed");
    locate(8, 5);
    PD::print("by");
    locate(11, 5);
//SJH    PD::print((char)pgm_read_uint8_t(mname + death - 4));
  }
  locate(11 - digits(hero.au), 6);
  PD::print("$");
  PD::print(hero.au);
  if (PC::buttons.pressed(BTN_A)) {
    rank = checkHiScore();
    gstate = 4;
  }
}

uint8_t Game::digits(uint16_t num) {
  uint8_t ans = 1;
  while (num > 10) {
    ans++;
    num /= 10;
  }
  return ans;
}

void Game::winner() {
  wiz = 0;
  adepth = 26;
  EEPROM.update(20,0);

  locate(0, 3);
  PD::print(" , CONGRATULATION ,  ");
  locate(0, 4);
  PD::print("YOU MADE IT OUT WITH ");
  locate(0, 5);
  PD::print("THE AMULET OF YENDOR ");

  if (PC::buttons.pressed(BTN_A)) {
    rank = checkHiScore();
    gstate = 4;
  }
}

void Game::score() {

  locate(0, 0);
  PD::print("- ) Hall Of Fame )  -");

  for (int i = 0; i < 5; i++) {
    if (rank == i + 1) {
    locate(0, 2 + i);
    PD::print('>');
    }

    locate(1, 2 + i);
    PD::print(i + 1);
    locate(3, 2 + i);
    PD::print(glory[4 - i].gold);
    locate(14, 2 + i);
    if (glory[4 - i].depth == 255) {
      PD::print("Win ,");
    } else {
      PD::print(glory[4 - i].depth);
    }
  }

  if (PC::buttons.pressed(BTN_A)) {
    gstate = 0;
  }
  if (PC::buttons.pressed(BTN_UP) && PC::buttons.pressed(BTN_DOWN)) {
    for (int i = 0; i < 5; i++) {
      glory[i] = {0, 0};
    }
    clearHiScore();
  }

}

uint8_t Game::checkHiScore() {
  uint8_t result = 0;
  if (hero.dlv == 0) hero.dlv = 255;
  if (glory[0].gold < hero.au) {
    glory[0] = {hero.au, hero.dlv};
    for (int i = 0; i < 5; ++i) {
      for (int j = i + 1; j < 5; ++j) {
        if ( glory[i].gold > glory[j].gold ) {
          tglory = glory[i];
          glory[i] = glory[j];
          glory[j] = tglory;
        }
      }
    }
    saveHiScore();
    for (int i = 0; i < 5; i++) {
      if (glory[i].gold == hero.au) {
        result = 5 - i;
      }
    }
  }
  return result;
}

void Game::gameloop() {
  uint8_t mm, r;

  if (PC::buttons.pressed(BTN_LEFT)) {
    printf("move Left\n");
    heroMove(1);
  }

  if (PC::buttons.pressed(BTN_RIGHT)) {
    printf("move Right\n");
    heroMove(3);
  }

  if (PC::buttons.pressed(BTN_UP)) {
    printf("move Up\n");
    heroMove(2);
  }

  if (PC::buttons.pressed(BTN_DOWN)) {
    //SJH if (arduboy.buttonsState() == 24) {
    //  ss = 1;
    //} else {
    printf("move Down\n");
      heroMove(4);
    //}
  }

  if (PC::buttons.pressed(BTN_A)) {
    if (ss == 1) {
      ss = 0;
    } else {
      if (dungeon[hero.hx][hero.hy] >= 11 && dungeon[hero.hx][hero.hy] <= 16 && hero.hheld == 0) {
        hero.hmdet = 0;
        hero.hisee = 0;
        if (inv[hero.im - 1].ii == 144) {
          hero.dlv--;
        } else {
          hero.dlv++;
        }
        if (hero.dlv == 0) {
          gstate = 3;
        } else {
          gstate = 5;
        }
      } else {
        search();
        wakeUp();
        tweatHero();
        moveMonst();
      }
    }
  }

  if (PC::buttons.pressed(BTN_B)) {
    printf("Btn B 3\n");
    ss = 0;
    setActiveMessage(29);
    inventry(0);
  }

  makeKnown();
  if (hero.hblnd == 0) drawMap();
  drawHero();
  if (hero.hblnd == 0) drawThing();
  if (hero.hblnd == 0) drawMonst();
    showMsg();
  if (ss == 1) {
    showStatus();
  }
}

void Game::buildDungeon() {
  clearDungeon();
  makeDungeon4();
  placeMonst();
  placeThing();
  //  mess(0);
  //  addBuf((char)dlv);
}

void Game::heroMove(uint8_t dir) {
  if (hero.hconf > 0) {
    dir = random(4) + 1;
  }
  int8_t dx = (dir - 2) * (dir % 2);
  int8_t dy = (dir - 3) * ((dir - 1) % 2);
  uint8_t mm, r;

  if (hero.hslep > 0) {
    dx = 0;
    dy = 0;
  }
  ss = 0;
  if (monst[hero.hx + dx][hero.hy + dy] == 0) {
    printf("Hero hx %i, dx %i, hy %i, dy %i, dun %i\n", hero.hx, dx, hero.hy, dy, dungeon[hero.hx + dx][hero.hy + dy]);
    if ((hero.hy + dy) <= 7 && (hero.hy + dy) >= 0 && (hero.hx + dx) >= 0 && (hero.hx + dx) <= 20
        && dungeon[hero.hx + dx][hero.hy + dy] >= 1 && dungeon[hero.hx + dx][hero.hy + dy] <= 200) {
    printf("a1\n");
      if (hero.hheld == 0) {
        hero.hx = hero.hx + dx;
        hero.hy = hero.hy + dy;
    printf("a11\n");
      }
      setActiveMessage(29);
//      clearBuf();
      if (hero.hlevi == 0) {
        checkThing(hero.hx, hero.hy);
        if(dungeon[hero.hx][hero.hy] >= 31 && dungeon[hero.hx][hero.hy] <= 106){
          dungeon[hero.hx][hero.hy] += 80;
        }
        if(dungeon[hero.hx][hero.hy] >= 111 && dungeon[hero.hx][hero.hy] <= 186){
          traped( dungeon[hero.hx][hero.hy]/10-11 );
        }
      }
    }
  } else {
    printf("b");
    mm = monst[hero.hx + dx][hero.hy + dy];
    r = ms[mm - 1] % 32;
    setActiveMessage(29);
//    clearBuf();
    hitMonst(mm, r, dx, dy);
  }
  wakeUp();
  tweatHero();
  if (hero.hfast > 0) {
    if (hero.ht % 2 == 0) {
      moveMonst();
    }
  } else {
    moveMonst();
  }
}

// bear?

void Game::traped(uint8_t vari){
  uint8_t dmg=0;
  if(random(2)==0){
    flashHero('^');
  
    switch (vari){
      case 0:     //door
        hero.dlv++;
        buildDungeon();
        break;
      case 1:     //arrow
        dmg = random(3)+1;
        charon(dmg, 2);
        break;
      case 2:     //sleep
        if(hero.hslep==0) hero.hslep=5;
        break;
      case 3:     //teleport
        teleportHero();
        break;
      case 4:     //poison
        if (hero.st > 3 && hasRing(5) == 0) hero.st--;
        break;
      case 5:     //rust
        uint8_t eq = equip(4, 1);
        rustArmor( eq );
        break;
    }
  }
  setActiveMessage( 23 + vari);
}

void Game::charon(uint8_t dmg, uint8_t reason){
  if( hero.hp <= dmg ) {
    death = reason;
    gstate = 2;
  } else {
    hero.hp=hero.hp-dmg;
  }
}
// 0 :steave
// 1 :frozen
// 2 :lack of luck
// 3 :hellfire
// 4-:killed by X

void Game::rustArmor(uint8_t eq){
  if ( equip(4, 1) != 0 && bitRead(inv[eq - 1].i4, 3) == 0) {
    if (hasRing(9) == 0 && astat[inv[eq - 1].ii % 16 ] + inv[eq - 1].i2 > 1) {
      inv[eq - 1].i2--;
    }
  }
}

