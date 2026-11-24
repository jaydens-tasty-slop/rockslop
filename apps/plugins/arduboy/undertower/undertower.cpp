#include <Arduino.h>
//#include <SPI.h>
//Use for PROGMEM
//#include <avr/pgmspace.h>
#include "overworld.h"
#include "save.h"
#include "dialogue.h"
#include "battle.h"
#include "dungeon.h"
#include "events.h"
#include <Arduboy2.h>
#define MAPSIZE 16
/* Arduino IDE auto-prototypes */
void step_transition();
void print_progress();
void try_combat();
uint16_t calculate_damage(uint8_t lvl);
void load_enemy_data(uint8_t index, uint8_t slot);
void gen_enemies();
void copy_to_buffer(byte index, const char arr[][8]);
uint8_t append_to_msg_buffer(uint8_t index, const char arr[][8], uint8_t offset);
void copy_action_to_msg_buffer(uint8_t source, uint8_t dest, uint16_t amount, uint8_t type);
void draw_menu(byte index);
void give_xp();
void do_combat_step();
void do_combat();
uint8_t huff_tree_get(uint8_t item);
void fill_dialogue_buffer(uint16_t seek_index);
void display_dialogue(uint16_t index, uint8_t len, uint8_t who, const char name_list[][8]);
void step_dialogue();
void draw_dungeon();
byte test_collision(byte dir);
void test_collide_exit(uint8_t collision);
void step_dungeon();
void mapinit(char map[][MAPSIZE], int width, int height);
void mapgen(char map[][MAPSIZE], int mapwidth, int mapheight, int startx, int starty, int endx, int endy);
uint8_t check_proximity(char map[][MAPSIZE], uint8_t i, uint8_t j, uint8_t extra);
void mapdetail(char map[][MAPSIZE], int width, int height);
void mapexits(char map[][MAPSIZE], int width, int height);
void check_events();
uint8_t world_get(uint16_t index);
void draw_world();
uint8_t test_world_collision(uint8_t dir);
void step_world();
void save_game();
void restore_game();

Arduboy2 gb; //Replacing Gamebuino object with Arduboy2 object

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define WORLD 0
#define COMBAT 1
#define DUNGEON 2
#define DIALOGUE 3
//Transitions to world or dungeon modes
#define TO_WORLD 4
#define TO_COMBAT 5
#define TO_DUNGEON 6
//Menu mode
#define MAIN_MENU 7
#define PAUSE_MENU 8
#define GAME_OVER 9
#define YOU_WIN 10

#define TRANSITION_DIFF 4

byte mode = WORLD;

void setup() {
  // put your setup code here, to run once:
  gb.begin();
  gb.setFrameRate(25);
  gb.invert(true);//Make white the predominant color
  //gb.titleScreen(F("Test"));
  //gb.battery.show = false;
  mode = MAIN_MENU;

  /*for(byte i = 0; i < 16; i++){
    for(byte j = 0; j < 16; j++){
      map_cache[i][j] = pgm_read_byte(&world[i*64+j]);
    }
  }*/
}

void step_transition(){
  uint8_t i;
  for(i = 0; i < SCREEN_HEIGHT/2+1 - abs(transition); i++ ){
    gb.drawFastVLine(i,0,SCREEN_HEIGHT);
    gb.drawFastVLine(SCREEN_WIDTH-i,0,SCREEN_HEIGHT);
    gb.drawFastHLine(0,i,SCREEN_WIDTH);
    gb.drawFastHLine(0,SCREEN_HEIGHT-i,SCREEN_WIDTH);
  }
  transition+=4;
  //If we are halfway into the transition, start drawing the next scene
  //And stop saving the last frame of the previous scene
  if( transition >= 0 ){
    persistence = false;
  }
  //If we are done with the transition, move to the ordinary non-transition mode
  if( transition > SCREEN_HEIGHT/2 ){
    mode -= TRANSITION_DIFF;
  }
}

void print_progress(){
  uint16_t progress = 0;
  for( uint8_t i = 0; i < 6; i++ ){
    progress += game_status[i];
  }
  progress = (100*progress + (36+3+4+4+4+4)/2)/(36+3+4+4+4+4);
  gb.setCursor(SCREEN_WIDTH/2-2*6, SCREEN_HEIGHT-8);
  gb.print(progress);
  gb.print(F("%"));
}

void loop() {
  if(gb.nextFrame()){
    if( !persistence ){
      gb.clear();
    }
    gb.pollButtons();
    
    //Set common values and tick animation frames regardless of mode, to save space by avoiding duplication between modes
    gb.setCursor(0, 6);
  
    dudeframe++;
    dudeframe%=7;

    worldframe++;
    worldframe%=8;
    
    switch( mode ){
      case TO_WORLD:
        step_transition();
      case WORLD:
        if( transition >= 0 ){
          draw_world();
          step_world();
          if( game_status[STATUS_MAIN] == -1 ){
            game_status[STATUS_MAIN] = 0;
            display_dialogue(TXT_INTRO,TXT_INTRO_LEN,5,player_names);
          }
          else if( game_status[STATUS_MAIN] == 0 && dudex < 7*8 ){
            game_status[STATUS_MAIN] = 1;
            display_dialogue(TXT_SDW_INTRO,TXT_SDW_INTRO_LEN,SHADOW,player_names);
          }else if( game_status[STATUS_MAIN] == 1 && dudey < 42*8 ){
            game_status[STATUS_MAIN] = 2;
            display_dialogue(TXT_SDW_CATPAW,TXT_SDW_CATPAW_LEN,SHADOW,player_names);
          }/*else if( game_status[STATUS_MAIN] == 12 && dudex > 55*8 && dudey < 45*8 ){
            game_status[STATUS_MAIN] = 13;
            display_dialogue(TXT_SDW_SHIP,TXT_SDW_SHIP_LEN,SHADOW,player_names);
          }*/
        }
        break;
      case TO_DUNGEON:
        step_transition();
      case DUNGEON:
        if( transition >= 0 ){
          draw_dungeon();
          step_dungeon();

          check_events();//Dialogue and boss battles encountered in dungeons
        }
        break;
      case TO_COMBAT:
        if( transition >= 0 ){
          do_combat();//Draw first to avoid the text overdrawing the transition
        }
        step_transition();
        break;
      case COMBAT:
        if( transition >= 0 ){
          do_combat();
        }
        break;
      case DIALOGUE:
        if( meta_mode == WORLD )
          draw_world();
        else
          draw_dungeon();
        step_dialogue();
        break;
      case MAIN_MENU:
        // Draw tower on right
        for( uint8_t i = 0; i < 8; i++ ){
          uint8_t tile = 38;
          for( uint8_t j = 0; j < 5; j++ ){
            // Draw windows every other row at 5th position
            if( j == 4 && i % 2 == 1 ){
              tile = 11;
            }
            // If we are no longer drawing the curved wall, draw normal wall
            else if( j > 2 ){
              tile = 27;
            }
            gb.drawSlowXYBitmap( SCREEN_WIDTH - 5*8 + j*8, i*8, &tiles[tile*8], 8, 8 );
          }
        }
        //Make background behind text black
        gb.fillRect(0,0,128 - 5*8,64);
        gb.setTextColor(BLACK);
        gb.setTextBackground(WHITE);
        gb.setCursor(6, 0);
        gb.print(F("UNDER\n      THE\n         TOWER"));
        gb.setCursor(2*6, SCREEN_HEIGHT/2+8);
        gb.println(F("NEW"));
        gb.setCursor(2*6, gb.getCursorY());
        gb.println(F("LOAD"));
        gb.setCursor(1*6, SCREEN_HEIGHT/2+8);
        if(menu_selection == 1){
          gb.setCursor(1*6, SCREEN_HEIGHT/2+16);        
        }
        gb.print(F("\20"));
        if(gb.justPressed(UP_BUTTON)){
          menu_selection--;
          if( menu_selection == 255 ) menu_selection = 1;
        }
        else if(gb.justPressed(DOWN_BUTTON)){
          menu_selection++;
          menu_selection%=2;
        }
        else if(gb.justPressed(A_BUTTON)){
          gb.initRandomSeed();//For random numbers, later
          if(menu_selection == 1){
            restore_game();
          }else{
            mode = WORLD;
          }
        }
        break;
      case PAUSE_MENU:
        gb.fillRect(0,0,128,64);
        gb.setCursor(SCREEN_WIDTH/2-3*6, gb.getCursorY());
        gb.println(F("PAUSED"));
        gb.setCursor(1*6, gb.getCursorY());
        gb.print(F("FIND "));
        if( game_status[STATUS_MAIN] == 0 ){
          gb.print(F("A  BETTER LIFE"));
        }else if( game_status[STATUS_MAIN] <= 2 ){
          gb.print(F("GIRL AT CATPAW"));
        }else if( game_status[STATUS_MAIN] <= 8 ){
          gb.print(F(" GIRL'S  HOUSE"));
        }else if( game_status[STATUS_MAIN] <= 9 ){
          gb.print(F(" GIRL'S FATHER"));
        }else if( game_status[STATUS_MAIN] <= 12 ){
          gb.print(F(" A   BIG  SHIP"));
        }else if( game_status[STATUS_MAIN] <= 15 ){
          gb.print(F("   THE   NURSE"));
        }else if( game_status[STATUS_MAIN] <= 18 ){
          gb.print(F(" THE  HOSPITAL"));
        }else if( game_status[STATUS_MAIN] <= 26 ){
          gb.print(F("THE MAD DOCTOR"));
        }else if( game_status[STATUS_MAIN] <= 27 ){
          gb.print(F("   THE   TOWER"));
        }else if( game_status[STATUS_MAIN] <= 35 ){
          gb.print(F("  THE   LEADER"));
        }
        gb.setCursor(SCREEN_WIDTH/2-2*6, SCREEN_HEIGHT/2);
        gb.println(F("BACK"));
        if( meta_mode == WORLD ){
          gb.setCursor(SCREEN_WIDTH/2-2*6, gb.getCursorY());
          gb.print(F("SAVE"));
        }else{ //We are in a dungeon
          //Print the floor of the dungeon the player is on
          gb.setCursor(SCREEN_WIDTH/2-7*6, SCREEN_HEIGHT-16);
          gb.print(F("FLOOR "));
          if( dungeon_level < 9 ) gb.print(0);
          gb.print( dungeon_level + 1 );
          gb.print(F(" OF "));
          if( pgm_read_byte(&(dungeons[dungeonid].size)) < 10 ) gb.print(0);
          gb.print( pgm_read_byte(&(dungeons[dungeonid].size)) );
        }
        gb.setCursor(SCREEN_WIDTH/2-3*6, SCREEN_HEIGHT/2);
        if(menu_selection == 1){
          gb.setCursor(SCREEN_WIDTH/2-3*6, SCREEN_HEIGHT/2 + 8);          
        }
        gb.print(F("\20"));
        print_progress();
        if(meta_mode == WORLD && gb.justPressed(UP_BUTTON)){
          menu_selection--;
          if( menu_selection == 255 ) menu_selection = 1;
        }
        else if(meta_mode == WORLD && gb.justPressed(DOWN_BUTTON)){
          menu_selection++;
          menu_selection%=2;
        }
        else if(gb.justPressed(A_BUTTON)){
          if(menu_selection == 1){
            save_game();
          }
          mode = meta_mode;
        }
        break;
      case GAME_OVER:
        gb.fillRect(0,0,128,64);
        gb.setCursor(SCREEN_WIDTH/2-4*6, gb.getCursorY());
        gb.print(F("GAME OVER"));
        gb.setCursor(SCREEN_WIDTH/2-3*6, SCREEN_HEIGHT/2);
        gb.print(F("\20LOAD"));
        if(gb.justPressed(A_BUTTON)){
          restore_game();
        }
        break;
      case YOU_WIN:
        gb.fillRect(0,0,128,64);
        gb.setCursor(SCREEN_WIDTH/2-3*6-3, gb.getCursorY());
        gb.print(F("YOU WIN"));
        print_progress();
    }

    if( (mode == WORLD || mode == DUNGEON) && gb.justPressed(B_BUTTON) ){ // Was C button for Gamebuino, but A and B aren't used outside battles
      menu_selection = 0;
      meta_mode = mode;
      mode = PAUSE_MENU;
    }
    gb.display();
  }
}

bool persistence = false;

//Scamp: Level 1
//Ruffian: Level 2
//Scoundrel: Level 4
//Thief (or maybe Shadow) recruits you on his quest
#include "battle.h"
#include "dungeon.h"

uint8_t meta_mode = WORLD; //Mode that we came from, either WORLD or DUNGEON

struct enemy{
  int8_t lvl;
  uint8_t spd;
  uint8_t img;
  uint8_t nme;
};

struct enemy enemy_buffer[3];//Store data for enemies to battle


struct min_enemy{
  int8_t lvl;
  uint8_t spd;
  uint8_t img;
};

//This is not efficient, because there is always an
//entry for the enemy name even though every enemy has
//a unique name and I don't want two different enemies 
//with the same name.  However, the way I copy names into
//the buffer uses a generic approach that always copies from an
//array of 8-char strings in progmem, and I don't want to change all
//that code for a special case with the name!  Therefore, this
//approach is a bit inefficient because refactoring is a huge chore.
const struct min_enemy enemies[] PROGMEM = {
  {1,5,1}, //RAT
  {1,4,0}, //SCAMP
  {2,3,0}, //RUFFIAN
  {4,2,0}, //THUG
  {4,5,1}, //BIG RAT
  {5,4,0}, //PATRON
  {5,5,2}, //BOUNCER
  {10,5,0}, //SLAVER (boss)
  {8,6,1}, //SEA RAT
  {9,4,4}, //SWABBIE
  {11,3,4}, //SAILOR
  {10,5,4}, //SKIPPER
  {20,4,4}, //CAPTAIN (boss)
  {14,7,1}, //BAD RAT
  {15,5,0}, //WATCHER
  {19,3,2}, //BRUISER (boss/enemy)
  {24,2,2}, //MUSCLER (boss/enemy)
  //{25,3,2},  //Remove OVERMAN
  {25,4,5}, //DOCTOR
  {26,5,0}, //PATIENT
  {26,6,0}, //SUBJECT
  {26,7,0}, //MUTANT
  {32,8,5}, //MADMAN (boss)
  {30,7,1}, //WOW RAT
  {34,3,0}, //SNOB
  {37,2,0}, //RICHMAN
  {42,8,1}, //MAX RAT (boss)
  {40,4,2}, //GUARD
  {45,2,6}, //GOLEM
  {42,5,0}, //OFFICER
  {50,8,7}, //LEADER (boss)
  {1,2,3}, //CRAB
  {10,7,0}, //SHADOW (boss)
};

const char enemy_names[][8] PROGMEM = {
"RAT",
"SCAMP",
"RUFFIAN",
"THUG",
"BIG RAT",
"PATRON",
"BOUNCER",
"SLAVER",
"SEA RAT",
"SWABBIE",
"SAILOR",
"SKIPPER",
"CAPTAIN",
"BAD RAT",
"WATCHER",
"BRUISER",
"MUSCLER",
//"OVERMAN",
"DOCTOR",
"PATIENT",
"SUBJECT",
"MUTANT",
"MADMAN",
"WOW RAT",
"SNOB",
"RICHMAN",
"MAX RAT",
"GUARD",
"GOLEM",
"OFFICER",
"LEADER",
"CRAB",
"SHADOW"
};

const byte enemybmps[] PROGMEM = {
  // Scoundrel
0b01000000,
0b01111000,
0b00101100,
0b00100100,
0b00101010,
0b00011101,
0b00100001,
0b00100101,
0b00100101,
0b01010110,
0b10010100,
0b00011100,
0b00010100,
0b00110010,
0b11100010,
0b10000011,
                   // Rat
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000001,
0b01010010,
0b01111001,
0b11111110,
0b01111100,
                   // Bouncer
0b00000000,
0b00000000,
0b00011000,
0b00011000,
0b00111100,
0b01111110,
0b11111111,
0b11110111,
0b11111001,
0b10001110,
0b01111110,
0b01111110,
0b01100110,
0b01100110,
0b00100100,
0b01100110,
                   // Crab
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b01000010,
0b10000001,
0b11000011,
0b10000001,
0b10111101,
0b01000010,
0b10101001,
0b10000001,
0b01111110,
0b00100100,
                  //Sailor
0b00111100,
0b01111100,
0b10100100,
0b00100100,
0b00111100,
0b01111110,
0b11000011,
0b11111111,
0b10100101,
0b10111101,
0b00100100,
0b00111100,
0b01111110,
0b01000010,
0b01000010,
0b11000011,
                   //Doctor
0b00011000,
0b00111100,
0b00111100,
0b00100100,
0b00011000,
0b11111111,
0b10000001,
0b11100111,
0b10100101,
0b10100101,
0b00100100,
0b00100100,
0b00100100,
0b00111100,
0b00100100,
0b01100110,
                   //Golem
0b11000011,
0b10000001,
0b10111101,
0b10100101,
0b10111101,
0b11111111,
0b01011010,
0b01100110,
0b01000010,
0b01111110,
0b00111100,
0b01111100,
0b01101100,
0b01101100,
0b01101110,
0b11100110,
                     //Leader
0b00111100,
0b00111100,
0b01111110,
0b00100100,
0b00100100,
0b01111110,
0b11111111,
0b10111101,
0b10111101,
0b10111101,
0b00111100,
0b00111100,
0b01111110,
0b01000010,
0b01000010,
0b11000011,
};

const uint8_t world_spawns[4][4][3] PROGMEM = {
  {
    // Top far left (Rich District)
    {
      // Wow Rat, Snob, Richman
      WOW_RAT,SNOB,RICHMAN
    },
    // Top center left (Right Rich District)
    {
      WOW_RAT,SNOB,RICHMAN
    },
    // Top center right (Top Left Medical District
    {
      WOW_RAT,PATIENT,SUBJECT
    },
    // Top far right (Top Right Medical District)
    {
      WOW_RAT,SUBJECT,MUTANT
    }
  },
  {
    // Mid top far left (Red Light District)
    {
      RAT,PATRON,BOUNCER
    },
    // Mid top center left (TOWER DISTRICT)
    {
      BAD_RAT,BRUISER,MUSCLER //Problem here is that a tiny bit of this area is in a much earlier place
    },
    // Mid top center right (Bottom Left Medical District)
    {
      WATCHER,BRUISER,DOCTOR
    },
    // Mid top far right (Bottom Right Medical District)
    {
      MUSCLER,DOCTOR,PATIENT
    }
  },
  {
    // Mid bottom far left (Left Factory District)
    {
      // Rat, Ruffian, Thug
      RAT,RUFFIAN,THUG
    },
    // Mid bottom center left (Right Factory District)
    {
      // Big Rat, Ruffian, Thug
      BIG_RAT,RUFFIAN,THUG
    },
    // Mid bottom center right (Top Left Dock District)
    {
      // SEA_RAT, SKIPPER, WATCHER
      SEA_RAT,SKIPPER,WATCHER
    },
    // Mid bottom far right (Top Right Dock District)
    {
      // Sea Rat, Sailor, Skipper
      SEA_RAT,SAILOR,SKIPPER
    }
  },
  {
    // Bottom far left (Left Sewer District)
    {
      // Rat, Scamp, Thug
      RAT,SCAMP,RUFFIAN
    },
    // Bottom center left (Right Sewer District)
    {
      // Rat, Scamp, Ruffian
      RAT,CRAB,SCAMP
    },
    // Bottom center right (Bottom Left Dock District)
    {
      // Sea rat, Swabbie, Sailor
      SEA_RAT,SWABBIE,SAILOR
    },
    // Bottom far right (Bottom Right Dock District)
    {
      // Sea Rat, Sailor, Skipper
      SEA_RAT,SAILOR,SKIPPER
    }
  }
};

//Stats:
//Level (determines max health + base damage)
//Health
//Speed

const char player_names[][8] PROGMEM = {
"MUDLARK",
"SHADOW",
"NURSE",
"GIRL", //Not actually playable characters, but I needed the text somewhere....
"FATHER",
"",
"MAN",
};

struct character party[3] = {
  {1,20,4,0,0,0,0}, // Mudlark
  {0,160,3,0,0,0,0}, // Shadow
  {0,200,4,0,0,0,0} // Nurse
};

uint8_t shadow_stealth_bonus = 0;
int8_t nurse_protect_bonus = -1;// -1 means she is not protecting anyone

//Abilities:
//Mudlark: Rally, scavenge
//Nurse: Heal one, heal all
//Thief: Sneak, Speed up

//TODO: More effects, more items
#define HEAL 0

#define ITEM_FRUIT 0
#define ITEM_BREAD 1
#define ITEM_MEAT 2
#define ITEM_TONIC 3
#define ITEM_TEA 4
#define ITEM_LIQUOR 5

const char item_names[][8] PROGMEM = {
  "FRUIT",  // 10% HP
  "BREAD",  // 15% HP
  "MEAT",    // 1 damage increase 
  "TONIC",  // 50% HP
  "TEA",    // 1 SP
  "LIQUOR", // 1 damage resist       
};

#define INVENTORY_MAX 8

unsigned char inventory[6] = {2,1,0,0,1,0};//Each element == how much of each item

uint8_t next_combat = 32;

void try_combat(){
  next_combat--;
  if( next_combat == 0 ){
    meta_mode = mode; //Remember the mode we came from
    mode = TO_COMBAT;
    transition = -SCREEN_HEIGHT/2;
    persistence = true;
    next_combat = random(192)+64;//32-256 steps
  }
}

const char menu_text[][8] PROGMEM = {
  "FLAIL",
  "RALLY",
  "SEARCH",
  "OTHER",
  "STRIKE",
  "VANISH", // Perhaps SNEAK or LURK?
  "HASTE",
  "OTHER",
  "HP ONE",
  "HP ALL",
  "PROTECT",
  "OTHER",
  "FOOD",
  "DRINK",
  "RUN",
  "",//TODO: optimize these out
  };

const char combat_text[][8] PROGMEM = {
  " HIT ",
  "\n FOR ",
  " DAMAGE",
  " HEAL ",
  " FALLS",
  "YOU WIN",
  " XP GET",
  "TRAPPED",
  " SPD UP",
  " FOUND\n",
  "\n GIVE ",
  "TO WHO?",
  "PROTECT",
  " DMG UP",
  " DEF UP",
  "ALL",
  "BAG MAX",
  " USES ",
};

#define MUDLARK_MENU 0
#define SHADOW_MENU 1
#define NURSE_MENU 2
#define SECONDARY_MENU 3
#define ENEMY_MENU 4
#define ALLY_MENU 5
#define FOOD_MENU 6
#define DRINK_MENU 7
// More...

#define PRECOMBAT -1
#define MUDLARK 0
#define SHADOW 1
#define NURSE 2
#define ENEMY1 3
#define ENEMY2 4
#define ENEMY3 5
#define MESSAGE 6
#define VICTORY 7
#define POSTCOMBAT 8 // Used for allocating xp to allies
#define DEFEAT 9

char combat_mode = PRECOMBAT;
byte menu_selection = 0;
byte combat_selection = 0;
char combat_buffer[8] = "\0\0\0\0\0\0\0";
char combat_message[64];//Max of 64 characters in message, which should be plenty
int8_t combat_status[6] = {0,0,0,0,0,0};//Determines who moves next
uint8_t combat_xp = 0;
uint8_t is_boss = 0;

uint16_t enemy_health[3];

uint16_t calculate_damage(uint8_t lvl){
  return lvl*10/8;
}

void load_enemy_data(uint8_t index, uint8_t slot){
  uint8_t en_ind;
  const struct min_enemy* en;
  if( meta_mode == WORLD ){
    //Use dudex and dudey to determine which pool to spawn enemies from
    en_ind = pgm_read_byte(&(world_spawns[dudey/8/16][dudex/8/16][index]));
    en = &(enemies[en_ind]);
  }else if( meta_mode == DUNGEON ){
    //Use dungeon data to determine pool to spawn enemies from
    en_ind = pgm_read_byte(&(dungeons[dungeonid].spawns[index]));
    en = &(enemies[en_ind]);
  }else{
    //If meta_mode actually contains the boss data
    //It's always the center the first time it's called
    //if( slot == 1 ){ //If it's the center enemy
      is_boss = 1;
      en_ind = meta_mode;
      en = &(enemies[meta_mode]);
      meta_mode = DUNGEON;//This will be used to return to the dungeon now (bosses can only be in dungeons)
    //}
    /*else{
      //If it's not the center enemy, we don't want it
      enemy_buffer[slot].lvl = -1;
      combat_xp++;//Compensate for loss in combat_xp
      return;//Skip setting enemy_buffer
    }*/
  }
  enemy_buffer[slot].lvl = pgm_read_byte(&(en->lvl));
  enemy_buffer[slot].spd = pgm_read_byte(&(en->spd));
  enemy_buffer[slot].img = pgm_read_byte(&(en->img));
  enemy_buffer[slot].nme = en_ind;
}

void gen_enemies(){
  uint8_t enemy_index = 0;
  combat_xp = 0;
  is_boss = 0;// Assume it is not a boss until we generate the first enemy
  //Always generate center enemy
  enemy_index = random(3);
  load_enemy_data(enemy_index,1);
  combat_xp+=enemy_buffer[1].lvl;
  enemy_health[1] = enemy_buffer[1].lvl*10;
  //50% chance of left enemy
  if( random(2) == 0 ){
    enemy_index = random(3);
    load_enemy_data(enemy_index,0);
    combat_xp+=enemy_buffer[0].lvl;
    enemy_health[0] = enemy_buffer[0].lvl*10;
  }else{
    enemy_buffer[0].lvl = -1;
  }
  //50% chance of right enemy
  if( random(2) == 0 ){
    enemy_index = random(3);
    load_enemy_data(enemy_index,2);
    combat_xp+=enemy_buffer[2].lvl;
    enemy_health[2] = enemy_buffer[2].lvl*10;
  }else{
    enemy_buffer[2].lvl = -1;
  }
}

void copy_to_buffer(byte index, const char arr[][8]){
  for(byte i = 0; i < 7; i++){
    combat_buffer[i] = pgm_read_byte(((byte*)&arr[index])+i);
  }
  //Final null byte was defined initially
}

uint8_t append_to_msg_buffer(uint8_t index, const char arr[][8], uint8_t offset){
  for(byte i = 0; i < 8; i++){
    byte tmp = pgm_read_byte(((byte*)&arr[index])+i);
    if( tmp != '\0' ){
      combat_message[offset+i] = tmp;
    }else{
      return offset+i;
    }
  }
  return offset+8; // all 8 chars, no terminator
}

#define PL2EN 0
#define EN2PL 1
#define EFALL 2
#define PWIN 3
#define PHEAL 4
#define PSPEED 5
#define PITEM 6
#define PROTECT 7
#define PDAMAGE 8
#define PDEFENSE 9
#define HEALALL 10
#define PFALL 11

//TODO: space optimize this
void copy_action_to_msg_buffer(uint8_t source, uint8_t dest, uint16_t amount, uint8_t type){
  uint8_t offset = 0;
  if( type == PL2EN || type == PHEAL || type == PSPEED || type == PITEM || type == PDAMAGE || type == PDEFENSE )
    offset = append_to_msg_buffer( source, player_names, offset );
  else if( type == EN2PL || type == PROTECT ){
    if( type == PROTECT ){
      //NURSE PROTECT <PARTY MEMBER>
      //offset = append_to_msg_buffer( NURSE, player_names, offset );
      //combat_message[offset++] = ' ';
      offset = append_to_msg_buffer( 12, combat_text, offset );
      combat_message[offset++] = ' ';
      offset = append_to_msg_buffer( dest, player_names, offset );
      combat_message[offset++] = '\n';
      combat_message[offset++] = ' ';
      dest = NURSE;// Nurse takes the damage instead
    }
    offset = append_to_msg_buffer( source, enemy_names, offset );
  }else if( type == EFALL ){
    offset = append_to_msg_buffer( source, enemy_names, offset );
    offset = append_to_msg_buffer( 4, combat_text, offset );
    combat_message[offset] = '\0';
    return;
  }else if( type == PFALL ){
    offset = append_to_msg_buffer( source, player_names, offset );
    offset = append_to_msg_buffer( 4, combat_text, offset );
    combat_message[offset] = '\0';
    return;
  }else if( type == PWIN ){
    offset = append_to_msg_buffer( 5, combat_text, offset );
    combat_message[offset++] = '\n';
    combat_message[offset++] = ' ';
    combat_message[offset++] = amount/100+'0';
    combat_message[offset++] = amount%100/10+'0';
    combat_message[offset++] = amount%100%10+'0';
    offset = append_to_msg_buffer( 6, combat_text, offset );
    // If we have more than one party member, ask who to give xp to
    if( party[SHADOW].level != 0 || party[NURSE].level != 0 ){ 
      offset = append_to_msg_buffer( 10, combat_text, offset );
      offset = append_to_msg_buffer( 11, combat_text, offset );
    }
    combat_message[offset] = '\0';
    return;
  }else if( type == HEALALL ){
    offset = append_to_msg_buffer( 15, combat_text, offset );
  }
  if( type == PHEAL || type == HEALALL ){
    offset = append_to_msg_buffer( 3, combat_text, offset );
  }else if( type == PSPEED ){
    offset = append_to_msg_buffer( 8, combat_text, offset );
  }else if( type == PDAMAGE ){
    offset = append_to_msg_buffer( 13, combat_text, offset );
  }else if( type == PDEFENSE ){
    offset = append_to_msg_buffer( 14, combat_text, offset );
  }else if( type == PITEM ){
    offset = append_to_msg_buffer( 9, combat_text, offset );
    combat_message[offset++] = ' ';
    offset = append_to_msg_buffer( amount, item_names, offset );
    //Check if player has more than max of item in inventory.
    //If so, we must say that the mudlark consumes it immediately.
    if( inventory[amount] > INVENTORY_MAX ){
      combat_message[offset++] = '\n';
      combat_message[offset++] = ' ';
      offset = append_to_msg_buffer( 16, combat_text, offset );
      combat_message[offset++] = '\n';
      combat_message[offset++] = ' ';
      offset = append_to_msg_buffer( 0, player_names, offset ); //MUDLARK
      offset = append_to_msg_buffer( 17, combat_text, offset ); // USES 
      offset = append_to_msg_buffer( amount, item_names, offset );//<item>
    }
  }else{
    offset = append_to_msg_buffer( 0, combat_text, offset );
  }
  if( type == PL2EN )
    offset = append_to_msg_buffer( dest, enemy_names, offset );
  else if( type == EN2PL || type == PROTECT )
    offset = append_to_msg_buffer( dest, player_names, offset );
  if( type != PSPEED && type != PDAMAGE && type != PDEFENSE && type != PITEM ){
    offset = append_to_msg_buffer( 1, combat_text, offset );
    combat_message[offset++] = amount/100+'0';
    combat_message[offset++] = amount%100/10+'0';
    combat_message[offset++] = amount%100%10+'0';
    offset = append_to_msg_buffer( 2, combat_text, offset );
    //If shadow was struck and had an active bonus, remove it and say so
    if( type == EN2PL && dest == SHADOW && shadow_stealth_bonus > 0 )
    {
      shadow_stealth_bonus = 0;
      combat_message[offset++] = '\n';
      combat_message[offset++] = ' ';
      offset = append_to_msg_buffer( SHADOW, player_names, offset );
      offset = append_to_msg_buffer( 9, combat_text, offset );
    }
  }
  combat_message[offset] = '\0';
}

void draw_menu(byte index){
  gb.setCursor(0, SCREEN_HEIGHT/2-8);
  uint8_t offset = 0;
  for(byte i = 0; i < 4; i++){
    gb.setCursor(1, gb.getCursorY() + 8);
    if( combat_selection == i ) gb.print(F("\20"));
    gb.setCursor(6, gb.getCursorY());
    combat_message[0] = '\0';//If we don't copy anything in, print nothing
    if( index <= SECONDARY_MENU ){
      offset = append_to_msg_buffer(index*4+i,menu_text,0);//Ordinary menus with fixed text
    }else if(index == ENEMY_MENU){//Dynamically generated menus with text for allies/enemies
      //Menu for selecting an enemy to attack
      /*
          0 1 2 Result
        0 Y ? ? 0
        0 N Y ? 1
        0 N N Y 2
        1 Y Y ? 1
        1 N Y Y 2
        2 Y Y Y 2
      */
      byte yeses = 0;
      for( byte j = 0; j < 3; j++ )
      {
        if( enemy_buffer[j].lvl != -1 ){
          yeses++;
          if( yeses > i ){
            offset = append_to_msg_buffer(enemy_buffer[j].nme,enemy_names,0);
            break;
          }
        }
      }
    }else if( index == FOOD_MENU || index == DRINK_MENU ){
      uint8_t item = 0;
      uint8_t j = index == FOOD_MENU ? 0 : 3;
      for( ; j < (index == FOOD_MENU ? INVENTORY_SIZE/2: INVENTORY_SIZE); j++ ){
        if( inventory[j] > 0 ) item++;
        if( item == i+1 ){
          offset = append_to_msg_buffer(j,item_names,0);
          combat_message[offset++] = 'x';
          combat_message[offset++] = inventory[j]/10+'0';
          combat_message[offset++] = inventory[j]%10+'0';
          break;
        }
      }
    }else{  //Allies menu (ALLY_MENU)
      if( i < 3 && party[i].level != 0 ){
        offset = append_to_msg_buffer(i,player_names,0);
      }
    }
    combat_message[offset] = '\0';
    gb.print(combat_message);
  }
}

void give_xp(){
  //Give xp to chosen character
  party[combat_selection].xp+=combat_xp;
  if( party[combat_selection].xp >= party[combat_selection].level*2 ){
    party[combat_selection].xp -= (party[combat_selection].level*2);// Keep previous xp
    party[combat_selection].health += ((party[combat_selection].level+1)*20)-(party[combat_selection].level*20);// Give enough health that full health stays at full health
    party[combat_selection].level++;//Level up!
  }
  combat_mode = PRECOMBAT;
  mode = meta_mode; //Switch back to previous mode, either dungeon or overworld
}

void do_combat_step(){
  uint8_t i;
  for( i = 0; i < 3; i++ ){
    // If a party member is present, add their speed
    if( party[i].level != 0 ){
      combat_status[i] += party[i].speed + party[i].bonus_speed;
    }
    // If an enemy is present, add their speed
    if( enemy_buffer[i].lvl != -1 ){
      combat_status[i+ENEMY1] += enemy_buffer[i].spd;
    }
  }

  uint8_t maxi = 0;
  for(i = 0; i < 6; i++){
    if( combat_status[i] > combat_status[maxi] ) maxi = i;
  }
  //If nurse was protecting anyone, reset this as her turn starts.
  if( maxi == NURSE ) nurse_protect_bonus = -1;
  combat_mode = maxi;
  combat_selection = 0;//Force the first item to be selected by default
  menu_selection = maxi;//Only relevant for player characters?
  combat_status[maxi] = 0;//Reset that unit back to starting value
}

void do_combat(){
  if( combat_mode == PRECOMBAT ){
    gen_enemies();
    for(uint8_t i = 0; i < 6; i++){
      combat_status[i] = -1;//Reset combat status
      // Reset party members' temporary bonuses
      if( i < 3 ){
        party[i].bonus_speed = 0;
        party[i].bonus_damage = 0;
        party[i].bonus_defense = 0;
      }
    }
    shadow_stealth_bonus = 0;
    nurse_protect_bonus = -1;
    do_combat_step();
  }

  // Draw background rectangles on top and bottom of screen
  gb.fillRect(0,0,SCREEN_WIDTH,8);
  gb.fillRect(0,SCREEN_HEIGHT/2-1,SCREEN_WIDTH,SCREEN_HEIGHT/2+1);
  
  //Get enemy type
  //graphics hardcoded for now
  gb.setCursor(0, 0);

  if(enemy_buffer[0].lvl != -1){
    //gb.cursor_x = 0;
    copy_to_buffer(enemy_buffer[0].nme,enemy_names);
    gb.print(combat_buffer);
    gb.drawSlowXYBitmap(SCREEN_WIDTH/4-4,SCREEN_HEIGHT/8,&enemybmps[enemy_buffer[0].img*16],8,16);
    gb.drawRect(SCREEN_WIDTH/4-4-4, SCREEN_HEIGHT/8 + 18, 16, 3);
    gb.drawFastHLine(SCREEN_WIDTH/4-4-4, SCREEN_HEIGHT/8 + 19, (16*enemy_health[0])/(enemy_buffer[0].lvl*10) );
  }
  if(enemy_buffer[1].lvl != -1){
    gb.setCursor(SCREEN_WIDTH/2-(3*6)-3, 0);
    copy_to_buffer(enemy_buffer[1].nme,enemy_names);
    gb.print(combat_buffer);
    gb.drawSlowXYBitmap(SCREEN_WIDTH/2-4,SCREEN_HEIGHT/8,&enemybmps[enemy_buffer[1].img*16],8,16);
    gb.drawRect(SCREEN_WIDTH/2-4-4, SCREEN_HEIGHT/8 + 18, 16, 3);
    gb.drawFastHLine(SCREEN_WIDTH/2-4-4, SCREEN_HEIGHT/8 + 19, (16*enemy_health[1])/(enemy_buffer[1].lvl*10) );
  }
  if(enemy_buffer[2].lvl != -1){
    gb.setCursor(SCREEN_WIDTH-(7*6), 0);
    copy_to_buffer(enemy_buffer[2].nme,enemy_names);
    gb.print(combat_buffer);
    gb.drawSlowXYBitmap(SCREEN_WIDTH/4*3-4,SCREEN_HEIGHT/8,&enemybmps[enemy_buffer[2].img*16],8,16);
    gb.drawRect(SCREEN_WIDTH/4*3-4-4, SCREEN_HEIGHT/8 + 18, 16, 3);
    gb.drawFastHLine(SCREEN_WIDTH/4*3-4-4, SCREEN_HEIGHT/8 + 19, (16*enemy_health[2])/(enemy_buffer[2].lvl*10) );
  }

  //If a message is displayed (dismissing the message steps combat forward)
  if( combat_mode == MESSAGE || combat_mode == VICTORY || combat_mode == DEFEAT ){
     gb.setCursor(6, SCREEN_HEIGHT/2);
     gb.print(combat_message);
     if(gb.justPressed(A_BUTTON)){
        byte step_forward = 1;
        //gb.sound.playTick(); // TODO SOUND
        if( combat_mode == VICTORY ){
          //If there is more than one party member, ask who to give xp to
          if( party[SHADOW].level > 0 ){
            combat_mode = POSTCOMBAT;
            menu_selection = ALLY_MENU;
          }else{
            combat_selection = 0;// Select mudlark to give xp to
            give_xp();
          }
          return;
        }
        if( combat_mode == DEFEAT ){
          combat_mode = PRECOMBAT;
          mode = GAME_OVER;
          return;
        }
        //First check if all enemies have died
        if( enemy_buffer[0].lvl == -1 && enemy_buffer[1].lvl == -1 && enemy_buffer[2].lvl == -1 ){
          step_forward = 0;
          combat_mode = VICTORY;
          copy_action_to_msg_buffer(0,0,combat_xp, PWIN);
        }
        //Second do a check whether anyone has died
        for( byte i = 0; i < 3; i++ ){
          if( enemy_buffer[i].lvl != -1 && enemy_health[i] == 0 ){
            copy_action_to_msg_buffer(enemy_buffer[i].nme,0,0, EFALL);
            combat_status[ENEMY1+i] = -1;
            enemy_buffer[i].lvl = -1;//This will break center enemy TODO: support absence of center enemy
            step_forward = 0;
            break;
          }
          //Loss condition---if any party member is dead
          if( party[i].health == 0 ){
            copy_action_to_msg_buffer(i,0,0,PFALL);
            step_forward = 0;
            combat_mode = DEFEAT;
            break;
          }
        }
        if( step_forward )
          do_combat_step();
     }
  }

  //If it is the player's turn (one of 3 party members or post-combat xp allocation)
  else if( (combat_mode >= MUDLARK && combat_mode <= NURSE) || combat_mode == POSTCOMBAT ){

    if( combat_mode >= MUDLARK && combat_mode <= NURSE ){
      gb.drawRect(SCREEN_WIDTH/2, SCREEN_HEIGHT/2+1, SCREEN_WIDTH/2-2, 3, BLACK);
      gb.drawLine(SCREEN_WIDTH/2, SCREEN_HEIGHT/2+2, SCREEN_WIDTH/2+((SCREEN_WIDTH/2-4)*party[combat_mode].health)/(party[combat_mode].level*20), SCREEN_HEIGHT/2+2,BLACK);

      gb.setCursor(SCREEN_WIDTH-(8*6+1), SCREEN_HEIGHT/2+6);
      copy_to_buffer(combat_mode,player_names);
      gb.print(combat_buffer);
      gb.setCursor(SCREEN_WIDTH-(8*6+1), SCREEN_HEIGHT/2+14);
      gb.print(party[combat_mode].health);
      gb.print(F("/"));
      gb.print(party[combat_mode].level*20);

      gb.setCursor(SCREEN_WIDTH-(8*6+1), SCREEN_HEIGHT/2+22);
      gb.print(F("XP"));
      gb.print(party[combat_mode].xp);
      gb.print(F("L"));
      gb.print(party[combat_mode].level);
    }

    draw_menu(menu_selection);
    
    if(gb.justPressed(UP_BUTTON)){
      //gb.sound.playTick(); // TODO SOUND
      combat_selection--;
    }else if(gb.justPressed(DOWN_BUTTON)){
      //gb.sound.playTick(); // TODO SOUND
      combat_selection++;
    }else if(gb.justPressed(A_BUTTON)){
      //gb.sound.playOK(); // TODO SOUND
      // Flail, Strike
      if( (menu_selection == MUDLARK_MENU || menu_selection == SHADOW_MENU) && combat_selection == 0){
        menu_selection = ENEMY_MENU;
        combat_selection = 0;
      }else if( menu_selection == NURSE_MENU && combat_selection == 0){
        //Nurse's HEAL ONE ability
        //Signal that we are going to heal, not protect, by resetting bonus (in case player entered protect menu and then cancelled)
        nurse_protect_bonus = -1;
        menu_selection = ALLY_MENU;
        combat_selection = 0;
      }else if( menu_selection == MUDLARK_MENU && combat_selection == 1){
        //Mudlark's RALLY ability; select an ally to boost damage of
        menu_selection = ALLY_MENU;
        combat_selection = 0;
      }else if( menu_selection == SHADOW_MENU && combat_selection == 1){
        //Go into stealth, get damage bonus unless hit
        shadow_stealth_bonus++;
        //Say that shadow is sneaking
        combat_message[append_to_msg_buffer( 5, menu_text, 0 )] = 0;
        combat_mode = MESSAGE;
      }else if( menu_selection == NURSE_MENU && combat_selection == 1 ){
        uint16_t healing = party[NURSE].level; // Heal 5% of NURSE's health for everyone
        // Heal an extra amount by (5% of original healing + 1) * bonus_damage
        // Therefore, the nurse's damage bonus is used for healing since she has no attack. 
        healing += (healing/10/2 + 1)*party[NURSE].bonus_damage;
        for( uint8_t i = 0; i < 3; i++ ){
          party[i].health+=healing;
          if( party[i].health > party[i].level*20 ) party[i].health = party[i].level*20;//Cap off healing
        }
        copy_action_to_msg_buffer(0,0,healing, HEALALL);
        // TODO: Display how much all heal by, now that the heal amount is unified by the nurse's level
        combat_mode = MESSAGE;
      }else if( menu_selection == MUDLARK_MENU && combat_selection == 2 ){ //Scavenge
        if( random(100) < 30 ){ //Highest chance to get fruit
          inventory[ITEM_FRUIT]++;
          copy_action_to_msg_buffer(0,0,ITEM_FRUIT, PITEM);
          //If the inventory slot is full, reset to max and consume by mudlark immediately
          if( inventory[ITEM_FRUIT] > INVENTORY_MAX ){
            inventory[ITEM_FRUIT] = INVENTORY_MAX;
            party[MUDLARK].health+=2*party[MUDLARK].level;//Heal 10%
          }
        }else if( random(100) < 30 ){ //Second highest chance to get bread
          inventory[ITEM_BREAD]++;
          copy_action_to_msg_buffer(0,0,ITEM_BREAD, PITEM);
          //If the inventory slot is full, reset to max and consume by mudlark immediately
          if( inventory[ITEM_BREAD] > INVENTORY_MAX ){
            inventory[ITEM_BREAD] = INVENTORY_MAX;
            party[MUDLARK].health+=3*party[MUDLARK].level;//Heal 15%
          }
        }else{ //All the rest of the items have an equal chance
          uint8_t chosen = random(4) + 2;
          inventory[chosen]++;
          copy_action_to_msg_buffer(0,0, chosen, PITEM);
          //If the inventory slot is full, reset to max and consume by mudlark immediately
          if( inventory[chosen] > INVENTORY_MAX ){
            inventory[chosen] = INVENTORY_MAX;
            if( chosen == ITEM_MEAT ){
              party[MUDLARK].bonus_damage+=4; // Meat increases damage
            }else if( chosen == ITEM_TONIC ){
              party[MUDLARK].health+=5*party[MUDLARK].level;//Heal 25%
            }else if( chosen == ITEM_TEA ){
              party[MUDLARK].bonus_speed+=4;//Speed up
            }else if( chosen == ITEM_LIQUOR ){
              party[MUDLARK].bonus_defense+=2;// Liquor increases damage resistance
            }
          }
        }
        if( party[MUDLARK].health > party[MUDLARK].level*20 ) party[MUDLARK].health = party[MUDLARK].level*20;//Cap off healing
        combat_mode = MESSAGE;
      }else if( menu_selection == SHADOW_MENU && combat_selection == 2 ){ //Shadow's speed boost
        party[SHADOW].bonus_speed+=3;//Speed up by 3!
        copy_action_to_msg_buffer(SHADOW,0,1, PSPEED);
        combat_mode = MESSAGE;
      }else if( menu_selection == NURSE_MENU && combat_selection == 2 ){ //Nurse's protect
        nurse_protect_bonus = 4;// Signal that we are going to protect, not heal
        menu_selection = ALLY_MENU;
        combat_selection = 0;
      }else if( (menu_selection >= MUDLARK_MENU && menu_selection <= NURSE_MENU ) && combat_selection == 3 ){
        menu_selection = SECONDARY_MENU;
        combat_selection = 0;
      }else if( menu_selection == SECONDARY_MENU ){
        if( combat_selection == 0 ){ // FOOD
          //If the player has any food to eat, only then show the menu
          if( inventory[0] > 0 || inventory[1] > 0 || inventory[2] > 0 ){
            menu_selection = FOOD_MENU;
            combat_selection = 0;
          }
        }else if( combat_selection == 1 ){ // DRINK
          //If the player has any drinks to drink, only then show the menu
          if( inventory[3] > 0 || inventory[4] > 0 || inventory[5] > 0 ){
            menu_selection = DRINK_MENU;
            combat_selection = 0;
          }
        }else{ // 1 == RUN
          //Only allow running as a 50% chance in non-boss battles
          if( !is_boss && random(2) == 0 ){
            combat_mode = PRECOMBAT;
            mode = meta_mode;
          }else{
            combat_message[append_to_msg_buffer( 7, combat_text, 0 )] = 0;
            combat_mode = MESSAGE;
          }
        }
      }else if( menu_selection == ENEMY_MENU ){
        uint16_t damage = calculate_damage(party[combat_mode].level);// Base damage
        if( combat_mode == MUDLARK ) damage*=2;//Mudlark does double base damage
        if( combat_mode == SHADOW ) damage*=(shadow_stealth_bonus+1);//Rapidly increase stealth bonus
        // Increase damage by (5% of original damage + 1) * bonus_damage
        damage += (damage/10/2 + 1)*party[combat_mode].bonus_damage;
        if( combat_selection == 1 && enemy_buffer[0].lvl == -1 ){
          combat_selection = 2;//Must be the third one
        }else{
          for( ; combat_selection < 3; combat_selection++ ){
            if( enemy_buffer[combat_selection].lvl != -1 ){
              break;  // As soon as we find an existing enemy, that's the one that was selected
            }
          }
        }
        copy_action_to_msg_buffer(combat_mode,enemy_buffer[combat_selection].nme,damage, PL2EN);
        if( damage > enemy_health[combat_selection] ){
          enemy_health[combat_selection] = 0;
        }else{
          enemy_health[combat_selection]-=damage;
        }
        combat_mode = MESSAGE;
      }else if( menu_selection == ALLY_MENU ){
        //Assume mudlark is always slot 0, shadow is always slot 1, and nurse is always slot 2
        //TODO: This assumption will cause problems if we change in the future to have, say, the nurse
        //but not the shadow in the party.
        // Check if this is post-combat xp allocation or nurse ability
        if( combat_mode == POSTCOMBAT ){
          give_xp();
          return;
        }else if( combat_status[MUDLARK] == 0 ){ // If it is the mudlark's turn
          party[combat_selection].bonus_damage+=2;
          copy_action_to_msg_buffer(combat_selection,0,1, PDAMAGE);
          combat_mode = MESSAGE;
        }else if( nurse_protect_bonus == -1 ){ // If we are not trying to select who to protect, and instead heal
          //Nurse's HEAL ONE ability
          //Remember, this assumes that we are not missing a character---if I add the ability to 
          //revive fallen party members (vs an instant game over) then this will need to change...
          uint16_t healing = 3*party[NURSE].level;// Heal 15% of NURSE's health
          // Heal an extra amount by (5% of original healing + 1) * bonus_damage
          // Therefore, the nurse's damage bonus is used for healing since she has no attack. 
          healing += (healing/10/2 + 1)*party[NURSE].bonus_damage;
          party[combat_selection].health += healing;
          if( party[combat_selection].health > party[combat_selection].level*20 ){
            //Cap off health
            party[combat_selection].health = party[combat_selection].level*20;
          }
          copy_action_to_msg_buffer(combat_selection,0,healing, PHEAL);
          combat_mode = MESSAGE;
        }else{ // Protect selected party member
          nurse_protect_bonus = combat_selection;
          uint8_t offset;
          offset = append_to_msg_buffer( 12, combat_text, 0 );
          combat_message[offset++] = ' ';
          offset = append_to_msg_buffer( combat_selection, player_names, offset );
          combat_message[offset++] = '\0';
          combat_mode = MESSAGE;
        }
      }else if( menu_selection == FOOD_MENU || menu_selection == DRINK_MENU ){
        uint8_t item = 0;
        for( uint8_t i = 0; i < INVENTORY_SIZE/2; i++ ){
          if( inventory[menu_selection == FOOD_MENU ? i : i + (INVENTORY_SIZE/2)] != 0 ) item++;
          if( item == combat_selection+1 ){
            item = menu_selection == FOOD_MENU ? i : i + (INVENTORY_SIZE/2);
            break;
          }
        }
        if( item == ITEM_FRUIT ){
          party[combat_mode].health+=2*party[combat_mode].level;//Heal 10%
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,2*party[combat_mode].level, PHEAL);
        }else if( item == ITEM_BREAD ){
          party[combat_mode].health+=3*party[combat_mode].level;//Heal 15%
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,3*party[combat_mode].level, PHEAL);
        }else if( item == ITEM_MEAT ){
          party[combat_mode].bonus_damage+=4; // Meat increases damage
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,1, PDAMAGE);
        }else if( item == ITEM_TONIC ){
          party[combat_mode].health+=5*party[combat_mode].level;//Heal 25%
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,5*party[combat_mode].level, PHEAL);
        }else if( item == ITEM_TEA ){
          party[combat_mode].bonus_speed+=4;//Speed up
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,1, PSPEED);
        }else if( item == ITEM_LIQUOR ){
          party[combat_mode].bonus_defense+=2;// Liquor increases damage resistance
          inventory[item]--;
          copy_action_to_msg_buffer(combat_mode,0,1, PDEFENSE);
        }
        if( party[combat_mode].health > party[combat_mode].level*20 ) party[combat_mode].health = party[combat_mode].level*20;//Cap off healing
        combat_mode = MESSAGE;
      }
    }else if(gb.justPressed(B_BUTTON)){
      //gb.sound.playCancel(); // TODO SOUND
      



      //This is for debugging  TODO: remove
      /*if( menu_selection == MUDLARK_MENU ){
        combat_mode = PRECOMBAT;
        mode = meta_mode;
      }*/




      if( menu_selection == ENEMY_MENU || menu_selection == ALLY_MENU || menu_selection == SECONDARY_MENU ){
        //since the combat mode and menu selection values line up for the party
        //members, we can do this to switch back to the right primary menu
        //depending on which party member's turn it is
        menu_selection = combat_mode; 
        combat_selection = 0;
      }else if( menu_selection == FOOD_MENU || menu_selection == DRINK_MENU ){
        menu_selection = SECONDARY_MENU;
        combat_selection = 0;
      }
    }
    byte mod = 4;
    if( menu_selection == ENEMY_MENU ){
      mod--;
      for( byte i = 0; i < 3; i++ ){
        if( enemy_buffer[i].lvl == -1 ){
          mod--;
        }
      }
    }else if( menu_selection == ALLY_MENU ){
      //Determine how many party members there are so we can
      //wrap the selection cursor around
      mod = 1;
      if( party[SHADOW].level != 0 ) mod++;
      if( party[NURSE].level != 0 ) mod++;
    }else if( menu_selection == SECONDARY_MENU ){
      mod = 3;
    }else if( menu_selection == FOOD_MENU || menu_selection == DRINK_MENU ){
      uint8_t item = 0;
      for( uint8_t i = 0; i < INVENTORY_SIZE/2; i++ ){
        if( inventory[menu_selection == FOOD_MENU ? i : i + (INVENTORY_SIZE/2)] != 0 ) item++;
      }
      if( item < 4 ) mod = item; 
    }
    combat_selection%=mod;
  }

  //If it is an enemy's turn
  else if( combat_mode >= ENEMY1 && combat_mode <= ENEMY3 ){
    int16_t damage = calculate_damage(enemy_buffer[combat_mode-ENEMY1].lvl);
    //Choose which member of the party to attack
    uint8_t member = party[NURSE].level > 0 ? random(3) : (party[SHADOW].level > 0 ? random(2) : 0 );
    if( nurse_protect_bonus == member ){
      //Damage is reduced by 10% of nurse's level if nurse protects
      damage = party[NURSE].level >= damage ? 1 : damage - party[NURSE].level;
      // Reduce damage by an extra amount of (5% of reduced damage + 1) * bonus_defense 
      damage -= (damage/10/2 + 1)*party[NURSE].bonus_defense;
      if( damage <= 0 ) damage = 1;
      copy_action_to_msg_buffer(enemy_buffer[combat_mode-ENEMY1].nme,member,damage, PROTECT);
      member = NURSE; //Nurse takes damage instead
    }else{
      // Reduce damage by (5% of original damage + 1) * bonus_defense 
      damage -= (damage/10/2 + 1)*party[member].bonus_defense;
      if( damage <= 0 ) damage = 1;
      copy_action_to_msg_buffer(enemy_buffer[combat_mode-ENEMY1].nme,member,damage, EN2PL);
    }
    
    if( damage > party[member].health ){
      party[member].health = 0;
    }else{
      party[member].health-=damage;
    }
    combat_mode = MESSAGE;
  }

  //DEBUG TODO: remove
  /*gb.display.cursorX = 0;
  gb.display.cursorY = 6;
  gb.display.println(enemy_buffer[1].nme);
  gb.display.println(enemy_buffer[1].lvl);
  gb.display.print((char)(combat_mode+'0'));
  gb.display.print(' ');
  gb.display.print((char)(mudlark_level+'0'));
  gb.display.print(' ');
  gb.display.print(combat_xp);
  gb.display.cursorX = 0;
  gb.display.cursorY = 12;
  for( uint8_t i = 0; i < 6; i++ ){
    gb.display.print((char)(combat_status[i]+'1'));
  }
  gb.display.cursorX = 0;
  gb.display.cursorY = 18;
  for( uint8_t i = 0; i < 3; i++ ){
    gb.display.print((char)(enemies[i]+'1'));
  }*/

  //gb.drawRect(0,SCREEN_HEIGHT/2-1,SCREEN_WIDTH,SCREEN_HEIGHT/2+1);
  
  
}

#include "dialogue.h"
#include "battle.h"
#include "save.h"

const uint8_t huff_tree[][3] PROGMEM = {
{255,1,24,},
{255,2,15,},
{255,3,4,},
{'E',0,0,},
{255,5,14,},
{255,6,7,},
{'Y',0,0,},
{255,8,13,},
{255,9,12,},
{255,10,11,},
{'J',0,0,},
{'?',0,0,},
{'V',0,0,},
{'G',0,0,},
{'I',0,0,},
{255,16,19,},
{255,17,18,},
{'S',0,0,},
{'O',0,0,},
{255,20,23,},
{255,21,22,},
{'D',0,0,},
{'L',0,0,},
{'H',0,0,},
{255,25,46,},
{255,26,33,},
{255,27,28,},
{'A',0,0,},
{255,29,30,},
{'U',0,0,},
{255,31,32,},
{'C',0,0,},
{'F',0,0,},
{255,34,41,},
{255,35,36,},
{'\x00',0,0,},
{255,37,38,},
{'P',0,0,},
{255,39,40,},
{'K',0,0,},
{'B',0,0,},
{255,42,43,},
{'N',0,0,},
{255,44,45,},
{'M',0,0,},
{'W',0,0,},
{255,47,48,},
{' ',0,0,},
{255,49,50,},
{'T',0,0,},
{255,51,52,},
{'\n',0,0,},
{'R',0,0,},
};

const uint8_t dialogue[] PROGMEM = {
0xe7,0x34,0xd3,0x1f,0x13,0x1a,0x7b,0x30,0x87,0x62,0xe8,0x5c,0x70,0x4c,0x82,0x1a,0x19,0xec,0xc9,0xd,0xcb,0xed,0x53,0x60,0xb9,0xa,0x1f,0x26,0xfc,0x9a,0x3e,0x99,0x22,0x71,0xec,0xe6,0x5b,0xe8,0xae,0xa5,0x46,0x3b,0x7b,0x73,0x8d,0xcb,0x78,0xfd,0x2f,0x73,0x68,0xdc,0xe3,0x54,0xab,0xff,0x64,0x9a,0xa,0x8b,0x16,0x32,0xdb,0xda,0xe9,0x33,0x4e,0xd8,0xe4,0xd5,0xc9,0xed,0xce,0x37,0xcc,0xa8,0xfd,0x39,0xc1,0x34,0x7c,0x6a,0xd6,0x5b,0xed,0xed,0x9,0xae,0x93,0x1b,0x1f,0xac,0x94,0xe7,0x34,0xc6,0x9a,0x2b,0xac,0xbb,0xd9,0x6c,0x32,0xcf,0xb9,0xc2,0xf,0x6e,0x78,0xec,0xd3,0x2a,0x12,0x92,0xb3,0x7,0xf7,0x38,0xdc,0xb7,0x8f,0xd0,0xf3,0xc1,0x51,0x96,0xba,0xf,0x95,0xc,0xf6,0x9f,0xeb,0x76,0x89,0xe3,0xfe,0xc4,0x59,0x7f,0x25,0x63,0x6b,0x68,0x79,0xe0,0xa8,0xd1,0xba,0x12,0xb7,0xb7,0x3c,0x76,0xbc,0xda,0xdd,0x54,0x13,0xda,0x33,0xca,0xcc,0xf8,0x64,0xea,0xd6,0x52,0xbc,0xbb,0x4e,0x7f,0x4e,0xc4,0x59,0x7b,0x5d,0x24,0xed,0x57,0xd4,0xa8,0xc7,0xed,0x33,0xaa,0xd,0x79,0xa0,0x69,0x28,0x80,0x56,0xdc,0xe3,0x4d,0x1d,0x54,0x5f,0xed,0x1a,0xa6,0xc4,0xc3,0x2c,0xfe,0xcc,0x13,0x43,0x5,0xa6,0x14,0x8c,0x59,0xfd,0xb8,0xd3,0x38,0x6b,0x3d,0xab,0xac,0xac,0xcd,0x79,0xf3,0x9d,0xa9,0x3f,0xc0,0xce,0x3f,0xa2,0xcc,0xf6,0xaf,0xf3,0xb1,0x79,0xc7,0xf7,0x2f,0xb3,0x8f,0xe9,0xe3,0x9c,0x7e,0x9c,0xe3,0x4d,0x1d,0x54,0x5f,0xec,0xd3,0x9,0xac,0xbf,0xe7,0xa1,0x16,0x59,0x91,0xf1,0xb9,0x7d,0x8e,0xdc,0x3f,0x38,0xf8,0xc7,0x6f,0x6b,0x89,0xab,0xc9,0xe,0xc1,0x10,0xa6,0x9c,0xf1,0x6a,0xd8,0x8b,0x2a,0x1e,0xbe,0x13,0x44,0xff,0x83,0x67,0xb4,0xeb,0xfd,0x71,0x33,0x4e,0x71,0x4b,0x89,0xa7,0x8e,0x71,0xfc,0xd3,0x2a,0x13,0xda,0x3b,0x73,0x8c,0xc5,0x9a,0xb2,0x7b,0x72,0xee,0x71,0x91,0x65,0xcf,0x43,0xba,0xaf,0x84,0x43,0x2a,0x35,0x79,0x77,0xb5,0x96,0xfc,0x45,0x96,0xba,0x49,0xde,0xd5,0x42,0x26,0xb8,0x9b,0x82,0x75,0x2f,0xd,0x6e,0x62,0xd8,0x7b,0x1e,0x47,0x86,0xb7,0x14,0x29,0xdb,0xd8,0x8b,0x2c,0x76,0xd5,0xe3,0xb9,0xa2,0x8d,0x34,0xe1,0x6f,0x6e,0x79,0x16,0x99,0xd5,0xac,0xf6,0x9e,0x39,0xc7,0xf4,0x57,0x52,0xa2,0x97,0xba,0xe2,0xd4,0x26,0xe7,0x1f,0x5,0x34,0x3d,0x17,0x63,0xb3,0x5,0x7e,0xc,0xf6,0x20,0xec,0x7a,0x12,0xb7,0xb5,0xcb,0xf1,0x96,0x7c,0x45,0x95,0x2e,0x26,0xd,0x60,0x4e,0xf6,0x64,0x48,0xb7,0xe1,0xfc,0x69,0xed,0x58,0xd9,0x68,0xaa,0xa0,0xcd,0xca,0xb4,0x47,0x1a,0xb5,0x96,0xf4,0xcb,0x3f,0xb2,0x9,0xbe,0x39,0x37,0x38,0x27,0xb5,0xf1,0x6e,0xce,0x6c,0x60,0xb5,0x2a,0x68,0x84,0x18,0xb5,0xdc,0xbe,0xdc,0xe3,0x57,0x32,0xf2,0x39,0xd5,0xec,0x76,0xdc,0xe3,0x3c,0x7e,0xba,0xfe,0x9c,0xe6,0x99,0x1c,0xea,0xc6,0x9e,0xd7,0xb8,0xf8,0xce,0x69,0xec,0xc8,0x91,0x6f,0xc3,0xf8,0xd2,0x88,0xe3,0x5f,0x9,0xa3,0x5a,0x5f,0x41,0xed,0x1d,0xb9,0xc6,0x98,0xf8,0x9e,0xce,0xa9,0x51,0xf4,0x37,0x5e,0xe2,0xd4,0x47,0x1a,0x75,0x95,0x99,0xed,0x19,0x4,0xdf,0x1d,0xb9,0x7d,0xb9,0xc6,0xa9,0xb0,0x5c,0x85,0x39,0xe2,0xd5,0xb1,0x16,0x54,0x3d,0x74,0x93,0xb7,0xc7,0x4b,0xf6,0xdc,0xbe,0xdc,0xe3,0x3a,0xa5,0x47,0xd0,0xda,0x73,0x8c,0xe2,0x19,0x98,0xb3,0x72,0xff,0xd9,0xe1,0x31,0x6b,0x61,0xae,0x86,0x51,0xc6,0xba,0x18,0x37,0x38,0xd5,0x36,0xb,0x90,0xf6,0x8b,0x33,0x1e,0xba,0x49,0xde,0xd3,0x9d,0x99,0x96,0x5d,0xaf,0x72,0x52,0xf1,0xaf,0x36,0xb7,0x38,0x6d,0x5e,0xc4,0x59,0x6f,0x8e,0x97,0xed,0xb9,0xc7,0xc5,0xf,0x31,0x75,0x97,0xb7,0x7e,0x49,0xde,0xc4,0x59,0x6a,0xf2,0xec,0x79,0xe0,0xa8,0xf6,0xb2,0xe9,0x9d,0x4e,0x61,0x4e,0x71,0x9d,0x52,0xa3,0xe8,0x6f,0xb1,0xa6,0xb2,0xff,0x9e,0x9c,0xe6,0x9a,0xa6,0xc4,0xc3,0x1a,0x7b,0x4c,0xf0,0xa9,0x4b,0xde,0x3b,0x3c,0x26,0x71,0xec,0xc5,0xb0,0x29,0xa2,0x2e,0x22,0xcb,0x4,0x9a,0x2a,0xc,0xa5,0xee,0x4d,0x1f,0x18,0x8b,0x2f,0x66,0x29,0xd8,0xbd,0xce,0x68,0x53,0x45,0x97,0xf1,0x7f,0x11,0xd9,0xa2,0x18,0x3f,0xed,0x74,0x30,0x68,0xb6,0x59,0xe7,0x1f,0xa1,0xe9,0x96,0x4d,0x66,0xb2,0xf6,0xf8,0x43,0x4e,0xa7,0x38,0xd5,0x36,0xb,0x90,0xf6,0x34,0xc9,0x57,0xc4,0x30,0xec,0x5d,0x23,0x5c,0x68,0xf1,0x55,0xf,0xb5,0xf0,0x99,0x6c,0xd2,0x7b,0x4e,0xbf,0xdc,0xe3,0x54,0xab,0xfa,0x11,0x65,0xae,0x5b,0x27,0xf,0xd2,0xf1,0xae,0x92,0x76,0x2d,0x77,0x2e,0xe7,0x1e,0xdc,0xb7,0x8f,0xee,0x5d,0x32,0xda,0x7f,0xad,0xbb,0xdb,0x9c,0x66,0x88,0x60,0xfd,0x39,0xcd,0x37,0x2d,0xe3,0xf8,0xd3,0xd9,0x91,0x62,0xc7,0xd6,0x49,0x4b,0xc6,0xba,0x49,0xda,0xb8,0xc9,0x2f,0x8f,0x6b,0xc6,0x8f,0x8d,0xf1,0xc,0x25,0x11,0x77,0x38,0x41,0xa3,0xe3,0xdb,0x9c,0x68,0xb7,0x26,0x3f,0x6b,0xcd,0xad,0xd3,0x7e,0x48,0xf4,0x22,0xcb,0x5e,0x6d,0x6e,0xaa,0x9,0xed,0x3a,0xff,0x11,0x65,0xfd,0x37,0xce,0xe0,0x94,0xbd,0xe3,0xb4,0xdf,0x3b,0x82,0x14,0xfb,0x1e,0x8b,0xb4,0xcd,0x11,0x63,0xb1,0x7e,0xdc,0xe3,0x4c,0x7c,0x4a,0x11,0x65,0xa3,0xe3,0xd9,0x81,0x3b,0xea,0x43,0xb1,0x78,0xfa,0x87,0xd8,0xd3,0x31,0x6c,0x28,0x45,0x96,0x8f,0x8c,0xd1,0xc,0x1f,0xf6,0xb2,0xdf,0xae,0x93,0x1b,0x1f,0xad,0x46,0x88,0x66,0xe7,0x1a,0xa0,0xb5,0x34,0x7b,0x45,0x99,0x8f,0x5e,0x6d,0x6f,0xb4,0x30,0xa9,0xa0,0xc4,0x59,0x52,0xcb,0xec,0xd1,0xdb,0x9c,0x6a,0x82,0xd4,0xd1,0xec,0xc1,0x31,0xb0,0x6e,0x70,0x7f,0xd3,0xc7,0xa,0x45,0x99,0x91,0x75,0xe7,0xcf,0xb9,0xc7,0xb4,0xc7,0xc4,0xd1,0xdb,0x9c,0x1f,0xfd,0xa7,0x3,0xa8,0x7c,0x9a,0x79,0x74,0xbe,0x33,0x60,0x9e,0xc7,0x6d,0xce,0x33,0xc5,0x98,0x99,0x67,0xf6,0x8c,0x87,0x62,0xda,0x35,0xd2,0x63,0x63,0xf5,0xa9,0x7b,0xc7,0x66,0x1b,0x3d,0xb9,0xc1,0x34,0xce,0xaa,0x80,0xa6,0x9c,0xf1,0xd8,0xd3,0xda,0x2c,0xbc,0xe3,0xf9,0x39,0x7e,0x4a,0x5e,0x1b,0x32,0xdc,0x37,0x2e,0xe7,0x1e,0xd3,0x99,0x6f,0xc7,0x62,0xf5,0x78,0xfd,0xf,0x45,0xdb,0x9c,0x6e,0x5a,0xb4,0xe6,0x5b,0xf0,0xfd,0x27,0x32,0xdf,0xb5,0xc2,0x97,0x13,0x4d,0xf1,0x7e,0x78,0x4f,0x6b,0xa5,0xc7,0x63,0xc,0xd1,0x66,0x7b,0x74,0x56,0x16,0xd7,0x13,0x23,0x9d,0x54,0x3d,0x17,0x69,0xa2,0xae,0x83,0xb6,0xb2,0xdf,0x4e,0x78,0xb5,0x6c,0x45,0x96,0x9d,0x7f,0xec,0x9c,0xb5,0x54,0x76,0x2f,0x73,0x8b,0xa9,0xce,0x69,0xaf,0x8f,0x87,0x59,0x20,0xc6,0x9e,0xd3,0xc9,0xad,0xcb,0x3e,0xfc,0x72,0x7b,0x38,0x6d,0x54,0xe7,0x1a,0xb9,0x96,0x58,0x4e,0xdf,0x8e,0xf6,0x5,0x47,0xf3,0x44,0x54,0x99,0x65,0xd4,0xbd,0xc5,0xfe,0xdc,0xea,0x83,0x7e,0x39,0x3d,0x92,0x68,0xf8,0x66,0xb8,0x53,0xe7,0x33,0xe4,0xb2,0xd7,0x4f,0x66,0x99,0x51,0x9c,0x7c,0x50,0xf4,0x5d,0x8a,0x8f,0x93,0x7c,0xe6,0x7a,0x1e,0xe7,0x3b,0x56,0xd7,0x41,0x2b,0x8f,0x6e,0x78,0xed,0x7c,0x26,0x8f,0x6a,0xe7,0xda,0xe9,0x4c,0xf4,0x45,0xb8,0x2d,0x86,0x4e,0x56,0x8f,0x6b,0x89,0xaf,0x8f,0x84,0xa7,0x38,0x26,0x8f,0x8c,0x76,0xf6,0xe7,0x8e,0xd7,0xc7,0xc3,0xac,0x90,0x53,0x9e,0x2d,0x59,0x34,0xeb,0xff,0x62,0xc7,0x71,0xd8,0xbd,0xce,0x2e,0xf6,0xaf,0x13,0x56,0xa0,0x0,
};

uint16_t huff_index = 0;
uint8_t huff_tree_index = 0;
uint8_t huff_mask = 0;

uint8_t huff_tree_get(uint8_t item){
  return pgm_read_byte((uint8_t*)&huff_tree[huff_tree_index][item]);
}

void fill_dialogue_buffer(uint16_t seek_index){
  uint8_t tmp, offset = 0;
  while( true ){
    tmp = pgm_read_byte((uint8_t*)&dialogue[huff_index]);
    //Read a single bit
    if( (tmp & huff_mask) == 0 ){
      // If bit was zero, go to left node
      huff_tree_index = huff_tree_get(1);
    }else{
      // If bit was one, go to right node
      huff_tree_index = huff_tree_get(2);
    }
    //If we are at a leaf node, we have a complete code
    tmp = huff_tree_get(0);
    if( tmp != 255 ){
      // If we're still seeking to our desired start location
      if( seek_index > 0 ){
        seek_index--;
      }else{
        combat_message[offset++] = tmp;
        //If the character we just read was a zero byte, we are done loading the buffer
        if( tmp == '\0' ){
          return;
        }
      }
      huff_tree_index = 0;// Return to root
    }
    //Advance to next bit
    huff_mask >>= 1; //Shift mask
    if( huff_mask == 0 ){
      //If mask has read every bit, it's time to shift to the next byte
      huff_index++;
      huff_mask = 0x80;
    }
  }
}

void display_dialogue(uint16_t index, uint8_t len, uint8_t who, const char name_list[][8]){
  meta_mode = mode;
  dialogue_index = index;
  dialogue_remaining = len;
  copy_to_buffer(who,name_list);
  huff_mask = 0x80;
  huff_index = 0;
  huff_tree_index = 0;
  // Seek to index (from start!) and
  // fill dialogue buffer with first set of dialogue
  fill_dialogue_buffer(dialogue_index);
  mode = DIALOGUE;
}

uint16_t dialogue_index;
uint8_t dialogue_remaining;

void step_dialogue(){
  //gb.display.setColor(WHITE);
  gb.fillRect(0,SCREEN_HEIGHT/2-1,SCREEN_WIDTH,SCREEN_HEIGHT/2+1, BLACK);

  gb.setTextColor(WHITE);
  gb.setTextBackground(BLACK);

  gb.setCursor(1, SCREEN_HEIGHT/2);
  //If the first char is a zero, then no character is speaking, so skip this
  //so that narration gets four lines to work with
  if( combat_buffer[0] != 0 ){
    gb.println(combat_buffer);//Display name of speaker, set before this
  }
  gb.setCursor(6, gb.getCursorY());
  gb.print(combat_message);

  gb.setTextColor(BLACK);
  gb.setTextBackground(WHITE);

  //gb.display.setColor(BLACK);
  gb.drawRect(0,SCREEN_HEIGHT/2-1,SCREEN_WIDTH,SCREEN_HEIGHT/2+1);

  if(gb.justPressed(A_BUTTON)){
    //gb.sound.playOK(); // TODO SOUND
    if( dialogue_remaining == 0 ){
      // If dialogue is done, return to world mode
      if( game_status[STATUS_MAIN] == 36 ){
        mode = YOU_WIN;
      }else{
        mode = meta_mode;
      }
    }else{
      // If dialogue is not done, move to next chunk of text
      dialogue_remaining--;
      // This time we fill the dialogue buffer we will start from the end of the
      // previous dialogue
      fill_dialogue_buffer(0);
    }
  }
}


#include "overworld.h"
#include "dungeon.h"
#include "battle.h"
char dungeon_map[16][16];
uint8_t dungeonid = 0;//Used to identify dungeon
uint8_t dungeon_generated = 0;
uint8_t dungeon_level = 0;
int8_t previous_level = -1; //-1 means that the player entered the dungeon

#define DUN_STACK 0 // A stack of rooms
#define DUN_ZIGZAG 1 // Zig-zagging rooms, ascending, starting to the left
#define DUN_ZIGZAGR 2 // Zig-zagging rooms, ascending, starting to the right
#define DUN_SPLIT 3 // Rooms to the left AND right
#define DUN_SPIRAL 4 // Spiral of rooms, ascending, 4 per floor

#define DUN_THEME_CATPAW 0 // Catpaw theme
#define DUN_THEME_BRICK 1 // Brick building theme
#define DUN_THEME_SHIP 2 // Ship theme
#define DUN_THEME_WAREHOUSE 1 // Warehouse theme (NOW MERGED WITH BRICK)
#define DUN_THEME_HOSPITAL 3 // Hospital theme
#define DUN_THEME_HOUSE 0 // House theme (NOW MERGED WITH CATPAW)
#define DUN_THEME_TOWER 3 // The final tower theme (NOW MERGED WITH HOSPITAL)

#define DUN_END_DOOR 0 // Dungeon ends in an exit door 
#define DUN_END_BOSS 1 // Dungeon ends in a boss
#define DUN_END_CHEST 2 // Dungeon has a guaranteed treasure chest at the end

#define DUN_TILE_SIDE 2
#define DUN_TILE_DETAIL1 3
#define DUN_TILE_DETAIL2 4
#define DUN_TILE_DETAIL3 5
#define DUN_TILE_DOOR 6
#define DUN_TILE_STAIRSUP 7
#define DUN_TILE_STAIRSDN 8

const byte common_tiles[] PROGMEM = {
//Door
0b11111111,
0b10000001,
0b10000001,
0b10000001,
0b11000001,
0b10000001,
0b10000001,
0b10000001,
//Stairs up
0b11110000,
0b10011100,
0b10010111,
0b10010101,
0b10010101,
0b11110101,
0b10011101,
0b10000111,
//Stairs down
0b11111111,
0b11111111,
0b10111111,
0b10101111,
0b10101011,
0b10101011,
0b10101011,
0b11111111,
//Wall top
0b10101010,
0b01010101,
0b10101010,
0b01010101,
0b10101010,
0b01010101,
0b10101010,
0b01010101,
};

/*const struct dungeon dungeons[] PROGMEM = {
  {10,24,DUN_THEME_CATPAW,2,{BIG_RAT,PATRON,BOUNCER}},
  {10,24,DUN_THEME_CATPAW,2,{BIG_RAT,PATRON,BOUNCER}}
};*/

const struct dungeon dungeons[] PROGMEM = {
  {10,24,DUN_THEME_CATPAW,3,{BIG_RAT,PATRON,BOUNCER}},//{10,24,DUN_STACK,DUN_THEME_CATPAW,2,DUN_END_BOSS},
  {40,56,DUN_THEME_HOUSE,3,{THUG,SEA_RAT,SWABBIE}},//{40,56,DUN_STACK,DUN_THEME_BRICK,3,DUN_END_CHEST},
  {40,48,DUN_THEME_BRICK,5,{SAILOR,SEA_RAT,SWABBIE}},//{40,48,DUN_STACK,DUN_THEME_BRICK,6,DUN_END_CHEST},//{40,48,DUN_ZIGZAG,DUN_THEME_BRICK,6,DUN_END_CHEST},
  {57,51,DUN_THEME_SHIP,2,{SWABBIE,SAILOR,SKIPPER}},//{57,51,DUN_STACK,DUN_THEME_SHIP,2,DUN_END_CHEST},//{57,51,DUN_ZIGZAG,DUN_THEME_SHIP,2,DUN_END_CHEST},
  {56,42,DUN_THEME_SHIP,4,{SWABBIE,SAILOR,SKIPPER}},//{56,42,DUN_STACK,DUN_THEME_SHIP,4,DUN_END_BOSS},//{56,42,DUN_SPLIT,DUN_THEME_SHIP,4,DUN_END_BOSS},
  {54,31,DUN_THEME_WAREHOUSE,4,{WATCHER,BRUISER,MUSCLER}},//{54,31,DUN_STACK,DUN_THEME_BRICK,2,DUN_END_CHEST},//{54,31,DUN_STACK,DUN_THEME_BRICK,2,DUN_END_CHEST},
  //The following two were originally intended to connect
  //{44,31,DUN_THEME_WAREHOUSE,2,{WATCHER,BRUISER,MUSCLER}},//{44,31,DUN_STACK,DUN_THEME_WAREHOUSE,2,DUN_END_CHEST},
  //{36,31,DUN_THEME_WAREHOUSE,2,{WATCHER,BRUISER,MUSCLER}},//{36,31,DUN_STACK,DUN_THEME_WAREHOUSE,2,DUN_END_CHEST},
  //Two warehouse doors
  //{26,29,DUN_THEME_WAREHOUSE,4,{WATCHER,BRUISER,MUSCLER}},//{26,29,DUN_STACK,DUN_THEME_WAREHOUSE,4,DUN_END_CHEST},//{26,29,DUN_ZIGZAG,DUN_THEME_WAREHOUSE,4,DUN_END_CHEST},
  //{25,29,DUN_THEME_WAREHOUSE,4,{WATCHER,BRUISER,MUSCLER}},//{25,29,DUN_STACK,DUN_THEME_WAREHOUSE,4,DUN_END_CHEST},//{25,29,DUN_ZIGZAG,DUN_THEME_WAREHOUSE,4,DUN_END_CHEST},
  //The two hospital doors
  {50,16,DUN_THEME_HOSPITAL,7,{DOCTOR,PATIENT,SUBJECT}},//{50,16,DUN_STACK,DUN_THEME_HOSPITAL,9,DUN_END_BOSS},//{50,16,DUN_SPLIT,DUN_THEME_HOSPITAL,9,DUN_END_BOSS},
  {49,16,DUN_THEME_HOSPITAL,7,{DOCTOR,PATIENT,SUBJECT}},//{49,16,DUN_STACK,DUN_THEME_HOSPITAL,9,DUN_END_BOSS},//{49,16,DUN_SPLIT,DUN_THEME_HOSPITAL,9,DUN_END_BOSS},
  {55,4,DUN_THEME_WAREHOUSE,6,{WOW_RAT,WOW_RAT,WOW_RAT}},//{55,4,DUN_STACK,DUN_THEME_WAREHOUSE,8,DUN_END_CHEST},//{55,4,DUN_SPIRAL,DUN_THEME_WAREHOUSE,8,DUN_END_CHEST},
  {14,4,DUN_THEME_HOUSE,2,{SNOB,RICHMAN,SNOB}},//{14,4,DUN_STACK,DUN_THEME_HOUSE,2,DUN_END_CHEST},
  //{8,13,DUN_THEME_HOUSE,2,{SNOB,RICHMAN,MAX_RAT}},//{8,13,DUN_STACK,DUN_THEME_HOUSE,2,DUN_END_CHEST},
  //{14,13,DUN_THEME_HOUSE,2,{SNOB,RICHMAN,MAX_RAT}},//{14,13,DUN_STACK,DUN_THEME_HOUSE,2,DUN_END_CHEST},
  //The two doors of THE TOWER
  {28,16,DUN_THEME_TOWER,15,{GUARD,GOLEM,OFFICER}},//{28,16,DUN_STACK,DUN_THEME_TOWER,40,DUN_END_BOSS},//{28,16,DUN_SPIRAL,DUN_THEME_TOWER,40,DUN_END_BOSS},
  {29,16,DUN_THEME_TOWER,15,{GUARD,GOLEM,OFFICER}},//{29,16,DUN_STACK,DUN_THEME_TOWER,40,DUN_END_BOSS},//{28,16,DUN_SPIRAL,DUN_THEME_TOWER,40,DUN_END_BOSS},
};

void mapinit(char map[][MAPSIZE], int width, int height);
void mapgen(char map[][MAPSIZE], int mapwidth, int mapheight, int startx, int starty, int endx, int endy);
void mapdetail(char map[][MAPSIZE], int mapwidth, int mapheight);

const byte dungeon_tiles[] PROGMEM = {
  //DUN_THEME_CATPAW
0b01010101,
0b01010101,
0b01010101,
0b01010101,
0b11111111,
0b10000001,
0b10000001,
0b11111111,
0b01111110,
0b10000001,
0b10000001,
0b11000011,
0b10111101,
0b10000001,
0b10000001,
0b10000001,
0b10000001,
0b10000001,
0b10000001,
0b11100111,
0b10111101,
0b10000001,
0b10111101,
0b11000011,
0b00000000,
0b00000000,
0b01111110,
0b10000001,
0b01111110,
0b00011000,
0b00111100,
0b01000010,
  //DUN_THEME_BRICK
0b00100010,
0b11111111,
0b10001000,
0b11111111,
0b00100010,
0b11111111,
0b10001000,
0b11111111,
0b00000000,
0b11111111,
0b10100001,
0b10011001,
0b11000101,
0b10100011,
0b10011001,
0b11111111,
0b11111111,
0b10001001,
0b10010001,
0b10100011,
0b11000101,
0b10001001,
0b10010001,
0b11111111,
0b00011000,
0b01100110,
0b10000001,
0b11100111,
0b10011001,
0b10000001,
0b11100111,
0b11111111,
  //DUN_THEME_SHIP
0b00000000,
0b00000000,
0b00000000,
0b11111111,
0b00000000,
0b00000000,
0b00000000,
0b11111111,

0b00011000,
0b01100110,
0b10000001,
0b11100111,
0b10011001,
0b10000001,
0b11100111,
0b11111111,

0b10011001,
0b10000001,
0b11100111,
0b11111111,
0b10011001,
0b10000001,
0b01100110,
0b00011000,

0b11111111,
0b10000001,
0b10000001,
0b11111111,
0b10000111,
0b10011001,
0b11100001,
0b11111111,
  //DUN_THEME_HOSPITAL
0b00000001,
0b00000001,
0b00000001,
0b11111111,
0b00010000,
0b00010000,
0b00010000,
0b11111111,

0b01111110,
0b10000001,
0b10000001,
0b11000011,
0b10111101,
0b10000001,
0b10000001,
0b10000001,

0b10000001,
0b10000001,
0b10000001,
0b11100111,
0b10111101,
0b10000001,
0b10111101,
0b11000011,

0b00011100,
0b00110110,
0b11101100,
0b11011110,
0b01011011,
0b00111011,
0b01110111,
0b11011100,
};

void draw_dungeon(){
  uint8_t tile;
  for(byte i = 0; i < 16; i++){
    for(byte j = 0; j < 16; j++){
      tile = dungeon_map[i][j];
      if( tile != 0 ){
        if( tile <= NUM_DUN_TILES+1 ){
          /*gb.display.cursorX = (j*8 - dudex)+(SCREEN_WIDTH/2-4);
          gb.display.cursorY = (i*8 - dudey)+(SCREEN_HEIGHT/2-4);
          gb.display.print( ((tile-2)+(pgm_read_byte(&(dungeons[dungeonid].theme))*NUM_DUN_TILES) ) );*/
          gb.drawSlowXYBitmap((j*8 - dudex)+(SCREEN_WIDTH/2-4),(i*8 - dudey)+(SCREEN_HEIGHT/2-4),&dungeon_tiles[((tile-2)+(pgm_read_byte(&(dungeons[dungeonid].theme))*NUM_DUN_TILES))*8],8,8);
        }else{
          //If this is a common tile, then draw that specific tile
          gb.drawSlowXYBitmap((j*8 - dudex)+(SCREEN_WIDTH/2-4),(i*8 - dudey)+(SCREEN_HEIGHT/2-4),&common_tiles[(tile-2-NUM_DUN_TILES)*8],8,8);
        }
      }
    }
  }
  //Draw player.  If moving and frames dictate it so, play walking animation
  if( player_moving && (dudeframe/2 == 0 || dudeframe/2 == 2) ){
    gb.drawSlowXYBitmap(SCREEN_WIDTH/2-4,SCREEN_HEIGHT/2-4,player_sprites+((dudeframe/2)+dudeanimation*3)*8,8,8);
  }else{
    gb.drawSlowXYBitmap(SCREEN_WIDTH/2-4,SCREEN_HEIGHT/2-4,player_sprites+(1+dudeanimation*3)*8,8,8);
  }
  /*gb.display.cursorX = 0;
  gb.display.cursorY = 0;
  //gb.display.print(dungeon_map[1][1]-1+((pgm_read_byte(&(dungeons[dungeonid].theme))*NUM_DUN_TILES)*10));
  gb.display.println(dungeonid);*/
}

byte test_collision(byte dir){
  uint8_t tile = 0;

  //Prioritize exit tiles over other tiles
  if( dir == UP ){
    tile = dungeon_map[(dudey-0)/8][(dudex+1)/8];
    if( tile != DUN_TILE_DOOR && tile != DUN_TILE_STAIRSUP && tile != DUN_TILE_STAIRSDN && dungeon_map[(dudey-0)/8][(dudex+6)/8] ){
      return dungeon_map[(dudey-0)/8][(dudex+6)/8];
    }else{
      return tile;
    }
  }else if( dir == DOWN ){
    tile = dungeon_map[(dudey+7)/8][(dudex+1)/8];
    if( tile != DUN_TILE_DOOR && tile != DUN_TILE_STAIRSUP && tile != DUN_TILE_STAIRSDN && dungeon_map[(dudey+7)/8][(dudex+6)/8] ){
      return dungeon_map[(dudey+7)/8][(dudex+6)/8];
    }else{
      return tile;
    }
  }else if( dir == LEFT ){
    tile = dungeon_map[(dudey+1)/8][(dudex-0)/8];
    if( tile != DUN_TILE_DOOR && tile != DUN_TILE_STAIRSUP && tile != DUN_TILE_STAIRSDN && dungeon_map[(dudey+6)/8][(dudex-0)/8] ){
      return dungeon_map[(dudey+6)/8][(dudex-0)/8];
    }else{
      return tile;
    }
  }else{
    tile = dungeon_map[(dudey+1)/8][(dudex+7)/8];
    if( tile != DUN_TILE_DOOR && tile != DUN_TILE_STAIRSUP && tile != DUN_TILE_STAIRSDN && dungeon_map[(dudey+6)/8][(dudex+7)/8] ){
      return dungeon_map[(dudey+6)/8][(dudex+7)/8];
    }else{
      return tile;
    }
  }
}

void test_collide_exit(uint8_t collision){
  switch( collision ){
    case DUN_TILE_DOOR: 
      //switch( pgm_read_byte(&(dungeons[dungeonid].type)) ){
      //  case DUN_STACK:
          //In a stack layout, the only door is the exit to the dungeon,
          //so this one is simple: exit the dungeon and place the player outside
          mode = TO_WORLD;
          transition = -SCREEN_HEIGHT/2;
          persistence = true;
          dudex = pgm_read_byte(&(dungeons[dungeonid].x))*8;
          dudey = (pgm_read_byte(&(dungeons[dungeonid].y))+1)*8;
      //  break;
        //TODO: This requires more complex logic, depending on dungeon layout
      //}
      return;
    case DUN_TILE_STAIRSUP:
      previous_level = dungeon_level;
      dungeon_level++;
      dungeon_generated = 0;
      mode = TO_DUNGEON;
      transition = -SCREEN_HEIGHT/2;
      persistence = true;
      return;
    case DUN_TILE_STAIRSDN:
      previous_level = dungeon_level;
      dungeon_level--;
      dungeon_generated = 0;
      mode = TO_DUNGEON;
      transition = -SCREEN_HEIGHT/2;
      persistence = true;
      return;
  }
}

void step_dungeon(){

  if( dungeon_generated == 0 ){
     mapinit(dungeon_map,16,16);
     mapgen(dungeon_map,16,16,0,0,16,16);
     mapdetail(dungeon_map,16,16);
     //Default spawn position if spawning fails
     dudex = 16;
     dudey = 16;
     //Generate exits and spawn player
     mapexits(dungeon_map,16,16);
     dungeon_generated = 1;
  }
  
  player_moving = 0;
  

  /*  
  dudeframe++;
  dudeframe%=7;

  worldframe+=1;
  worldframe%=4;*/

  uint8_t collision = 0;
  
  if( gb.pressed(UP_BUTTON) ){
    collision = test_collision(UP);
    if( collision ){
      test_collide_exit(collision);
    }else{
      dudey--;
      dudeanimation = UP;
      player_moving = 1;
    }
  }
  else if( gb.pressed(DOWN_BUTTON) ){
    collision = test_collision(DOWN);
    if( collision ){
      test_collide_exit(collision);
    }else{
      dudey++;
      dudeanimation = DOWN;
      player_moving = 1;
    }
  }
  if( gb.pressed(LEFT_BUTTON) ){
    collision = test_collision(LEFT);
    if( collision ){
      test_collide_exit(collision);
    }else{
      dudex--;
      dudeanimation = LEFT;
      player_moving = 1;
    }
  }
  else if( gb.pressed(RIGHT_BUTTON) ){
    collision = test_collision(RIGHT);
    if( collision ){
      test_collide_exit(collision);
    }else{
      dudex++;
      dudeanimation = RIGHT;
      player_moving = 1;
    }
  }

  if( player_moving ){
    try_combat();
  }
}

void mapinit(char map[][MAPSIZE], int width, int height){
    int i,j;

    //Clear all tiles
    for( i = 0; i < width; i++ ){
      for( j = 0; j < height; j++ ){
        map[j][i] = 0;
        
        //Less runtime efficiency, but more space efficiency putting this here
        map[0][j] = DUN_WALL;
        map[height-1][j] = DUN_WALL;

        //Take advantage of square room TODO: if we have rectangular rooms this will break
        map[j][0] = DUN_WALL;
        map[j][width-1] = DUN_WALL;
      }
    }
    //Generate walls around the edges
    /*for( i = 0; i < width; i++ ){
      map[0][i] = DUN_WALL;
      map[height-1][i] = DUN_WALL;

      //Take advantage of square room TODO: if we have rectangular rooms this will break
      map[i][0] = DUN_WALL;
      map[i][width-1] = DUN_WALL;
    }*/
    /*for( j = 0; j < height; j++ ){
      map[j][0] = DUN_WALL;
      map[j][width-1] = DUN_WALL;
    }*/
}

#define HORIZONTAL 0
#define VERTICAL 1
#define MIN_WIDTH 6
#define MIN_HEIGHT 8
#define HALL_CHANCE 90
#define MIN_HALL_WIDTH 8
#define MIN_HALL_HEIGHT 10
#define MAX_HALL_WIDTH 12
#define MAX_HALL_HEIGHT 14
#define EXTRA_DOOR 10
void mapgen(char map[][MAPSIZE], int mapwidth, int mapheight, int startx, int starty, int endx, int endy){
    int i,j,orientation,position,door,door2,doorcount,hall,colspacex,colspacey;
    int width = endx-startx;
    int height = endy-starty;
    
    if( width < MIN_WIDTH && height < MIN_HEIGHT ){
        return;
    }

    //Determine whether to generate a big hall or not.
    //The size of the room determines the column spacing for now
    if( width >= MIN_HALL_WIDTH && height >= MIN_HALL_HEIGHT && width < MAX_HALL_WIDTH && height < MAX_HALL_HEIGHT ){
        hall = rand()%100;//Percent chance out of 100
        if( HALL_CHANCE > hall ){
            return;//Do not subdivide; return immediately
        }
    }
    
    //Determine whether we will split the space
    //horizontally or vertically by choosing whichever
    //orientation is larger (this avoids extremely long rooms)
    if( width >= height ){
        //If there is a door (or more than one door!) into a small room,
        //we may not be able to generate a wall in any location!
        //so abort if room is min + number of horiz door tiles
        doorcount = 0;
        for( i = startx; i < endx; i++ ){
            if( map[starty][i] == 0 ) doorcount++;
            if( map[endy][i] == 0 ) doorcount++;
        }
        if( width < MIN_WIDTH + doorcount ){
            return;
        }
        orientation = VERTICAL;
        //puts("Trying vertical");
    }else{
        //If there is a door (or more than one door!) into a small room,
        //we may not be able to generate a wall in any location!
        //so abort if room is min + number of vert door tiles
        doorcount = 0;
        for( i = starty; i < endy; i++ ){
            if( map[i][startx] == 0 ) doorcount++;
            if( map[i][endx] == 0 ) doorcount++;
        }
        if( height < MIN_HEIGHT + doorcount ){
            return;
        }
        orientation = HORIZONTAL;
        //puts("Trying horizontal");
    }
    //printf("startx %d, starty %d\n",startx,starty);
    //mapprint(map,MAPSIZE,MAPSIZE);
    position = -1;
    if( orientation == HORIZONTAL ){
        //Make sure the position is valid:
        //1. It must have generated at least one number
        //2. It must not be too close to existing walls
        //3. It must not be over a door
        while( position == -1 || position < starty + (MIN_HEIGHT/2) || position > endy - (MIN_HEIGHT/2) || map[position][startx] == 0 || map[position][endx] == 0 ){
            position = starty + (rand()%height);
        }
        //Generate a door at a random position
        door = startx + 1 + (rand()%(width-1));
        //Generate an extra door if wall is long enough.
        //Doors may overlap or be next to each other because such
        //doors shouldn't be a problem and it might result in interesting maps.
        //If it is not long enough set to -1 so it won't interfere
        door2 = width >= EXTRA_DOOR ? startx + 1 + (rand()%(width-1)) : -1;
        //printf("HORIZ %d\n",position);
        for( i = startx; i < startx + width; i++ ){
            if( i != door && i != door2 )
                map[position][i] = DUN_WALL;
        }
        //Recursively call to fill the two new spaces we generated
        mapgen(map, mapwidth, mapheight, startx, starty, endx,position);
        mapgen(map, mapwidth, mapheight, startx, position, endx, endy);
    }else if( orientation == VERTICAL ){
        //Make sure the position is valid:
        //1. It must have generated at least one number
        //2. It must not be too close to existing walls
        //3. It must not be over a door
        while( position == -1 || position < startx + (MIN_WIDTH/2) || position > endx - (MIN_WIDTH/2) || map[starty][position] == 0 || map[endy][position] == 0 ){
            position = startx + (rand()%width);
        }
        //Generate a door at a random position
        //(allocating space for it to be 2 high)
        door = starty + 1 + (rand()%(height-3));//I don't quite follow why this would need to be 3, but it seems to prevent a too-small doorway.  TODO: investigate
        //Generate an extra door if wall is long enough.
        //Doors may overlap or be next to each other because such
        //doors shouldn't be a problem and it might result in interesting maps.
        //If it is not long enough set to -1 so it won't interfere
        door2 = height >= EXTRA_DOOR ? starty + 1 + (rand()%(height-3)) : -1;
        //printf("VERT %d\n",position);
        for( i = starty; i < starty + height; i++ ){
            if( i != door && i != door+1 && i != door2 && i != door2+1 )
                map[i][position] = DUN_WALL;
        }
        //Recursively call to fill the two new spaces we generated
        mapgen(map, mapwidth, mapheight, startx, starty, position,endy);
        mapgen(map, mapwidth, mapheight, position, starty, endx, endy);
    }
}

uint8_t check_proximity(char map[][MAPSIZE], uint8_t i, uint8_t j, uint8_t extra){
  uint8_t k,m;
  for( k = i-2; k <= i+1+extra; k++ ){
    for( m = j-1; m <= j+1; m++ ){
      if( map[k][m] == DUN_WALL ){
        return 0;
      }
    }
  }
  return 1;
}

#define DECORATION_CHANCE 20
void mapdetail(char map[][MAPSIZE], int width, int height){
  uint8_t i,j,k,m;
  //Avoid checking the boundary tiles
  for( i = 1; i < height-1; i++ ){
    for( j = 1; j < width-1; j++ ){
      //Place a sideways wall under each wall top
      if( map[i-1][j] == DUN_WALL && map[i][j] == 0 ){
        map[i][j] = 2;
      }
      //Check if we want to put something decorative
      if( i > 1 && map[i][j] == 0 && DECORATION_CHANCE > (rand()%100) ){
        if( 50 > (rand()%100) ){
          if( check_proximity( map, i, j, 0 ) ){
            map[i][j] = 5;
          }
        }else{
          if( check_proximity( map, i, j, 1 ) ){
            map[i][j] = 3;
            map[i+1][j] = 4;
          }
        }
      }
    }
  }
}

void mapexits(char map[][MAPSIZE], int width, int height){
  uint8_t i,j;
  //switch( pgm_read_byte(&(dungeons[dungeonid].type)) ){
  //  case DUN_STACK:
      //If on ground floor, generate entrance door
      if( dungeon_level == 0 ){
        while(1){//Generate until we get a valid placement
          i = height-1;
          j = 1+(rand()%(width-1));
          //If the space above is clear, place a door
          if( map[i-1][j] == 0 ){
            map[i][j] = DUN_TILE_DOOR;
            //If the previous room is outside, spawn player here
            if( previous_level == -1 ){
              dudex = j*8;
              dudey = (i-1)*8;
            }
            break;
          }
        }
      }else{ //If not on ground level, generate stairs down
        while(1){//Generate until we get a valid placement
          i = 1+(rand()%(height-1));
          j = 1+(rand()%(width-1));
          //If the space for the stairs is clear, place stairs down
          if( map[i][j] == 0 ){
            map[i][j] = DUN_TILE_STAIRSDN;
            //If the previous room is below this one, spawn player here
            if( previous_level >= 0 && previous_level < dungeon_level ){
              if( map[i][j+1] == 0 ){
                dudex = (j+1)*8;
                dudey = i*8;
              }else if( map[i+1][j] == 0 ){
                dudex = j*8;
                dudey = (i+1)*8;
              }else if( map[i][j-1] == 0 ){
                dudex = (j-1)*8;
                dudey = i*8;
              }else if( map[i-1][j] == 0 ){
                dudex = j*8;
                dudey = (i-1)*8;
              }
            }
            break;
          }
          
        }
      }
      //If not at the top of the stack of rooms, generate stairs up
      if( dungeon_level < pgm_read_byte(&(dungeons[dungeonid].size))-1 ){
        while(1){//Generate until we get a valid placement
          i = 1+(rand()%(height-1));
          j = 1+(rand()%(width-1));
          //If the space for the stairs is clear, place stairs up
          if( map[i][j] == 0 ){
            map[i][j] = DUN_TILE_STAIRSUP;
            //If the previous room is above this one, spawn player here
            if( previous_level > dungeon_level ){
              if( map[i][j+1] == 0 ){
                dudex = (j+1)*8;
                dudey = i*8;
              }else if( map[i+1][j] == 0 ){
                dudex = j*8;
                dudey = (i+1)*8;
              }else if( map[i][j-1] == 0 ){
                dudex = (j-1)*8;
                dudey = i*8;
              }else if( map[i-1][j] == 0 ){
                dudex = j*8;
                dudey = (i-1)*8;
              }
            }
            break;
          }
        }
      }
      //break;
    //TODO: Add code that puts doors in random positions on fixed sides,
    //based on which level of the dungeon we are on, making sure they do
    //not have a wall blocking them. Do this for all other types.
  //}
}
//ALSO TODO: add drawing of exit doors (and chests) as a separate thing
//since it's common to all dungeons
//ALSO ALSO TODO: Add code to spawn the player at the correct door based
//on the room they last came from (or if they just entered the dungeon).

#include "events.h"
#include "save.h"
#include "battle.h"
#include "dungeon.h"
#include "dialogue.h"

struct dialogue_event{
  uint8_t status_index;
  uint8_t status_value;
  uint8_t dungeon_id;
  uint8_t dungeon_level;
  uint16_t dialogue_index;
  uint8_t dialogue_len;
  uint8_t name_index;
};

/* Boss events depend on dialogue occurring before
 * them, so they don't bother to check the dungeon.
 */
struct boss_event{
  uint8_t status_index;
  uint8_t status_value;
  uint8_t boss_id;
};

#define NUM_DIALOGUE_EVENTS 58
const dialogue_event dialogue_events[NUM_DIALOGUE_EVENTS] PROGMEM = {
  //CATPAW---SLAVER DIALOGUE
  {STATUS_MAIN,2,0,2,TXT_SLAVER,TXT_SLAVER_LEN,SLAVER},
  //CATPAW---GIRL THANKS
  {STATUS_MAIN,4,0,2,TXT_GIRL_THX,TXT_GIRL_THX_LEN,3},
  //CATPAW---SHADOW DIALOGUE
  {STATUS_MAIN,5,0,0,TXT_SDW_BATTLE,TXT_SDW_BATTLE_LEN,SHADOW},
  //CATPAW---SHADOW JOINS (SPECIAL CASE)
  {STATUS_MAIN,7,0,0,TXT_SDW_WIN,TXT_SDW_WIN_LEN,SHADOW},
  //FATHER'S RESIDENCE---GIRL WARNING
  {STATUS_MAIN,8,1,0,TXT_GIRL_FATHER,TXT_GIRL_FATHER_LEN,3},
  //FATHER'S RESIDENCE---ENEMY ALERT
  {STATUS_MAIN,9,1,2,TXT_ENEMY,TXT_ENEMY_LEN,BRUISER},
  //FATHER'S RESIDENCE---FATHER THANKS
  {STATUS_MAIN,11,1,2,TXT_FATHER,TXT_FATHER_LEN,4},
  //LARGE SHIP---SHADOW INFO
  {STATUS_MAIN,12,4,0,TXT_SDW_SHIP,TXT_SDW_SHIP_LEN,SHADOW},
  //LARGE SHIP---ENEMY ALERT
  {STATUS_MAIN,13,4,3,TXT_ENEMY,TXT_ENEMY_LEN,CAPTAIN},
  //LARGE SHIP---NURSE THANKS
  {STATUS_MAIN,15,4,3,TXT_NSE_THX,TXT_NSE_THX_LEN,NURSE},
  //LARGE SHIP---SHADOW RESPONSE
  {STATUS_MAIN,16,4,3,TXT_SDW_RETURN,TXT_SDW_RETURN_LEN,SHADOW},
  //LARGE SHIP---NURSE SUSPICIOUS (BUT JOINS)
  {STATUS_MAIN,17,4,3,TXT_NSE_SUSPICIOUS,TXT_NSE_SUSPICIOUS_LEN,NURSE},
  //HOSPITAL---NURSE CHAOS COMMENT
  {STATUS_MAIN,18,6,0,TXT_NSE_CHAOS,TXT_NSE_CHAOS_LEN,NURSE},
  {STATUS_MAIN,18,7,0,TXT_NSE_CHAOS,TXT_NSE_CHAOS_LEN,NURSE},
  //HOSPITAL---MADMAN ESCAPED COMMENT
  {STATUS_MAIN,19,6,6,TXT_MAD_ESCAPED,TXT_MAD_ESCAPED_LEN,MADMAN},
  {STATUS_MAIN,19,7,6,TXT_MAD_ESCAPED,TXT_MAD_ESCAPED_LEN,MADMAN},
  //HOSPITAL---NURSE WHY
  {STATUS_MAIN,20,6,6,TXT_NSE_WHY,TXT_NSE_WHY_LEN,NURSE},
  {STATUS_MAIN,20,7,6,TXT_NSE_WHY,TXT_NSE_WHY_LEN,NURSE},
  //HOSPITAL---MADMAN OFFER
  {STATUS_MAIN,21,6,6,TXT_MAD_OFFER,TXT_MAD_OFFER_LEN,MADMAN},
  {STATUS_MAIN,21,7,6,TXT_MAD_OFFER,TXT_MAD_OFFER_LEN,MADMAN},
  //HOSPITAL---NURSE SAYING PLAGUE SPREADS
  {STATUS_MAIN,22,6,6,TXT_NSE_SPREAD,TXT_NSE_SPREAD_LEN,NURSE},
  {STATUS_MAIN,22,7,6,TXT_NSE_SPREAD,TXT_NSE_SPREAD_LEN,NURSE},
  //HOSPITAL---MADMAN MISHAP
  {STATUS_MAIN,23,6,6,TXT_MAD_MISHAP,TXT_MAD_MISHAP_LEN,MADMAN},
  {STATUS_MAIN,23,7,6,TXT_MAD_MISHAP,TXT_MAD_MISHAP_LEN,MADMAN},
  //HOSPITAL---NURSE MONSTER
  {STATUS_MAIN,24,6,6,TXT_NSE_MONSTER,TXT_NSE_MONSTER_LEN,NURSE},
  {STATUS_MAIN,24,7,6,TXT_NSE_MONSTER,TXT_NSE_MONSTER_LEN,NURSE},
  //HOSPITAL---NURSE CONFRONT
  {STATUS_MAIN,26,6,6,TXT_NSE_CONFRONT,TXT_NSE_CONFRONT_LEN,NURSE},
  {STATUS_MAIN,26,7,6,TXT_NSE_CONFRONT,TXT_NSE_CONFRONT_LEN,NURSE},
  //TOWER---SHADOW WARNING
  {STATUS_MAIN,27,10,0,TXT_SDW_TOWER,TXT_SDW_TOWER_LEN,SHADOW},
  {STATUS_MAIN,27,11,0,TXT_SDW_TOWER,TXT_SDW_TOWER_LEN,SHADOW},
  //TOWER---LEADER SNEER
  {STATUS_MAIN,28,10,14,TXT_LDR_ANTS,TXT_LDR_ANTS_LEN,LEADER},
  {STATUS_MAIN,28,11,14,TXT_LDR_ANTS,TXT_LDR_ANTS_LEN,LEADER},
  //TOWER---NURSE RETRIBUTION
  {STATUS_MAIN,29,10,14,TXT_NSE_PAY,TXT_NSE_PAY_LEN,NURSE},
  {STATUS_MAIN,29,11,14,TXT_NSE_PAY,TXT_NSE_PAY_LEN,NURSE},
  //TOWER---LEADER SNIDE EVIL REPLY
  {STATUS_MAIN,30,10,14,TXT_LDR_CLEAN,TXT_LDR_CLEAN_LEN,LEADER},
  {STATUS_MAIN,30,11,14,TXT_LDR_CLEAN,TXT_LDR_CLEAN_LEN,LEADER},
  //TOWER---NURSE NOBLE COMEBACK
  {STATUS_MAIN,31,10,14,TXT_NSE_DESTROY,TXT_NSE_DESTROY_LEN,NURSE},
  {STATUS_MAIN,31,11,14,TXT_NSE_DESTROY,TXT_NSE_DESTROY_LEN,NURSE},
  //TOWER---SHADOW VICTORY
  {STATUS_MAIN,33,10,14,TXT_SDW_VICTORY,TXT_SDW_VICTORY_LEN,SHADOW},
  {STATUS_MAIN,33,11,14,TXT_SDW_VICTORY,TXT_SDW_VICTORY_LEN,SHADOW},
  //TOWER---NURSE VICTORY
  {STATUS_MAIN,34,10,14,TXT_NSE_VICTORY,TXT_NSE_VICTORY_LEN,NURSE},
  {STATUS_MAIN,34,11,14,TXT_NSE_VICTORY,TXT_NSE_VICTORY_LEN,NURSE},
  //TOWER---ENDING TEXT
  {STATUS_MAIN,35,10,14,TXT_ENDING,TXT_ENDING_LEN,5},
  {STATUS_MAIN,35,11,14,TXT_ENDING,TXT_ENDING_LEN,5},
  //FIGHTING BAR---WELCOME
  {STATUS_BAR,0,2,0,TXT_BAR,TXT_BAR_LEN,6},
  //FIGHTING BAR---TOP FIGHTER
  {STATUS_BAR,1,2,4,TXT_FIGHTER,TXT_FIGHTER_LEN,MUSCLER},
  //SMALL SHIP---OLD CAPTAIN
  {STATUS_MUTINY,0,3,0,TXT_MUTINY,TXT_MUTINY_LEN,CAPTAIN},
  //SMALL SHIP---NEW CAPTAIN
  {STATUS_MUTINY,1,3,1,TXT_MUTINEER,TXT_MUTINEER_LEN,SAILOR},
  //SMALL SHIP---OLD CAPTAIN THANKS
  {STATUS_MUTINY,3,3,0,TXT_CAPTAIN_THX,TXT_CAPTAIN_THX_LEN,CAPTAIN},
  //NORTH WAREHOUSE---FEAR THE RATS
  {STATUS_RATS,0,8,0,TXT_RATS,TXT_RATS_LEN,6},
  //NORTH WAREHOUSE---BEHOLD THE RAT
  {STATUS_RATS,1,8,5,TXT_MEGARAT,TXT_MEGARAT_LEN,5},
  //NORTH WAREHOUSE---RAT DENOUEMENT
  {STATUS_RATS,3,8,0,TXT_RAT_THX,TXT_RAT_THX_LEN,6},
  //SNOB HOUSE---INTRO
  {STATUS_SNOBS,0,9,0,TXT_SNOBS,TXT_SNOBS_LEN,SNOB},
  //SNOB HOUSE---RICH
  {STATUS_SNOBS,1,9,1,TXT_RICH,TXT_RICH_LEN,RICHMAN},
  //SNOB HOUSE---WOW
  {STATUS_SNOBS,3,9,0,TXT_SNOB_WOW,TXT_SNOB_WOW_LEN,SNOB},
  //DOCKS WAREHOUSE---HELP
  {STATUS_WARES,0,5,0,TXT_WARES,TXT_WARES_LEN,6},
  //DOCKS WAREHOUSE---ALERT
  {STATUS_WARES,1,5,3,TXT_ENEMY,TXT_ENEMY_LEN,MUSCLER},
  //DOCKS WAREHOUSE---THANKS
  {STATUS_WARES,3,5,0,TXT_WARES_THX,TXT_WARES_THX_LEN,6},
};

#define NUM_BOSS_EVENTS 11
const boss_event boss_events[NUM_BOSS_EVENTS] PROGMEM = {
  //CATPAW---SLAVER BOSS
  {STATUS_MAIN,3,SLAVER},
  //CATPAW---SHADOW BOSS
  {STATUS_MAIN,6,EN_SHADOW},
  //FATHER'S RESIDENCE---BRUISER BOSS
  {STATUS_MAIN,10,BRUISER},
  //LARGE SHIP---CAPTAIN BOSS
  {STATUS_MAIN,14,CAPTAIN},
  //HOSPITAL---MADMAN BOSS
  {STATUS_MAIN,25,MADMAN},
  //TOWER---LEADER FINAL BOSS
  {STATUS_MAIN,32,LEADER},
  //FIGHTING BAR---MUSCLER BOSS
  {STATUS_BAR,2,MUSCLER},
  //SMALL SHIP---CAPTAIN BOSS
  {STATUS_MUTINY,2,CAPTAIN},
  //NORTH WAREHOUSE---MAXIMUM RAT BOSS
  {STATUS_RATS,2,MAX_RAT},
  //SNOB HOUSE---RICHMAN BOSS
  {STATUS_SNOBS,2,RICHMAN},
  //DOCKS WAREHOUSE---MUSCLER BOSS
  {STATUS_WARES,2,MUSCLER},
};

void check_events(){
  uint8_t i,j;
  uint8_t* tmp;
  struct dialogue_event event;
  struct boss_event bevent;
  for( i = 0; i < NUM_DIALOGUE_EVENTS; i++ ){
    tmp = (uint8_t*)&dialogue_events[i];
    // Load our local variable with this entry
    for( j = 0; j < 8; j++ ){
      //Assume the byte layout in memory to read bytes
      *(((uint8_t*)&event)+j) = pgm_read_byte(tmp + j);
    }
    // If this event is active (correct status value, dungeon, and dungeon floor)
    if( game_status[event.status_index] == event.status_value && dungeonid == event.dungeon_id && dungeon_level == event.dungeon_level ){
      //Special case: shadow joins party
      if( game_status[event.status_index] == 7 ){
        party[SHADOW].level = 8;//Add shadow to party
      }
      if( game_status[event.status_index] == 17 ){
        party[NURSE].level = 10;//Add nurse to party
      }
      game_status[event.status_index]++;// Increment status to next state
      // Take advantage of having no battles with low-level enemies
      // Therefore, if the indices that we use of each array don't overlap,
      // we can avoid having to specify which array we are using
      if( event.name_index < 7 ){
        display_dialogue(event.dialogue_index,event.dialogue_len,event.name_index,player_names);
      }else{
        display_dialogue(event.dialogue_index,event.dialogue_len,event.name_index,enemy_names);
      }
      return;
    }
  }
  for( i = 0; i < NUM_BOSS_EVENTS; i++ ){
    tmp = (uint8_t*)&boss_events[i];
    // Load our local variable with this entry
    for( j = 0; j < 3; j++ ){
      //Assume the byte layout in memory to read bytes
      *(((uint8_t*)&bevent)+j) = pgm_read_byte(tmp + j);
    }
    // If this boss event is active (correct status value only)
    if( game_status[bevent.status_index] == bevent.status_value ){
      game_status[bevent.status_index]++;
      meta_mode = bevent.boss_id;//Boss id stored in meta_mode
      mode = COMBAT;
      return;
    }
  }
}


#include "dungeon.h"
#include "overworld.h"

int8_t transition = 0;

//up,down,left,right
//1,none,2

uint8_t player_moving = 0;

const uint8_t player_sprites[] PROGMEM = {
// Up
0b00011000,
0b01100110,
0b10000001,
0b10000001,
0b01000010,
0b10111100,
0b01011010,
0b00100001,

0b00011000,
0b01100110,
0b10000001,
0b10000001,
0b01000010,
0b00111100,
0b11011011,
0b00100100,

0b00011000,
0b01100110,
0b10000001,
0b10000001,
0b01000010,
0b00111101,
0b01011010,
0b10000100,
// Down
0b00011000,
0b01100110,
0b10000001,
0b10100101,
0b01000010,
0b10111100,
0b01011010,
0b00100001,

0b00011000,
0b01100110,
0b10000001,
0b10100101,
0b01000010,
0b00111100,
0b11011011,
0b00100100,

0b00011000,
0b01100110,
0b10000001,
0b10100101,
0b01000010,
0b00111101,
0b01011010,
0b10000100,
// Left
0b00011000,
0b01100110,
0b10000001,
0b10100001,
0b01000010,
0b00111100,
0b01011010,
0b00010100,

0b00011000,
0b01100110,
0b10000001,
0b10100001,
0b01000010,
0b00111100,
0b01011010,
0b00100100,

0b00011000,
0b01100110,
0b10000001,
0b10100001,
0b01000010,
0b00111100,
0b01011010,
0b00101000,
// Right
0b00011000,
0b01100110,
0b10000001,
0b10000101,
0b01000010,
0b00111100,
0b01011010,
0b00101000,

0b00011000,
0b01100110,
0b10000001,
0b10000101,
0b01000010,
0b00111100,
0b01011010,
0b00100100,

0b00011000,
0b01100110,
0b10000001,
0b10000101,
0b01000010,
0b00111100,
0b01011010,
0b00010100,
};

const byte tiles[] PROGMEM = {

0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,

0b00100010,
0b11111111,
0b10001000,
0b11111111,
0b00100010,
0b11111111,
0b10001000,
0b11111111,

0b10000001,
0b10000001,
0b10000001,
0b11111111,
0b10000001,
0b10000001,
0b10000001,
0b11111111,

0b00011000,
0b01100110,
0b10000001,
0b11111111,
0b10000001,
0b10000001,
0b11011011,
0b11111111,

0b11111111,
0b10000001,
0b10000001,
0b10000001,
0b11000001,
0b10000001,
0b10000001,
0b10000001,

0b00111100,
0b01111110,
0b11111111,
0b11111111,
0b11111111,
0b10000101,
0b01010110,
0b01010010,

0b00111100,
0b01111110,
0b11111111,
0b11111111,
0b11111111,
0b10010001,
0b01010110,
0b01000110,

0b01010010,
0b01000010,
0b01000010,
0b01001010,
0b01001010,
0b01001010,
0b01000010,
0b01010010,

0b01000110,
0b01010010,
0b01010010,
0b01010010,
0b01000010,
0b01001010,
0b01001010,
0b01001010,

0b00111010,
0b11111111,
0b10011000,
0b11111111,
0b00111010,
0b11111111,
0b10011000,
0b11111111,

0b00000000,
0b11111111,
0b10101010,
0b01010101,
0b00000000,
0b10101010,
0b01010101,
0b11111111,

0b00111100,
0b01100110,
0b11111111,
0b10100101,
0b10100101,
0b11111111,
0b01100110,
0b00111100,

0b00100010,
0b01100110,
0b10001000,
0b10011001,
0b00100010,
0b01100110,
0b10001000,
0b10011001,

0b00100010,
0b01010100,
0b10001000,
0b00010101,
0b00100010,
0b01010100,
0b10001000,
0b00010101,

0b00111010,
0b11111111,
0b10100100,
0b11100111,
0b00100110,
0b11100111,
0b10011000,
0b11111111,

0b00100010,
0b01010101,
0b10001000,
0b01010101,
0b00100010,
0b01010101,
0b10001000,
0b01010101,

0b00100010,
0b01010101,
0b00000000,
0b01010101,
0b00100010,
0b01010101,
0b00000000,
0b01010101,

0b00000000,
0b00100000,
0b01011100,
0b00000000,
0b00000000,
0b00000100,
0b00011010,
0b00000000,

0b00000000,
0b00000001,
0b11100010,
0b00000000,
0b00000000,
0b10000000,
0b01000011,
0b00000000,

0b11111111,
0b10011001,
0b10011001,
0b11111111,
0b10011001,
0b10011001,
0b11111111,
0b11111111,

0b11111111,
0b01010101,
0b00110110,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,

0b10000000,
0b10010000,
0b10101100,
0b10000000,
0b10000000,
0b10000100,
0b10111010,
0b10000000,

0b10000000,
0b10000100,
0b10111010,
0b10000000,
0b10000000,
0b10010000,
0b10101100,
0b10000000,

0b00000010,
0b00011101,
0b00000000,
0b00001000,
0b01110100,
0b00000000,
0b00000000,
0b11111111,

0b00100000,
0b01011100,
0b00000000,
0b00000100,
0b00001011,
0b00000000,
0b00000000,
0b11111111,

0b11111111,
0b10001001,
0b10010001,
0b10100011,
0b11000101,
0b10001001,
0b10010001,
0b11111111,

0b00000000,
0b11111111,
0b10100001,
0b10011001,
0b11000101,
0b10100011,
0b10011001,
0b11111111,

0b00000001,
0b00000001,
0b00000001,
0b11111111,
0b00010000,
0b00010000,
0b00010000,
0b11111111,

0b00000000,
0b00000000,
0b00000000,
0b11111111,
0b00000000,
0b00000000,
0b00000000,
0b11111111,

0b00000000,
0b00000011,
0b00001100,
0b11110000,
0b11000000,
0b00000000,
0b00000000,
0b00000000,

0b00000000,
0b00000000,
0b00000000,
0b11111111,
0b00000001,
0b00001110,
0b01110000,
0b10000000,

0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b11111111,

0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,
0b00010100,

0b00000000,
0b00000000,
0b01011000,
0b01100110,
0b10111010,
0b10111101,
0b11100011,
0b11111111,

0b00000000,
0b00000100,
0b01011010,
0b00110010,
0b01100110,
0b00111011,
0b01100110,
0b00010000,

0b00011000,
0b11011011,
0b11000011,
0b00001000,
0b11011110,
0b11011110,
0b00011100,
0b00000000,

0b00011000,
0b00011000,
0b00011000,
0b11111111,
0b11111111,
0b00011000,
0b00011000,
0b00011000,

0b00000011,
0b00001100,
0b00110000,
0b11010000,
0b00010011,
0b00011101,
0b00110001,
0b11000001,

0b11000000,
0b00110000,
0b00001100,
0b00001011,
0b11001000,
0b10111000,
0b10001100,
0b10000011,

0b00000011,
0b00001100,
0b00110000,
0b11010000,
0b00010011,
0b00011100,
0b00110000,
0b11000000,

0b00000011,
0b00001100,
0b00110000,
0b11000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,

0b11000000,
0b00110000,
0b00001100,
0b00001011,
0b11001000,
0b00111000,
0b00001100,
0b00000011,

0b11000000,
0b00110000,
0b00001100,
0b00000011,
0b00000000,
0b00000000,
0b00000000,
0b00000000,

0b10000000,
0b10010000,
0b10101110,
0b10000000,
0b10001000,
0b10110100,
0b10000000,
0b11111111,

0b10000000,
0b10000100,
0b10011010,
0b10000000,
0b10010000,
0b10101110,
0b10000000,
0b11111111,

0b11111111,
0b11010101,
0b10110110,
0b10010100,
0b10010100,
0b10010100,
0b10010100,
0b10010100,

0b00000000,
0b00000000,
0b00000000,
0b11111111,
0b10000000,
0b01110000,
0b00001110,
0b11111111,

0b00000000,
0b11000000,
0b00110000,
0b00001111,
0b00000011,
0b00000000,
0b00000000,
0b11111111,

0b00010000,
0b00001000,
0b00001000,
0b00010000,
0b00001000,
0b00001000,
0b00000100,
0b00001000,

0b00000000,
0b00000000,
0b00011000,
0b00100100,
0b00010011,
0b00010000,
0b00001000,
0b00010000,

0b00000000,
0b00000000,
0b00001100,
0b10010010,
0b01100001,
0b00000000,
0b00000000,
0b00000000,

0b00000000,
0b00000000,
0b00011000,
0b00100100,
0b11000010,
0b00000100,
0b00001000,
0b00010000,

0b00001000,
0b00010000,
0b00010000,
0b00001000,
0b00000100,
0b00001000,
0b00010000,
0b00001000,

0b00010000,
0b00001000,
0b00000100,
0b00001000,
0b11000100,
0b00100010,
0b00011100,
0b00000000,

0b00000000,
0b00000000,
0b00000000,
0b01101001,
0b10010110,
0b00000000,
0b00000000,
0b00000000,

0b00010000,
0b00100000,
0b00100000,
0b01000001,
0b00100010,
0b00011010,
0b00000100,
0b00000000,
};

const byte world[] PROGMEM = {
11,15,1,2,5,15,1,2,3,1,4,38,8,27,4,37,5,1,1,2,1,1,1,2,13,1,1,2,
5,17,11,1,1,2,5,1,1,2,3,1,4,38,1,27,1,11,4,27,1,11,1,27,4,37,5,1,1,2,1,1,1,2,13,1,1,2,
5,17,7,1,1,19,1,1,1,19,1,1,1,2,1,1,1,19,1,1,1,19,1,1,1,2,3,1,4,38,8,27,4,37,5,1,1,2,1,1,1,2,5,1,2,19,6,1,1,2,
5,17,10,1,1,14,1,2,1,14,3,1,1,14,1,2,1,1,1,14,1,1,4,38,1,27,1,11,4,27,1,11,1,27,4,37,5,1,1,2,1,1,1,2,1,1,1,14,1,1,1,14,1,1,2,19,1,1,1,14,4,1,1,2,
5,17,8,1,1,4,1,1,1,9,1,2,1,9,1,1,1,4,1,1,1,9,1,2,1,1,1,9,1,1,4,38,8,27,4,37,1,1,1,14,1,1,1,14,1,1,1,2,1,0,1,2,1,1,1,9,1,1,1,9,4,1,1,9,1,1,1,4,2,1,1,2,
5,17,11,0,1,32,9,0,4,38,1,27,1,11,4,27,1,11,1,27,4,37,1,1,1,9,1,1,1,9,1,1,1,2,16,0,1,21,
4,17,11,0,1,32,9,0,4,38,8,27,4,37,21,0,1,26,1,21,
4,17,11,10,1,3,5,10,1,3,3,0,4,38,1,27,1,11,4,27,1,11,1,27,4,37,6,0,1,3,12,10,1,3,1,0,1,25,1,21,
4,17,11,15,1,2,5,15,1,2,3,0,4,38,8,27,4,37,6,0,1,2,12,15,1,2,2,0,1,21,
4,17,11,15,1,2,5,15,1,2,3,0,4,38,1,27,1,11,4,27,1,11,1,27,4,37,4,0,1,3,1,10,1,2,12,15,1,2,1,10,1,3,
5,17,11,1,1,2,5,1,1,2,3,0,4,38,8,27,4,37,4,0,1,32,1,27,1,2,12,27,1,2,1,27,1,32,
5,17,11,1,1,2,1,1,1,19,1,1,1,19,1,1,1,2,3,0,4,38,1,27,1,11,4,27,1,11,1,27,4,37,4,0,1,32,1,27,1,2,1,27,1,19,1,27,1,19,4,27,1,19,1,27,1,19,1,27,1,2,1,27,1,32,
5,17,11,1,1,2,1,14,3,1,1,14,1,2,3,0,4,38,8,27,4,37,4,0,1,32,1,0,1,2,12,27,1,2,1,0,1,32,
5,17,11,1,1,2,1,9,1,1,1,4,1,1,1,9,1,2,3,0,4,38,1,27,1,11,4,27,1,11,1,27,4,37,4,0,1,32,1,0,1,2,1,27,1,19,1,27,1,19,4,27,1,19,1,27,1,19,1,27,1,2,1,0,1,32,
5,17,15,0,1,32,5,0,4,38,8,27,4,37,4,0,1,32,1,0,1,2,12,27,1,2,1,0,1,32,
5,17,15,0,1,32,5,0,1,42,1,41,2,38,1,27,1,11,4,27,1,11,1,27,1,39,1,37,1,39,1,40,4,0,1,32,1,0,1,2,1,27,1,19,1,27,1,19,1,36,2,27,1,36,1,19,1,27,1,19,1,27,1,2,1,0,1,32,
5,17,17,10,1,3,5,0,1,42,1,41,3,27,2,4,3,27,1,39,1,40,6,0,1,32,1,0,1,2,5,27,2,4,5,27,1,2,1,0,1,32,
5,17,17,15,1,2,23,0,1,32,16,0,1,32,
5,17,3,10,1,3,13,15,1,2,3,0,1,26,19,0,1,32,16,0,1,32,
5,17,3,12,1,2,4,1,1,3,3,10,1,3,4,1,1,2,3,0,1,25,5,0,1,26,13,0,1,32,6,0,1,36,2,0,1,36,6,0,1,32,
5,17,3,12,1,2,4,1,1,2,3,12,1,2,2,1,1,19,1,1,1,2,9,0,1,25,1,26,7,0,1,26,4,0,1,3,6,10,1,3,2,0,1,3,6,10,1,3,
5,17,3,1,1,2,4,10,1,2,3,12,1,2,4,1,1,2,10,0,1,25,6,0,1,26,1,25,4,0,1,2,6,27,1,2,2,0,1,2,6,27,1,2,
5,17,3,1,1,2,4,1,1,2,1,19,1,1,1,19,1,2,2,1,1,19,1,1,1,2,17,0,1,25,5,0,1,2,6,27,1,2,2,0,1,2,6,27,1,2,
5,17,3,1,1,2,4,1,1,2,1,1,1,35,1,1,1,2,4,1,1,2,40,0,1,26,
5,17,5,10,1,3,2,0,1,2,1,19,1,4,1,19,1,3,16,10,1,3,27,0,1,26,1,25,
5,17,5,12,1,2,6,0,1,2,16,13,1,2,1,3,2,10,1,3,23,0,2,25,
5,17,5,12,1,2,6,0,1,2,16,13,2,2,2,1,1,2,1,3,11,10,1,3,4,0,1,3,6,10,1,3,
5,17,5,1,1,2,6,0,1,2,4,19,3,27,4,19,3,27,2,19,2,2,2,1,2,2,11,13,1,2,4,0,1,2,6,12,1,2,
5,17,5,1,1,2,6,0,1,2,16,27,1,2,4,0,1,2,11,13,1,2,1,3,2,0,1,3,1,2,6,12,1,2,
5,17,2,1,1,5,1,1,1,5,1,2,6,0,1,2,4,19,13,27,4,0,1,2,11,1,2,2,2,0,2,2,6,1,1,2,
5,17,2,1,1,7,1,1,1,7,1,2,1,54,1,3,2,0,1,3,1,50,2,54,1,50,1,51,18,0,1,2,3,1,1,19,2,1,1,19,2,1,1,19,1,1,1,2,4,0,1,2,1,1,1,19,2,1,1,19,1,1,1,2,
12,17,1,32,2,0,1,32,4,17,1,55,1,51,17,0,1,2,1,1,1,4,9,1,1,2,4,0,1,2,2,1,1,4,3,1,1,2,
12,17,1,3,2,0,1,3,5,17,1,48,1,3,13,10,7,0,1,3,20,0,1,21,
4,17,2,54,1,50,1,54,1,50,2,54,1,2,2,0,1,2,1,54,1,50,1,51,2,17,1,48,1,2,13,13,7,0,1,3,20,0,1,21,
4,17,6,10,1,3,6,0,1,48,2,17,1,52,1,2,13,13,7,0,1,3,20,0,1,21,
4,17,6,12,1,2,6,0,1,48,2,17,1,52,1,2,13,27,7,0,1,3,20,0,1,21,
4,17,6,12,1,2,6,0,1,52,2,17,1,52,1,2,11,27,1,3,10,10,1,3,4,0,1,45,13,20,
5,17,6,1,1,2,6,0,1,52,2,17,1,52,1,2,1,19,3,27,1,19,3,27,1,19,2,27,1,2,10,13,1,2,4,0,1,21,
18,17,1,1,4,19,1,1,1,2,6,0,1,48,2,17,1,48,1,2,11,27,1,2,10,13,1,2,4,0,1,21,6,17,1,20,4,17,1,20,3,17,1,20,
2,17,6,1,1,2,6,0,1,3,2,10,1,3,12,0,1,2,10,1,1,2,4,0,1,21,6,17,1,20,4,17,1,20,3,17,1,20,
2,17,5,10,1,3,7,0,1,2,2,1,1,2,12,0,1,2,10,1,1,2,4,0,1,43,3,23,3,17,1,32,4,17,1,32,3,17,1,32,
2,17,5,12,1,2,12,0,1,3,7,10,1,3,2,0,1,2,10,1,1,2,6,0,1,26,1,0,1,21,2,23,1,31,4,23,1,31,3,23,1,31,
2,23,5,12,1,2,7,0,1,3,2,10,1,3,1,0,1,2,7,12,1,2,2,0,1,2,10,1,1,2,6,0,1,25,1,0,1,43,1,47,1,46,4,28,1,4,3,28,1,11,1,28,1,30,
1,29,5,1,1,2,1,0,1,3,3,10,1,3,1,0,1,2,2,1,1,2,1,0,1,2,7,1,1,2,4,0,1,3,4,10,1,3,7,10,1,3,9,0,1,26,4,0,1,28,1,30,1,29,
2,17,2,1,2,19,1,1,1,2,1,0,1,2,3,12,1,2,1,0,1,2,2,17,1,2,1,0,1,2,1,5,1,1,1,5,1,1,1,5,1,1,1,5,1,2,4,0,1,2,4,12,1,2,7,13,1,2,6,0,1,26,2,0,1,25,1,26,3,0,1,21,
4,17,2,1,2,19,1,1,1,2,1,0,1,2,3,1,1,2,1,49,1,53,2,17,1,48,1,0,1,2,1,7,1,1,1,7,1,1,1,7,1,1,1,7,1,2,4,0,1,2,4,12,1,2,7,13,1,2,6,0,1,25,1,26,2,0,1,25,2,0,1,26,1,21,
4,17,5,10,1,3,1,0,1,2,1,5,1,1,1,5,1,2,1,48,3,17,1,55,1,50,1,2,1,7,1,1,1,7,1,1,1,7,1,1,1,7,1,2,1,51,3,0,1,2,4,1,1,2,7,1,1,2,6,0,2,25,5,0,1,25,1,21,
4,17,5,12,1,2,1,0,1,2,1,7,1,1,1,7,1,2,1,53,14,17,1,55,1,51,2,0,1,2,4,1,1,2,2,19,3,1,2,19,1,2,4,0,1,45,9,20,
5,17,5,12,1,2,1,0,1,52,20,17,1,55,1,51,1,0,1,2,4,1,1,2,3,1,1,4,3,1,1,2,4,0,1,21,
14,17,5,1,1,2,1,0,1,48,9,17,1,49,1,50,1,54,1,51,8,17,1,52,19,0,1,21,
14,17,2,1,2,19,1,1,1,2,1,0,1,55,1,54,1,51,7,17,1,48,2,0,1,55,1,51,7,17,1,55,1,54,1,51,3,0,1,3,8,10,1,3,4,0,1,21,11,23,
3,17,2,1,2,19,1,1,1,2,3,0,1,55,1,50,1,54,1,50,1,54,1,50,1,54,1,50,1,53,3,0,1,55,1,54,1,50,1,54,1,50,1,51,4,17,1,48,3,0,1,2,8,13,1,2,4,0,1,43,1,47,1,46,5,28,1,4,1,28,1,30,1,29,
3,17,5,1,1,2,6,0,1,33,1,34,1,0,1,34,3,0,1,34,4,0,1,34,1,0,1,48,4,17,1,52,3,0,1,2,8,13,1,2,14,0,1,21,
4,17,8,10,1,3,3,0,1,33,4,0,1,34,1,33,3,0,1,34,3,0,1,48,4,17,1,48,3,0,1,2,8,1,1,2,14,0,1,21,
4,17,8,12,1,2,4,0,1,34,2,0,1,34,7,0,1,33,1,34,1,48,4,17,1,3,3,0,1,2,8,1,1,2,4,0,1,45,9,20,
5,17,8,12,1,2,3,0,1,34,3,0,1,34,1,0,1,34,4,0,1,33,2,0,1,55,1,51,3,17,1,32,3,0,1,2,1,1,1,14,5,1,1,14,1,2,4,0,1,21,7,17,1,26,
6,17,8,1,1,2,3,0,1,33,4,0,1,33,3,0,1,34,1,0,1,34,3,0,1,48,3,17,1,3,3,0,1,2,1,1,1,9,2,1,1,4,2,1,1,9,1,2,4,0,1,43,6,23,1,26,1,25,1,26,
5,17,8,1,1,2,2,0,1,33,4,0,1,34,3,0,1,33,1,0,1,34,4,0,1,52,3,17,1,32,24,0,3,25,1,21,
4,17,5,1,1,5,1,1,1,5,1,2,5,0,1,49,2,54,1,51,7,0,1,34,1,49,1,53,3,17,1,3,1,0,1,3,1,0,1,3,1,0,1,3,1,0,1,3,1,0,1,3,1,0,1,3,1,0,1,3,1,0,1,3,11,0,1,21,
4,17,5,1,1,7,1,1,1,7,1,2,2,54,1,50,1,54,1,50,1,53,2,17,1,55,2,50,1,54,2,50,1,54,2,50,1,53,4,17,1,2,1,20,1,2,1,20,1,2,1,20,1,2,1,20,1,2,1,20,1,2,1,20,1,2,1,20,1,2,1,20,1,2,11,20,
255,17,6,17,
};

#define COMPRESSED_SIZE 2652
//Keep track of this outside the function so we can search incrementally 
uint16_t world_ndx = 0; 
uint16_t world_cnt = 0;
//The world is now run-length encoded to save space, so this function is
//needed to retrieve the actual values for each tile in the map.  This
//can cause substantial overhead, so I keep track of the current index
//globally so that looping accesses to incremental indices don't have to
//loop far at all.
uint8_t world_get(uint16_t index){
  uint8_t curr = 0;
  uint8_t count = 0;

  //Check our global status; if we have already passed the tile, start over
  if( world_cnt > index ){
    world_ndx = 0;
    world_cnt = 0;
  }
  
  for(; world_ndx < COMPRESSED_SIZE; world_ndx += 2){
    curr = pgm_read_byte(&world[world_ndx+1]); //Get tile type
    count = pgm_read_byte(&world[world_ndx]); //Get tile count
    world_cnt += count;
    if( world_cnt > index ){ //If we have now encountered that index
      world_cnt -= count; //Roll back so we can quickly get a tile from the same block
      return curr;
    }
  }
  return 0; // off the map: blank tile
}

uint16_t dudex = 25*8;//10*8;//37*8;//25*8;//8*8;//25*8;
uint16_t dudey = 57*8;//26*8;//49*8;//57*8;//27*8;//57*8;

byte dudeanimation = DOWN;
byte dudeframe = 0;

byte worldframe = 0;

void draw_world(){
  uint8_t tile;
  //gb.println(dudex/8 - 8);
  //gb.display.println(dudey/8 - 4);
  //Signed because we may seek off the edge of the map
  for( int16_t y = dudey/8 - 4; y <= dudey/8 + 5; y++ ){
    for( int16_t x = dudex/8 < 8 ? 0 : dudex/8 - 8 ; x <= dudex/8 + 9; x++ ){
      //Draw the tile at the relative distance from the player
      tile = world_get(y*64+x);
      switch( tile ){
        case DRAIN:
        case FLOW:
        case WATER:
        case DOCK1:
        case DOCK2:
        case DOCK3:
          //First 4 are first frame, second 4 are second frame
          if( worldframe >= 0 && worldframe <= 3 ){
            gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[tile*8],8,8 );
          }else{
            //All animated tiles have their variation one tile after it
            gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[(tile+1)*8],8,8 );
          }
          break;
          
        default:
          //Draw extended water on the right side
          if( x > 63 ){
            //First 4 are first frame, second 4 are second frame
            if( worldframe >= 0 && worldframe <= 3 ){
              gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[WATER*8],8,8 );
            }else{
              //All animated tiles have their variation one tile after it
              gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[(WATER+1)*8],8,8 );
            }
          }else{
            gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[tile*8],8,8 );
          }
          //Draw duplicates of the leftmost tile to give the appearance of things continuing
          if( x == 0 ){
            gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4 - 8, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[tile*8],8,8 );
            gb.drawSlowXYBitmap( (x*8 - dudex) + SCREEN_WIDTH/2 - 4 - 16, (y*8 - dudey) + SCREEN_HEIGHT/2 - 4, &tiles[tile*8],8,8 );
          }
      } 
    }
  }
  //Draw player.  If moving and frames dictate it so, play walking animation
  if( player_moving && (dudeframe/2 == 0 || dudeframe/2 == 2) ){
    gb.drawSlowXYBitmap(SCREEN_WIDTH/2-4,SCREEN_HEIGHT/2-4,player_sprites+((dudeframe/2)+dudeanimation*3)*8, 8, 8);
  }else{
    gb.drawSlowXYBitmap(SCREEN_WIDTH/2-4,SCREEN_HEIGHT/2-4,player_sprites+(1+dudeanimation*3)*8, 8, 8);
  }
}

uint8_t test_world_collision(uint8_t dir){
  uint8_t tile = 0;
  if( dir == UP ){
    tile = world_get(((dudey-0)/8)*64+(dudex+1)/8);
    if( tile != DOOR && world_get(((dudey-0)/8)*64+(dudex+6)/8) )
      return world_get(((dudey-0)/8)*64+(dudex+6)/8);
    else
      return tile;
  }else if( dir == DOWN ){
    return world_get(((dudey+7)/8)*64+(dudex+1)/8) ||
      world_get(((dudey+7)/8)*64+(dudex+6)/8) != 0;
  }else if( dir == LEFT ){
    return world_get(((dudey+1)/8)*64+(dudex-0)/8) != 0 ||
      world_get(((dudey+6)/8)*64+(dudex-0)/8) != 0;
  }else{
    return world_get(((dudey+1)/8)*64+(dudex+7)/8) != 0 ||
      world_get(((dudey+6)/8)*64+(dudex+7)/8) != 0;
  }
}

void step_world(){
  player_moving = 0;
  if( gb.pressed(UP_BUTTON) ){
    uint8_t collision = 0;
    collision = test_world_collision(UP);
    if( collision == DOOR ){
      //Check whether we collided with a door based on our top left or top right
      collision = world_get(((dudey-1)/8)*64+dudex/8);
      //Depending on which, we set dungeon position to tile x/y of door.
      //This signals to the dungeon generator which dungeon we want to enter.
      uint8_t dungeonx,dungeony;
      if( collision == DOOR ){
        dungeonx = dudex/8;
        dungeony = ((dudey-1)/8);
      }else{
        dungeonx = (dudex+7)/8;
        dungeony = ((dudey-1)/8);
      }
      // Identify the dungeon we are entering
      for( uint8_t i = 0; i < NUM_DUNGEONS; i++ ){
        if( pgm_read_byte(&(dungeons[i].x)) == dungeonx && pgm_read_byte(&(dungeons[i].y)) == dungeony ){
          dungeonid = i;
          break;
        }
      }
      mode = TO_DUNGEON;
      transition = -SCREEN_HEIGHT/2;
      persistence = true;
      dungeon_generated = 0;
      dungeon_level = 0;
      previous_level = -1;
      return;
    }
    if( !collision ){
      dudey--;
      dudeanimation = UP;
      player_moving = 1;
    }
  }
  else if(gb.pressed(DOWN_BUTTON) && !test_world_collision(DOWN)){
    dudey++;
    dudeanimation = DOWN;
    player_moving = 1;
  }
  if(gb.pressed(LEFT_BUTTON) && !test_world_collision(LEFT)){
    dudex--;
    dudeanimation = LEFT;
    player_moving = 1;
  }
  else if(gb.pressed(RIGHT_BUTTON) && !test_world_collision(RIGHT)){
    dudex++;
    dudeanimation = RIGHT;
    player_moving = 1;
  }

  if( player_moving ){
    try_combat();
  }
}


#include "save.h"
#include "battle.h"
#include <EEPROM.h>

#define SAVE_BASE 79

//All game status variables
int8_t game_status[] = {-1,0,0,0,0,0};

void save_game(){
  uint8_t i;
  uint8_t base = SAVE_BASE+1;
  EEPROM.update(SAVE_BASE,'T');
  //Save game status
  for( i = 0; i < 6; i++ ){
    EEPROM.update(i+base,game_status[i]);
  }
  base = SAVE_BASE + 7;
  //Save each character's stats
  for( i = 0; i < 3; i++ ){
      EEPROM.update(base++,party[i].level);
      EEPROM.update(base++,*((uint8_t*)&party[i].health));
      EEPROM.update(base++,*(((uint8_t*)&party[i].health)+1));
      EEPROM.update(base++,party[i].xp);
  }
  base = SAVE_BASE + 7 + 12;
  //Save inventory
  for( i = 0; i < INVENTORY_SIZE; i++ ){
    EEPROM.update(base++,inventory[i]);
  }
  base = SAVE_BASE + 7 + 12 + INVENTORY_SIZE;
  //Save player coords
  EEPROM.update(base++,(dudex+2)/8);
  EEPROM.update(base,(dudey+2)/8);
}

void restore_game(){
  uint8_t i;
  uint8_t base = SAVE_BASE+1;
  if( EEPROM.read(SAVE_BASE) == 'T' ){
    //Save game status
    for( i = 0; i < 6; i++ ){
      game_status[i] = EEPROM.read(i+base);
    }
    base = SAVE_BASE + 7;
    //Load each character's stats
    for( i = 0; i < 3; i++ ){
      party[i].level = EEPROM.read(base++);
      *((uint8_t*)&party[i].health) = EEPROM.read(base++);
      *(((uint8_t*)&party[i].health)+1) = EEPROM.read(base++);
      party[i].xp = EEPROM.read(base++);
    }
    base = SAVE_BASE + 7 + 12;
    //Save inventory
    for( i = 0; i < INVENTORY_SIZE; i++ ){
      inventory[i] = EEPROM.read(base++);
    }
    base = SAVE_BASE + 7 + 12 + INVENTORY_SIZE;
    //Load player coords
    dudex = EEPROM.read(base++)*8;
    dudey = EEPROM.read(base)*8;
    //Switch modes to start up game
    mode = TO_WORLD;
  }
}

