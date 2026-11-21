#include "Game.h"
#include "Pokitto.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;


const char * Game::getItemName(uint8_t type, uint8_t r, uint8_t k)
{

  const uint8_t index[13]={0,1,3,11,18,32,44,55,65,66,80,92,103};
  if(k==0){
  	return nitem[index[type-1]+r];
  } else {
  	return nitem[index[type+4]+ttab[type-5][r]];
  }
}
