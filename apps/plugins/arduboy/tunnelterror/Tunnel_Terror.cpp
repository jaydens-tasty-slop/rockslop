/*  Tunnel Terror by Tom Millichamp & Jamie Millichamp
    January 2020
    Copyright 2020 Tom Millichamp
*/
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>
Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled); //enable sound (Arduboy Tones lib)

enum gamestates {title,gameplay,lose,win} gamestate;

int8_t strength=1, stamina=2, lives=3, chalices=0;
int8_t stamina_counter=0, level=1; //piece, 
bool forward_allowed=false, right_allowed=false, left_allowed=false, door=false;
bool invincibility=false, dbl_power=false, final_alien=false;;
// These are for the 3D points to create corridors from:
Point MIDDLE(WIDTH/2,HEIGHT/2);
Point DEPTH_LEFT_TOP(20, 10), DEPTH_LEFT_BOTTOM(20, HEIGHT-10) ;
Point DEPTH_RIGHT_TOP(108, 10), DEPTH_RIGHT_BOTTOM(108, HEIGHT-10) ;
Point DOOR_LEFT_TOP(45,22), DOOR_LEFT_BOTTOM(45,HEIGHT-10);
Point DOOR_RIGHT_TOP(83,22), DOOR_RIGHT_BOTTOM(83,HEIGHT-10);

class alien
{
  public:
    int8_t strength, stamina, type;
  public:
    alien(int8_t level, int8_t player_strength, int8_t player_stamina, bool final_alien){
      arduboy.initRandomSeed();
      strength=random(1,(player_strength/2)+level+2);
      stamina=random(1,(player_stamina/2)+level);
      type=random(1,4);
      if (level<3)
        type=random(1,3);
      if (final_alien){
        strength=random(player_strength/2,player_strength+1);
        stamina=random(player_stamina/2,player_stamina+1);
      }
    }
};

#include "graphics.h" // bitmaps are in separate file to keep it clean
#include "drawing.h" // also the functions to draw our items
#include "common.h" //common functions such as waitForButtonAPress
#include "corridors.h" //function to create corridor pieces
#include "battle_routine.h" //functions for fighting aliens

void alien_in_corridor();
void open_door();

void setup()
{
  arduboy.begin(); // initialize Arduboy
  arduboy.setFrameRate(15);
  //arduboy.audio.on();
  strength = 1;
  stamina = 2;
  lives = 3;
  level=1;
  chalices=0;
  stamina_counter=0;
  invincibility=false; //potions
  gamestate = title;
}

void loop()
{
  if (!arduboy.nextFrame())
    return;

  switch (gamestate)
  {
  case title:
    arduboy.clear(); //This also sets cursor to 0,0
    arduboy.drawLine(0,0,WIDTH,HEIGHT,WHITE);
    arduboy.drawLine(WIDTH,0,0,HEIGHT,WHITE);
    arduboy.setTextSize(2);
    arduboy.setCursor(30, 0);
    arduboy.print(F("Tunnel"));
    arduboy.setCursor(30, 18);
    arduboy.print(F("Terror"));
    arduboy.setTextSize(1);
    arduboy.setCursor(20, 50);
    arduboy.print(F("Press A to Start"));
    arduboy.display();
    arduboy.pollButtons();
    if (arduboy.justPressed(A_BUTTON)){
      create_corridor();
      gamestate=gameplay;
    }
    break;

  case gameplay:
    if (lives==0){
          gamestate=lose;
          break;
        }
    print_top_bar();
    arduboy.pollButtons();
    if (arduboy.justPressed(UP_BUTTON) && forward_allowed == true)
    {
      create_corridor();
      alien_in_corridor();
    }
    else if (arduboy.justPressed(UP_BUTTON) && door == true)
    {
        //player decided to open a door!
        open_door();
        create_corridor();
     }
    else if (arduboy.justPressed(LEFT_BUTTON) && left_allowed == true)
    {
      create_corridor();
      alien_in_corridor();
    }
    else if (arduboy.justPressed(RIGHT_BUTTON) && right_allowed == true)
    {
      create_corridor();
      alien_in_corridor();
    }
    break;

 
  case lose:
    arduboy.clear();
    arduboy.setTextSize(2);
    arduboy.print(F("Game Over!"));
    arduboy.setCursor(0, 20);
    arduboy.setTextSize(1);
    arduboy.print(F("\nNo more Lives\nPress A"));
    arduboy.display();
    waitForButtonAPress();
    setup();
    break;

  case win:
    arduboy.clear();
    arduboy.print(F("Zarc is DEAD!"));
    arduboy.print(F("\nPress A"));
    Sprites::drawOverwrite(MIDDLE.x-20,20,Dead_Alien, 0);
    arduboy.display();
    waitForButtonAPress();
    setup();
    break;
  }
  arduboy.display();
} // end of main loop


void end_game(){
    arduboy.clear();
    arduboy.print(F("Well Done!\nOnly Zarc stands\nin your way!"));
    draw_alien_big(MIDDLE.x-20,34);
    waitForButtonAPress();
    if (strength>10)
            strength=10;
    alien finalAlien(level,strength,stamina,true); //create the end of game alien
    fight_alien(finalAlien);
    if(strength>0)
      gamestate=win; //the player beat the big alien!
    else
      gamestate=lose;
}

// randomly create an alien in the corridor!
void alien_in_corridor(){
  arduboy.initRandomSeed();
  if (random(level,15)==8){
    sound.tone(NOTE_C4,500, NOTE_C3,800);
    alien newAlien(level,strength,stamina,false);
    draw_alien(newAlien.type);
    arduboy.setCursor(0,10);
    arduboy.print(F("You must fight!"));
    waitForButtonAPress();
    fight_alien(newAlien);
    create_corridor();
  }
}

void open_door(){
  arduboy.clear();
  alien newAlien(level,strength,stamina,false);
  arduboy.initRandomSeed();
  int8_t discovery = random(1, 15);
  int8_t chalice_counter=4; //x start co-cord to show row of chalices
  switch (discovery)
    {
    case 1:
      /* alien */
      sound.tone(NOTE_C4,500, NOTE_C3,800);
      draw_alien(newAlien.type);
      arduboy.setCursor(0,10);
      waitForButtonAPress();
      fight_alien(newAlien);
      return;
      break;
    case 2:
      /* sword */
      sound.tone(NOTE_C5,50, NOTE_E5,50, NOTE_G6,50);
      arduboy.print(F("Found Sword\n+1 Strength"));
      draw_sword();
      strength++;
      break;
    case 3:
      /* shield */
      sound.tone(NOTE_C5,50, NOTE_E5,50, NOTE_G6,50);
      arduboy.print(F("Found Shield\n+1 Strength"));
      draw_shield();
      strength++;
      break;
    case 4: case 5:
      /* food */
      sound.tone(NOTE_C5,50, NOTE_E5,50, NOTE_G6,50);
      arduboy.print(F("Found Food\n+1 Health"));
      draw_food();
      stamina++;
      break;
    case 6:
      /* armour */
      sound.tone(NOTE_C5,50, NOTE_E5,50, NOTE_G6,50);
      arduboy.print(F("Found Armour\n+1 Strength"));
      draw_armour(random(1,3));
      strength++;
      break;
    case 7:
      /* Chalice!! */
      sound.tone(NOTE_C4,50, NOTE_E4,50, NOTE_G4,50);
      arduboy.print(F("Found a Chalice!"));
      chalices++;
      strength++;
      stamina++;
      for(int8_t i=1;i<=chalices;i++){
        draw_chalice(chalice_counter, 20);
        chalice_counter+=34;
      }
      if (chalices==3){
          waitForButtonAPress();
          end_game(); //they have founbd all 3 chalices!
      }
      level++; //move to next level
      break;
    case 8:
      /* Invincibility Potion */
      sound.tone(NOTE_C4,50, NOTE_E4,50, NOTE_G4,50);
      arduboy.setCursor(0,0);
      arduboy.print(F("Invincibility Potion\n+1 Health"));
      draw_potion();
      invincibility=true;
      stamina++;
      break;
    case 9:
      /* double power Potion */
      sound.tone(NOTE_C4,50, NOTE_E4,50, NOTE_G4,50);
      arduboy.setCursor(0,0);
      arduboy.print(F("Double Attack Potion\n+1 Health"));
      draw_potion();
      dbl_power=true;
      stamina++;
      break;
    case 10:
      /* Drink */
      sound.tone(NOTE_C5,50, NOTE_E5,50, NOTE_G6,50);
      arduboy.setCursor(0,0);
      arduboy.print(F("Found Drink\n+1 Health"));
      draw_bottle();
      stamina++;
      break;
    case 11: case 12: case 13: case 14:
      /*Empty Room*/
      arduboy.setCursor(0,0);
      arduboy.print(F("Empty Room\nMove along!"));
      break;
    }
    if (strength>10)
            strength=10;
    if (chalices!=3)
      waitForButtonAPress();
}
