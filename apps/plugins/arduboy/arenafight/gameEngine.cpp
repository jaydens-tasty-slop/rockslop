#include <stdlib.h>
#include <math.h>
#include "gameEngine.h"
#include "Globals.h"
#include "actor.h"
#include "fightEngine.h"
#include "images.h"
//#include "bossActors.h"
#include "player.h"
#include "enemy.h"

//INITIALIZE ACTORS
//actor(char* nm, int h, int str, int def, int spd, int spc, byte BMP, equpment Item);
playerCharacter player = playerCharacter();
enemy mob = enemy("troll", {4, 2, 0, "Club"}, {30, 20, 6, 4,},  troll_bmp, 4);
//actor boss = actor("null", 0, 0, 0, 0, 0, warrior_bmp, {0, 0, 0, "null", 0});
//enemy slime = enemy("slime", {3, 0, 3, "slime ball"}, {25, 8, 8, 6, 8}, slime_bmp); 
 //actor troll = actor("troll", 30, 20, 6, 2, 4, warrior_bmp, {4, 2, 0, "Club", 0}); 
 //actor ogre = actor("ogre", 40, 20, 15, 4, 8, warrior_bmp, {6, 0, 0, "Ogre Claws", 0}); 
//playerCharacter test = playerCharacter("ogre", 40, 20, 15, 4, 8, warrior_bmp, {6, 0, 0, "Ogre Claws", 0});

//STORE INVENTORY ARRA
const equpment storeInventory[] = {{5, 2, 0, "Kleaver", 10}, {2, 5, 0, "Broad Shield", 10}, {0, 2, 5, "Wand", 10}, {7, 5, 3, "Katana", 20}, {5, 7, 3, "Iron Sheild", 20}, {3, 5, 7, "Mangus Rod", 20}}; 

//OTHER INTS FOR MENU LOCATIONS

uint16_t abuff = 0;
uint16_t equiptMenu = 0;
bool infight = false;
uint16_t itemSelected;
uint16_t arenaLvl = 0;      
uint16_t inbossfight =  0;
int mobHP;
int liveCounter = 5;

//CHARACTER SELECTION
void gameStart(){
  player = playerCharacter();
  arduboy.clear();
  drawBoarder();
  arduboy.setCursor(4, 3);
  arduboy.print(F("Pick your class."));
  arduboy.print(F("\n"));
  arduboy.drawLine(1, 13, 127, 13, WHITE);
  arduboy.setCursor(04, 15);

    
    if (menuCase > 2){ menuCase = 0;}
    if (menuCase < 0){ menuCase = 2;} 

    switch (menuCase) {

      case 0:
      arduboy.println(F("Warrior"));
      arduboy.drawBitmap(4, 40, warrior_bmp, 20, 20, WHITE);
      arduboy.setCursor(04, 25);
      arduboy.print(F("Strong Attack"));
      
      break;
      case 1:
      arduboy.println(F("Tank"));
      arduboy.drawBitmap(4, 40, tank_bmp, 20, 20, WHITE);
      arduboy.setCursor(04, 25);
      arduboy.print(F("High Health & Defense"));
      break;
      case 2:
      arduboy.print(F("Mage"));
      arduboy.drawBitmap(4, 40, mage_bmp, 20, 20, WHITE);
      arduboy.setCursor(04, 25);
      arduboy.print(F("Fast and frail"));
      break;
  
    }


   if( arduboy.pressed(A_BUTTON) == true and abuff ==0){
      abuff = 1;
      switch (menuCase) {

      case 0:
      player.pickClass(0);
      menuCase = 0;
      gameStatus = menu;
      break;
      case 1:
      player.pickClass(1);
      menuCase = 0;
      gameStatus = menu;
      break;
      case 2:
      player.pickClass(2);
      menuCase = 0;
      gameStatus = menu;
      break;
    }
 
  }

}


//INTRO SCREEN
void  Intro(){
  arduboy.clear();
  arduboy.drawBitmap(0, 0, splashScreen, 128, 64, WHITE);
  //ADD GAME SPLASH SCREEN
  arduboy.setCursor(25, 0);
  arduboy.print(F("Hit b to start"));
  //ADD HELP SCREEN
  if (arduboy.justPressed(B_BUTTON)){
    gameStatus = charSelect;
    menuCase = 0;
  }
}

//MAIN MENU AND ROOM SELECTION
void MainMenu() {
  
  arduboy.clear();
  arduboy.setCursor(48, 4);
  
  switch(arenaLvl){
    case 0:
    arduboy.print(F("Arena Rookie"));
    break;
    case 1:
    arduboy.print(F("Journeyman"));
    break;
    case 2:
    arduboy.print(F("Arena Master"));
    break;
  }
  //arduboy.print(F("MAIN MENU"));
  //arduboy.drawLine(0, 10, 130, 10, WHITE);
  arduboy.drawBitmap(0,0, mainMenu,128,64, WHITE);
  
  arduboy.setCursor(0, 12);
  
    if (menuCase > 4){ menuCase = 0;}
    if (menuCase < 0){menuCase = 4;} 

      arduboy.setCursor(9, 4);
      arduboy.print(F("Stats"));
      arduboy.setCursor(9, 14);
      if( player.hp > 0){
        arduboy.print(F("Battle"));
      }
      arduboy.setCursor(9, 24);
      arduboy.print(F("Reset"));
      arduboy.setCursor(9, 34);
     // if(arenaLvl < 3){
       arduboy.print(F("Fight Arena Boss"));   //  }
     // else{
     //  arduboy.print(F("You are the arena champion"));
     // }
      arduboy.setCursor(9, 44);
      arduboy.print(F("Store"));


    switch (menuCase) {

      case 0:
     // arduboy.print(F("stats"));
      arduboy.setCursor(3, 4);
      arduboy.print(F(">"));
      break;
      case 1:
        arduboy.setCursor(3, 14);
        arduboy.print(F(">"));
      break;
      case 2:
      arduboy.setCursor(3, 24);
      arduboy.print(F(">"));
      break;
      case 3:
      if(arenaLvl < 4){
      arduboy.setCursor(3, 34);
      arduboy.print(F(">"));
      break;
      case 4:
      arduboy.setCursor(3, 44);
      arduboy.print(F(">"));
      break;
    }
    }


   if( arduboy.pressed(A_BUTTON) == true and abuff ==0){
      abuff = 1;
      switch (menuCase) {

      case 0:
      gameStatus = statScreen;
      menuNum = 0;
      break;
      case 1:
      if( player.hp > 0){
        gameStatus = battle;
      }
      
      break;
      case 2:
      setup();
      break;
      case 3:
      if( player.hp > 0  && arenaLvl < 4){
        gameStatus = bossBattle;
      }
      break;
      case 4:
      gameStatus = store;
      menuCase = 0;
      menuNum = 0;
      break;
    }
    }


}

//INITIALIZE FIGHT ROOM 
void fight() {
  
    if(infight == false){
      arduboy.clear();
      generateMob();
      infight = true;
      menuCase = 0;

    }
  //RUN BATTLE
    BattleScene(mob);
  
  
  
}

void bossFight(){
  if(infight == false){
      arduboy.clear();
      //boss = slime;
      infight = true;
      menuCase = 0;
      inbossfight = 1;
    
    if(arenaLvl == 0){
      setMob("slime", {3, 0, 3, "slime ball"}, {28, 8, 10, 6, 8}, slime_bmp, 3);
      //mob.statSeed = {25, 8, 8, 6, 8};
      //mob.name = "Slime";
      mob.bmp = slime_bmp;
      //mob.weapon = {3, 0, 3, "slime ball"};
      //mob.level = 3;
    }
    else if (arenaLvl == 1){
      setMob("troll", {4, 2, 0, "Club"}, {20, 15, 10, 7, 8},  troll_bmp, 4);
      mob.bmp = troll_bmp;
      //mob.name = "Troll";
     // mob.level = 2;
    }
    else if (arenaLvl == 2){
      setMob("ogre", {6, 0, 0, "Ogre Claws"}, {20, 20, 15, 5, 0}, ogre_bmp, 5);
      mob.bmp = ogre_bmp;
     // mob.name = "Ogre";
    }
     else if (arenaLvl == 3){
      setMob("Master", {15, 0, 0, "Schimtar"}, {20, 15, 15, 10, 10}, ogre_bmp, 6);
      mob.bmp = arenaMaster_bmp;
     // mob.name = "Master";
     }
    }
  
    BattleScene(mob);
}


//PRINT PLAYER STATS AND MANAGE INVENTORY
void  StatMenu() {
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print(F("STATS v-INV >EQPT"));
    arduboy.drawLine(0, 10, 130, 10, WHITE);


    if(arduboy.pressed(B_BUTTON)) {
    gameStatus = menu;
    }

    if(equiptMenu == 0){
      player.printStats();

    if(arduboy.pressed(RIGHT_BUTTON)) {
      equiptMenu =1;
      menuCase =0;
    }
  }

  if (equiptMenu == 1){
    arduboy.clear();
    arduboy.setCursor(0, 0);
    arduboy.print(F("<STATS "));
    arduboy.drawLine(0, 10, 130, 10, WHITE);
    arduboy.setCursor(0, 12);
    arduboy.println(F("Select item to equipt"));
    
      if (menuCase > 4){ menuCase = 0;}
      if (menuCase < 0){menuCase = 4;} 
      arduboy.print(player.inventory[menuCase].name);
      
      if( arduboy.pressed(A_BUTTON) == true and abuff ==0){
          player.equipt(menuCase);
          equiptMenu = 0;

      }    
    
      if(arduboy.pressed(LEFT_BUTTON)) {
        equiptMenu =0;
      }
  }
}

//END SCREEN PAUSE ActorNm FIGHTS
//ADD LOOT/MONEY/DROPPED ITEMS/ ETC
void VictoryScreen(){
  arduboy.clear();
  drawBoarder();
  arduboy.setCursor(20, 20);
  arduboy.println(F("Victory"));
  arduboy.setCursor(15, 30);
  arduboy.println(F("take 2 coins"));
  arduboy.setCursor(15, 40);
  arduboy.println(F("as your prize"));
  arduboy.setCursor(10, 50);
  arduboy.println(F("B to return to menu"));

  //add bonuses

  if (arduboy.justPressed(B_BUTTON)){

    gameStatus = menu;
    infight = false; 
    player.wallet += 2;
    player.ex += mob.expDrop();
    player.checkLvlUp();
    
    if(inbossfight == 1){
      arenaLvl +=1;
      //player.levelUp();
      inbossfight = 0;
     }
  }
  
}

//death screen
void FailScreen(){
  arduboy.clear();
  drawBoarder();
  arduboy.setCursor(10, 20);
  arduboy.println(F("You have Failed"));
  arduboy.setCursor(10, 30);
  arduboy.println(F("B to return to menu"));

  if(player.wallet < 1){
    arduboy.setCursor(10, 40);
    arduboy.println(F("The Arena Pitties you"));
    arduboy.setCursor(10, 50);
    arduboy.println(F("Rest your wounds"));
    
  }else{
    arduboy.setCursor(10, 40);
    arduboy.print(F("you've lost 2 coins"));
  }
  
  //Add Repercussions

  if (arduboy.justPressed(B_BUTTON)){
    
      gameStatus = menu;
      infight = false; 
      player.hp = player.getStat(player.statSeed.totalHP);
      if(inbossfight == 1){
        inbossfight = 0;}
        
     
     if(player.wallet >= 2){
      player.wallet -= 2;}
     else {
      player.wallet = 0;
     }
  }
}

//FIGHT LOOP
void BattleScene(actor& opponent) {
 if (player.hp >0){
  if (opponent.hp >0){
    arduboy.clear();
    arduboy.setCursor(6, 4);
    arduboy.print(opponent.name);
    arduboy.setCursor(6, 14);
    arduboy.print(F("hp: "));
    arduboy.println(opponent.hp);
   // arduboy.drawLine(6, 14, 69, 14, WHITE);
    arduboy.drawBitmap(5, 27, battleBKG, 56,32);
    arduboy.drawBitmap(22, 29, mob.bmp, 20, 20, WHITE);
    

   // arduboy.drawLine(64, 1, 64, 64);
    drawFightBoarder();
    
    drawPlayerInfo();
    //drawBoarder();
    Engagement(player, opponent);

 
  }
  else {
    VictoryScreen();
    state = 0;
  }
 }
 else{
  FailScreen();
  state = 0;
 }
}

//PURCHASE EQUIPTMENT/HEAL
void Store() {

  arduboy.clear();
  arduboy.drawBitmap(0,0, storeScreen, 128, 64, WHITE);
  arduboy.setCursor(8, 41);
  arduboy.print("What'll have ye?");
 // arduboy.print(F("Store"));
  //arduboy.drawLine(0, 10, 130, 10, WHITE);
  arduboy.setCursor(5, 54);

  //scroll menu


    if(arduboy.pressed(B_BUTTON)) {
    gameStatus = menu;
    menuCase = 0;
    }

    if(menuNum == 0){
    if (menuCase > 2){ menuCase = 0;}
    if (menuCase < 0){menuCase = 2;} 

    switch (menuCase) {

      case 0:
      arduboy.print(F("Buy new equiptment"));
      break;
      case 1:
      if (player.hp < player.getStat(player.statSeed.totalHP)){
        arduboy.print(F("HEAL: 1 coin"));
      }else{
        arduboy.print(F("You need not heal"));
      }
      break;
      case 2:
      if (player.potion >= 0 && player.potion < 2 && player.wallet > 2){
          arduboy.print(F("Buy a Potion? 3 Coin"));
      }else if (player.wallet < 3){
        arduboy.print(F("ye short on coin"));
      }
      else {
        arduboy.print(F("Ye potion bag is ful"));
      }
      break;
   
    }

     if( arduboy.pressed(A_BUTTON) == true and abuff ==0){
      abuff = 1;
      switch (menuCase) {
       case 0:
      menuNum = 1;
      break;
      case 1:
      if(player.wallet > 0 && player.hp < player.getStat(player.statSeed.totalHP)){
      player.hp = player.getStat(player.statSeed.totalHP);
      player.wallet -=1;
      }
      break;
      case 2:
      if (player.potion >= 0 && player.potion <= 2 && player.wallet > 2){
          player.potion++;
          player.wallet -=3;
      }
      break;
      }
      
    }
   }

//display store inventory
  if(menuNum == 1){
    if (menuCase > 5){ menuCase = 0;}
    if (menuCase < 0){menuCase = 5;} 
   
    arduboy.clear();
    arduboy.setCursor(8, 41);
    arduboy.println(F("Here's what ye got"));
    arduboy.drawBitmap(0,0, storeScreen, 128, 64, WHITE);
    arduboy.setCursor(5, 54);
    arduboy.print(storeInventory[menuCase].name);
    arduboy.print(F(":"));
    arduboy.print(storeInventory[menuCase].cost);
    arduboy.println(F("coins"));

    if (player.wallet < storeInventory[menuCase].cost){
      arduboy.setCursor(8, 41);
      arduboy.print(F("Ye Broke"));
    }

      if( arduboy.pressed(A_BUTTON) == true and abuff == 0 and player.wallet >= storeInventory[menuCase].cost){
        abuff = 1;
        itemSelected = menuCase;
        menuNum = 2;
      }
   }

//pick slot to place item   
  if(menuNum == 2){
    if (menuCase > 4){ menuCase = 0;}
    if (menuCase < 0){menuCase = 4;} 
  
    arduboy.clear();
    arduboy.drawBitmap(0,0, storeScreen, 128, 64, WHITE);
    arduboy.setCursor(8, 41);
    arduboy.println(F("Select inv slot"));
    arduboy.setCursor(5, 54);
    arduboy.print(F("slot #: "));
    arduboy.print(menuCase);
    arduboy.print(" ");
    arduboy.println(player.inventory[menuCase].name);
    
    if( arduboy.pressed(A_BUTTON) == true and abuff ==0){
      player.inventory[menuCase] = storeInventory[itemSelected];
      player.wallet -= storeInventory[itemSelected].cost;
      menuNum = 0;
      abuff = 1;
    }  
  }
    
      
}

void generateMob(){
  int x = rand() %5;
  
 int moblvl= (player.level-1) + (rand() %(3));
 if(moblvl < 1){moblvl=1;}
  
  switch(x){
      case 0:
      setMob("", {1, 0, 0}, {8, 8, 5, 4, 0}, slime_bmp, moblvl);
      mob.name = "Knight";
      mob.bmp = warrior_bmp;
      break;
      
      case 1:
      setMob("", {0, 1, 0}, {10, 4, 7, 2, 2}, slime_bmp, moblvl);
      mob.name = "Tank";
      mob.bmp = tank_bmp;
      break;

      case 2:
      setMob("", {0, 0, 1}, {8, 1, 2, 8, 8}, mage_bmp, moblvl);
      mob.name = "Warlock";
      mob.bmp = mage_bmp;
      break;

      case 3:
      setMob("", {1, 0, 1}, {5, 7, 4, 8, 1}, mage_bmp, moblvl);
      mob.name = "Spider";
      mob.bmp = spider_bmp;
      break;

      case 4:
      setMob("", {0, 1, 1}, {7, 8, 6, 1, 3}, mage_bmp, moblvl);
      mob.name = "Orc";
      mob.bmp = orc_bmp;
      break;
    }
}

void drawBoarder(){
    arduboy.drawLine(3, 1, 123, 1, WHITE);
    arduboy.drawLine(3, 62, 123, 62, WHITE);
    arduboy.drawLine(1, 4, 1, 60, WHITE);
    arduboy.drawLine(126, 4, 126, 60, WHITE);

    arduboy.drawLine(3, 4, 1, 1, WHITE);
    arduboy.drawLine(124, 4, 126, 1, WHITE);
    arduboy.drawLine(124, 60, 126, 62, WHITE);
    arduboy.drawLine(3, 60, 1, 62, WHITE);
}

void drawPlayerInfo(){
  //arduboy.drawLine(64, 1, 64, 64);

    
    arduboy.setCursor(67, 36);
    arduboy.print(player.hp);
    arduboy.print(F("/"));
    arduboy.print(player.getStat(player.statSeed.totalHP));
    arduboy.print(F("HP"));
   // arduboy.drawLine(56, 60, 118, 60, WHITE);

    arduboy.drawBitmap(100, 6, player.bmp, 20, 20, WHITE);
    arduboy.setCursor(0, 50);

    arduboy.drawLine(67, 48, 122, 48, WHITE);
    arduboy.drawLine(67, 58, 122, 58, WHITE);
    arduboy.drawLine(67, 48, 67, 58, WHITE);
    arduboy.drawLine(122, 48, 122, 58, WHITE);

    arduboy.setCursor(66, 6);
    arduboy.print(F("lvl:"));
    arduboy.setCursor(88, 13);
    arduboy.print(mob.level);

    arduboy.setCursor(70, 50);

      if (player.potion == 1){
        arduboy.drawBitmap( 70, 23, potion_bmp, 8,8, WHITE);
      }
      else if (player.potion == 2){
        arduboy.drawBitmap( 70, 23, potion_bmp, 8,8, WHITE);
        arduboy.drawBitmap( 80, 23, potion_bmp, 8,8, WHITE);
      }
}


void drawFightBoarder(){
    arduboy.drawBitmap(0,0, battleMap, 128, 64);
}

void setMob(const char * nm, equpment wpn, baseStats seed, const byte * sprite, int lvl){
      mob.statSeed = seed;
      mob.name = nm;
      mob.bmp = sprite;
      mob.weapon = wpn;
      mob.level = lvl;
      mob.hp = mob.getStat(mob.statSeed.totalHP);
}

