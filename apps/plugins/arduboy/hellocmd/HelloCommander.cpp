#include <Arduino.h>
#include "globals.h"
#include "world.h"
#include "player.h"
#include "enemy.h"

void waitXFrames( uint8_t t, bool redraw );
void stateReset();
void stateRoundStart();
void pre();
void post();
FunctionPointer gameState = stateReset, retState;

Player player;
Enemy enemy;

uint16_t frame;


void waitXFrames( uint8_t t, bool redraw ){
    while( t-- ){
        while( !arduboy.nextFrame() );
        if( redraw ) pre();
        post();
    }
}

void stateReset(){
    nextMap=1947;
    prevMap=2512;
    dialoguePosition = 0;
    msgDelivered = 1;
    level = 0;
    world.generate(nextMap);
    player.gameReset();
    gameState = stateBossTalk;
    arduboy.setFrameRate(10);
}

void stateRoundStart(){
    enemy.roundReset();
    if( player.roundReset() )
        gameState = statePlayerTurn;
    else
        gameState = stateBossTalk;
}

void setup() {
  arduboy.boot();
  arduboy.flashlight();
  arduboy.setFrameRate(10);
  
  world.initSeed( micros() );
}

uint8_t previousButtonState, currentButtonState, _justPressed;

void pre(){
    arduboy.fillScreen(0);
    if( camera+1 )
        world.render( HIGHBYTE(camera), LOWBYTE(camera) );
}

void post(){
    if( printstr ){
        arduboy.setCursor( printstrX, printstrY );
        arduboy.print( PGMSTR(printstr) );
    }
    arduboy.display();
}

void loop() {
    
    if (!(arduboy.nextFrame()))
        return;

    previousButtonState = currentButtonState;
    currentButtonState = arduboy.buttonsState();
    _justPressed = (previousButtonState^currentButtonState)&currentButtonState;

    if( arduboy.everyXFrames(2) ) frame++;
    
    pre();
    
    auto oldGameState = gameState;
    gameState();
    
    stateInitialized = gameState == oldGameState;
    
    post();
    
}
