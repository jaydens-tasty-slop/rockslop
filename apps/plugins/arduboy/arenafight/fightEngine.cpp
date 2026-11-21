#include <stdlib.h>
#include <math.h>
#include "gameEngine.h"
#include "Globals.h"
#include "actor.h"
#include "player.h"

enum moveType {
  attack,
  defend,
  special,
  magic,
  item,
  skip, 
};

moveType playerMove;
moveType enemyMove;

uint16_t moveChoiseMade;
bool playerFirst;
bool turnComplete;
uint16_t eMoveChoise;



//take in 2 entities

void engage(actor& agressor, actor& target, moveType agesMove, moveType trgtMove){

      if(agesMove == attack){
        if(trgtMove == defend){
          target.takeDamage(agressor, (target.getStat(target.statSeed.defense)));
          
        
        }else if( trgtMove == attack){
          target.takeDamage(agressor, 0);
          
        }
        else{ //magic/item use results in more damage taken
          target.takeDamage(agressor, 2); 
              
        }
      }
      else if (agesMove == defend)
      {          target.takeDamage(agressor, -5); 
      }  
      else if (agesMove == item){}//nothing
      else if (agesMove == skip){}
      else if (agesMove == special){
        
      }
      else if (agesMove == magic){
        target.takeSpecial(agressor);
        
      }
        
      
}

void Engagement(playerCharacter& plyr, actor& cpu){
if(state == 0){
//recieve player input
  if(moveChoiseMade == 0){
    turnComplete = false;
    if (menuCase > 3){ menuCase = 0;}
    if (menuCase < 0){menuCase = 3;} 

    switch (menuCase) {

      case 0:
      arduboy.print(F("Attack"));
      break;
      case 1:
      arduboy.print(F("Defend"));
      break;
      case 2:
      arduboy.print(F("Magic"));
      break;
      case 3:
      arduboy.print(F("Potion"));
      break;
   
    }


   if( arduboy.justPressed(A_BUTTON) == true and abuff ==0){
      abuff = 1;
      switch (menuCase) {

      case 0:
      playerMove = attack;
      moveChoiseMade = 1;
      break;
      case 1:
      playerMove = defend;
      moveChoiseMade = 1;
      break;
      case 2:
      playerMove = magic;
      moveChoiseMade = 1;
      break;
      case 3:
      if (plyr.potion > 0){
      plyr.usePotion();
      playerMove = item;
      moveChoiseMade = 1;
      }
      break;
    }
   }


// get cpu move

  uint16_t cpuAtkRand = rand() %3;

  switch (cpuAtkRand) {

      case 0:
      enemyMove = attack;
      eMoveChoise = 1;
      break;
      case 1:
      enemyMove = defend;
      eMoveChoise = 1;
      break;
      case 2:
      enemyMove = special;
      eMoveChoise = 1;
      break;
    }
  }
  else if (moveChoiseMade == 1 && eMoveChoise == 1){
  //determine which moves first
    if(plyr.getStat(plyr.statSeed.speed) > cpu.getStat(cpu.statSeed.speed)) {
    playerFirst = true;
    }
    else{ playerFirst = false;}

    //execute commands

    if (playerFirst == true && turnComplete == false){
      engage(plyr, cpu, playerMove, enemyMove);
      engage(cpu, plyr, enemyMove, playerMove);

    //IMPLEMENT A PAUSE THAT READS ENEMY MOVE SELECTION
      //if (arduboy.everyXFrames(60)){
     // arduboy.print(enemyMove);}
      abuff = 1;
      turnComplete = true;
      moveChoiseMade = 0;
      state=1;

    }
    else if (playerFirst == false && turnComplete == false) {

      engage(cpu, plyr, enemyMove, playerMove); 
      engage(plyr, cpu, playerMove, enemyMove);
      abuff = 1;
      turnComplete = true;
      moveChoiseMade = 0;
      state =1;
    }
//loop

  }
}

else if(state == 1){
  arduboy.clear();
  drawFightBoarder();
  drawPlayerInfo();
  arduboy.setCursor(70, 50);
  arduboy.print(F("continue"));
  arduboy.setCursor(6,4);
   if(enemyMove == defend){
         arduboy.println(F("The mob"));
         arduboy.setCursor(6,14);
         arduboy.println(F("Defended"));
          
        
        }else if( enemyMove == attack){
          arduboy.println(F("The mob"));
          arduboy.setCursor(6,14);
          arduboy.println(F("Attacked"));
          
        }
        else{
          arduboy.println(F("The mob"));
          arduboy.setCursor(6,14);
          arduboy.println(F("used ")); 
          arduboy.setCursor(6,24);
          arduboy.println(F("Magic")); 

              
        }
  if(arduboy.justPressed(A_BUTTON) and abuff ==0){
    abuff = 1;
    state = 0;}
  
}
}



