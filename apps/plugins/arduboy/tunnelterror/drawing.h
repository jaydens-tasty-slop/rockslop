// Functions to draw main game pieces & dice
void draw_dice(int8_t X, int8_t Y, int8_t die_num){
  switch (die_num)
  {
  case 1:
    Sprites::drawOverwrite(X,Y,Dice1, 0);
    //arduboy.drawBitmap(X,Y,Dice1,30,30,WHITE);
    break;
  case 2:
    Sprites::drawOverwrite(X,Y,Dice2, 0);
    //arduboy.drawBitmap(X,Y,Dice2,30,30,WHITE);
    break;
  case 3:
    Sprites::drawOverwrite(X,Y,Dice3, 0);
    //arduboy.drawBitmap(X,Y,Dice3,30,30,WHITE);
    break;
  case 4:
    Sprites::drawOverwrite(X,Y,Dice4, 0);
    //arduboy.drawBitmap(X,Y,Dice4,30,30,WHITE);
    break;
  case 5:
    Sprites::drawOverwrite(X,Y,Dice5, 0);
    //arduboy.drawBitmap(X,Y,Dice5,30,30,WHITE);
    break;
  case 6:
    Sprites::drawOverwrite(X,Y,Dice6, 0);
    //arduboy.drawBitmap(X,Y,Dice6,30,30,WHITE);
    break;
  }
}
void draw_alien(int8_t alien_type){
  switch (alien_type)
  {
  case 1:
    Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Alien_Small1, 0);
    break;
  case 2:
    Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Alien_Small2, 0);
    break;
  case 3:
    Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Alien_Small3, 0);
    break;
  }
}
void draw_armour(int8_t armour_type){
  switch (armour_type)
  {
  case 1:
    Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Armour1, 0);
    break;
  case 2:
    Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Armour2, 0);
    break;
  }
}
void draw_food(){
  Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Food1, 0);
}
void draw_shield(){
  Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Shield, 0);
}
void draw_sword(){
  Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Sword, 0);
}
void draw_bottle(){
  Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Bottle, 0);
}
void draw_potion(){
  Sprites::drawOverwrite(MIDDLE.x-15,MIDDLE.y,Potion, 0);
}
void draw_chalice(int8_t X, int8_t Y){
  Sprites::drawOverwrite(X,Y,Chalice, 0);
}
void draw_alien_big(int8_t X, int8_t Y){
  Sprites::drawOverwrite(X,Y,Alien_Big, 0);
}
