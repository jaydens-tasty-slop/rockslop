#include <stdlib.h>
#include <math.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <ArduboyTonesPitches.h>
#include <avr/pgmspace.h>
#include "../lib/ab_math.h"


Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

#define N_PIECES 6
#define DOWN 0
#define B 1
#define HIPPODROME_MARK_MEMORY 0x77
#define MILLIS_END_GAME 1200.0
#define MILLIS_BACK_MENU 1500.0

enum PIECE {PAWN, BISHOP, KNIGHT, KING, QUEEN, ROOK, NONE};
PIECE init_board[16] = {
  KING, KING, QUEEN, QUEEN,
  ROOK, ROOK, BISHOP, BISHOP,
  ROOK, ROOK, BISHOP, BISHOP,
  KNIGHT, KNIGHT, KNIGHT, KNIGHT  
};

byte init_color[16] = {
  BLACK, WHITE, BLACK, WHITE,
  BLACK, WHITE, BLACK, WHITE,
  BLACK, WHITE, BLACK, WHITE,
  BLACK, WHITE, BLACK, WHITE  
};

struct hippodrome {
  PIECE board[16] = {
    NONE, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE
  };
  byte color[16] = {
    BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK,
  };
  short int movements[16];
  int n_movements = 0;
  short int position_movements = -1;
  int movements_done = 0;
  unsigned long time_init = 0;
  byte color_init = WHITE;
} hippodrome;

typedef struct score {
  int movs;
  int seconds;
  short int milliseconds;
} score;

score score_board[6];
byte n_scores = 0;
score last_score;
bool is_new_record = false;
bool from_game = false;

bool clear_screen = true;
bool is_pressed_left = false;
bool is_pressed_right = false;
bool is_pressed_down = false;
bool is_pressed_up = false;
bool is_pressed_a = false;
bool is_pressed_b = false;
enum GAME_STATE {MENU, GAME, RECORDS, INFO, END_GAME, ACHIEVEMENTS};
GAME_STATE mode = MENU;
bool active_sound;

PIECE info_piece = NONE;
byte info_color = BLACK;

byte colors_end_game[6] = {BLACK, BLACK, BLACK, BLACK, BLACK, BLACK};
short int actual_piece_end_game = -1;

// Type pieces (save memory with byte)
byte all_pieces[] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
byte pieces_menu[8] = {NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE};

byte colors_menu[8] = {BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK};
bool visible_pieces[8] = {false, false, false, false, false, false, false, false};
bool set_true = true;
const char *options[3] = {"start", "score", "about"};
byte option = 0;
unsigned long last_milliseconds = -1;
bool mode_wait = false;
short int menu_n_piece = -1;
byte key_menu = 0b11111111;
bool wait_end_game = false;
unsigned long last_milliseconds_game = -1;


unsigned char achievements1 = 0;
unsigned char achievements2 = 0;
unsigned char achievements3 = 0;
unsigned char achievements4 = 0;


bool achievements_completed[12];
short int achievements_position = 0;
short int last_achievement_read = -1;
char achievement_description[71];


// Achievements 1
const char string_0[] PROGMEM = "Get  5  scores   lessthan 30 seconds";
const char string_1[] PROGMEM = "Finish  a  game  witheach  knight  in  itscolor";
const char string_2[] PROGMEM = "Finish  a  game  withthe  4  bishops  in arow and in its color";
const char string_3[] PROGMEM = "Finish  a  game  withthe 4  rooks in a rowand in its color";
const char string_4[] PROGMEM = "Finish a game in lessthan 25 moves";
const char string_5[] PROGMEM = "Finish a game in lessthan 25 seconds";

const char string_6[] PROGMEM = "Get  5  scores   lessthan 25 seconds";
const char string_7[] PROGMEM = "Finish  a  game  with2 kings  and 2 queensin    the    startingsquares";
const char string_8[] PROGMEM = "Finish  a  game  withthe 4 bishops forminga  square  and in itscolor";
const char string_9[] PROGMEM = "Finish  a  game  withthe 4 rooks forming asquare   and  in  itscolor";
const char string_10[] PROGMEM = "Finish a game in lessthan 22 moves";
const char string_11[] PROGMEM = "Finish a game in lessthan 22 seconds";


// Achievements 2
const char string_12[] PROGMEM = "Unlock    all   basicachievements";
const char string_13[] PROGMEM = "Finish a game between150  and  151 seconds";
const char string_14[] PROGMEM = "Finish  a  game  with4 rooks and 4 bishopsin their color";
const char string_15[] PROGMEM = "Finish  a  game  withthe 4 rooks and the 4bishops in squares";
const char string_16[] PROGMEM = "Finish a game in lessthan 20 moves";
const char string_17[] PROGMEM = "Finish a game in lessthan 20 seconds";

const char string_18[] PROGMEM = "Finish  a  game  withnumber  of  movementsequal to the seconds";
const char string_19[] PROGMEM = "Finish a game in  150moves";
const char string_20[] PROGMEM = "Finish  a  game  withall the pieces in theopposite color";
const char string_21[] PROGMEM = "Finish  a  game  with4 rooks in a row  and4 bishops in another";
const char string_22[] PROGMEM = "Finish a game in lessthan 18 moves";
const char string_23[] PROGMEM = "Finish a game in lessthan 18 seconds";




const char *const achievements_descriptions[] PROGMEM = {
  string_0, string_1, string_2, string_3,
  string_4, string_5, string_6, string_7,
  string_8, string_9, string_10, string_11,
  string_12, string_13, string_14, string_15,
  string_16, string_17, string_18, string_19,
  string_20, string_21, string_22, string_23
};



// Music
const uint16_t blues_notes[N_PIECES] = {
  NOTE_C5, NOTE_DS5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_AS5
};

const uint16_t music_new_achievement[] PROGMEM = {
  NOTE_FS5, 124, NOTE_A5, 124, NOTE_B5, 124, NOTE_FS5, 124, NOTE_A5, 124, NOTE_B5, 124, NOTE_E6, 248, TONES_END
};

const uint16_t music_end[] PROGMEM = {
  NOTE_FS5, 124, NOTE_B5, 124, NOTE_E6, 248, TONES_END
};

const uint16_t music_menu[] PROGMEM = {
  NOTE_E4,62, NOTE_REST,62, NOTE_G4,62, NOTE_REST,62, NOTE_FS4,62, NOTE_REST,62, NOTE_A4,62, NOTE_REST,62,
  NOTE_E4,62, NOTE_REST,62, NOTE_B4,62, NOTE_REST,62, NOTE_G4,62, NOTE_REST,62, NOTE_FS4,62, NOTE_REST,62,
  TONES_REPEAT
};

const uint16_t music_end_game[] PROGMEM = {
  NOTE_C4, 62, NOTE_REST, 62, NOTE_DS4, 62, NOTE_REST, 62,
  NOTE_F4, 62, NOTE_REST, 62, NOTE_FS4, 62, NOTE_REST, 62,
  NOTE_G4, 62, NOTE_REST, 62, NOTE_AS4, 62, NOTE_REST, 62,
  NOTE_G4, 62, NOTE_REST, 62, NOTE_FS4, 62, NOTE_REST, 62,
  NOTE_G4, 62, NOTE_REST, 62, NOTE_FS4, 62, NOTE_REST, 62,
  NOTE_F4, 62, NOTE_REST, 62, NOTE_DS4, 62, NOTE_REST, 62,
  TONES_REPEAT
};





// Sprites
const unsigned char PROGMEM knight_back[] = {
  0x00, 0x00, 0x3e, 0x3e, 0xbe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM knight_fore[] = {
  0x00, 0x00, 0x00, 0x1c, 0x1c, 0x9c, 0xfc, 0xfc, 0xe4, 0xfc, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x38, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3e, 0x38, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM queen_back[] = {
  0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM queen_fore[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xdc, 0x04, 0xdc, 0xfc, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x3c, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3c, 0x30, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM bishop_back[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfc, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM bishop_fore[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3c, 0x30, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM pawn_back[] = {
  0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM pawn_fore[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xe4, 0xfc, 0xe4, 0xfc, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x3c, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3c, 0x30, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM rook_back[] = {
  0x00, 0x00, 0x00, 0x3e, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0x3e, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM rook_fore[] = {
  0x00, 0x00, 0x00, 0x00, 0x1c, 0xfc, 0xf0, 0xfc, 0xfc, 0xf0, 0xfc, 0x1c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x3c, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3c, 0x30, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM king_back[] = {
  0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x78, 0x00, 0x00, 
};

const unsigned char PROGMEM king_fore[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x70, 0x1c, 0x7c, 0xf0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x3c, 0x3f, 0x3f, 0x3c, 0x3f, 0x3f, 0x3f, 0x3c, 0x30, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM square_field[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
};

const unsigned char PROGMEM select_bmp[] = {
  0xc7, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xc7, 
  0xe3, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0xe3, 
};

const unsigned char PROGMEM none[] = {
  0x00, 0x00, 0x10, 0x88, 0x44, 0x20, 0x10, 0x08, 0x84, 0x42, 0x20, 0x10, 0x08, 0x84, 0x42, 0x00, 
  0x00, 0x42, 0x21, 0x10, 0x08, 0x04, 0x42, 0x21, 0x10, 0x08, 0x04, 0x22, 0x11, 0x08, 0x00, 0x00, 
};

const unsigned char PROGMEM possible[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x07, 
  0xe0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

const unsigned char PROGMEM title[] = {
  0xff, 0x01, 0x01, 0x9f, 0x9f, 0x01, 0x01, 0xff, 0x09, 0xf9, 0x01, 0x01, 0xf9, 0x09, 0xff, 0x01, 0x01, 0x99, 0x99, 0x81, 0x81, 0xff, 0x01, 0x01, 0x99, 0x99, 0x81, 0x81, 0xff, 0x01, 0x01, 0xf9, 0xf9, 0x01, 0x01, 0xff, 0x01, 0x01, 0xf9, 0xf9, 0x07, 0x04, 0xff, 0x01, 0x01, 0x99, 0x99, 0x61, 0x61, 0xff, 0x01, 0x01, 0xf9, 0xf9, 0x01, 0x01, 0xff, 0x01, 0x01, 0xe7, 0xe4, 0x9c, 0x9c, 0xe4, 0xe7, 0x01, 0x01, 0xff, 0x01, 0x01, 0x99, 0x99, 0x99, 0x99, 0xff, 
  0x0f, 0x08, 0x08, 0x0f, 0x0f, 0x08, 0x08, 0x0f, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09, 0x0f, 0x08, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x0f, 0x08, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x0f, 0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x0f, 0x08, 0x08, 0x09, 0x09, 0x0e, 0x02, 0x0f, 0x08, 0x08, 0x0f, 0x0f, 0x08, 0x08, 0x0f, 0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x0f, 0x08, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x08, 0x08, 0x0f, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x0f, 
};

const unsigned char PROGMEM note_back[] = {
  0x00, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0x00, 0x00
};

const unsigned char PROGMEM note_fore[] = {
  0x00, 0x00, 0x60, 0x60, 0x7e, 0x00, 0x00, 0x00
};









// Utils

void draw_piece(const unsigned char* back, const unsigned char* fore, int x, int y, int color);
void draw_knight(int x, int y, int color);
void draw_king(int x, int y, int color);
void draw_bishop(int x, int y, int color);
void draw_pawn(int x, int y, int color);
void draw_rook(int x, int y, int color);
void draw_queen(int x, int y, int color);
void draw_field(int x, int y, int color);
void draw_select(int x, int y, int color);
void draw_none(int x, int y, int color);
void draw_possible(int x, int y, int color);
void draw_title(int x, int y, int color);
void draw_note(int x, int y, int color);
void draw_piece_type(PIECE piece, int x, int y, int color);
void draw_board();
void draw_pieces();
void draw_end();
void draw_game();
void draw_name_piece();
void draw_time(int seconds, int milliseconds);
void draw_time();
void draw_movements();
void load_memory();
void initialize_memory();
void save_settings();
void save_score();
void initialize_menu();
void initialize_mode_game();
void initialize_end_game();
void initialize_achievements();
void initialize_info();
void mode_end_game();
void mode_info();
void mode_records();
void mode_game();
void mode_menu();
void mode_achievements();
void sort_scores();
void change_mode_info();
void change_mode_records();
void change_mode_end_game();
void change_mode_menu();
void change_mode_game();
void change_mode_achievements();

template <class T> int EEPROM_write(int ee, const T& value);
template <class T> int EEPROM_read(int ee, T& value);

bool add_key_menu(byte key) {
  key_menu = (key_menu << 1) + key;
  return (key_menu & 0b1111) == 0b0101 || (key_menu & 0b111111) == 0b001001;
}

bool key_basic() {
  return (key_menu & 0b1111) == 0b0101; 
}

int read_bit(int n, unsigned char x) {
  return x & (1 << n);
}

unsigned char write_bit_1(int n, unsigned char x) {
  return x | (1 << n);
}

int negate_color(int color) {
  return color == WHITE ? BLACK : WHITE;  
}

int get_color(int x, int y, int color_init) {
  int color = y % 2 ? color_init : negate_color(color_init);
  color = x % 2 ? color : negate_color(color);
  return color;  
}

void sort_movements() {
  for(int i = 0; i < hippodrome.n_movements; i++)
    for(int j = i+1; j < hippodrome.n_movements; j++)
      if(hippodrome.movements[i] > hippodrome.movements[j]) {
        int aux = hippodrome.movements[i];
        hippodrome.movements[i] = hippodrome.movements[j];
        hippodrome.movements[j] = aux;
      }
}

PIECE get_piece(int x, int y) {
   if(x < 0 || x >= 4 || y < 0 || y >= 4)
    return NONE;
   
   int n = y * 4 + x;
   return hippodrome.board[n];
}

bool check(int x, int y, int n, PIECE* pieces) {
  PIECE piece = get_piece(x, y);
  for(int i = 0; i < n; i++)
    if(pieces[i] == piece)
      return true;
  return false;
}

void push_movement(int n) {
  hippodrome.movements[hippodrome.n_movements] = n;
  hippodrome.n_movements++;
}

void check_movements() {
  hippodrome.n_movements = 0;
  hippodrome.position_movements = 0;
  int n = 0;
  for(int i = 0; i < 16; i++) {
    if(hippodrome.board[i] == NONE) {
      n = i;
      break;  
    }
  }
  
  int x = n % 4;
  int y = n / 4;
  PIECE piece;
  // Check rook, queen, king
  PIECE rqk[3] = {ROOK, QUEEN, KING};
  if( check(x-1, y, 3, rqk) )
    push_movement(y * 4 + (x-1));
  if( check(x+1, y, 3, rqk) ) 
    push_movement(y * 4 + (x+1));
  if( check(x, y-1, 3, rqk) )
    push_movement((y-1) * 4 + x);
  if( check(x, y+1, 3, rqk) )
    push_movement((y+1) * 4 + x);
  
  // Check bishop, queen, king
  PIECE bqk[3] = {BISHOP, QUEEN, KING};
  if( check(x-1, y-1, 3, bqk) )
    push_movement((y - 1) * 4 + (x - 1));
  if( check(x+1, y-1, 3, bqk) )
    push_movement((y - 1) * 4 + (x + 1));
  if( check(x-1, y+1, 3, bqk) )
    push_movement((y + 1) * 4 + (x - 1));
  if( check(x+1, y+1, 3, bqk) )
    push_movement((y + 1) * 4 + (x + 1));


  // Check knight
  PIECE k[1] = {KNIGHT};
  if( check(x-1, y-2, 1, k) )
    push_movement((y - 2) * 4 + (x - 1));
  if( check(x-1, y+2, 1, k) )
    push_movement((y + 2) * 4 + (x - 1));
  if( check(x+1, y-2, 1, k) )
    push_movement((y - 2) * 4 + (x + 1));
  if( check(x+1, y+2, 1, k) )
    push_movement((y + 2) * 4 + (x + 1));

  if( check(x-2, y-1, 1, k) )
    push_movement((y - 1) * 4 + (x - 2));
  if( check(x-2, y+1, 1, k) )
    push_movement((y + 1) * 4 + (x - 2));
  if( check(x+2, y-1, 1, k) )
    push_movement((y - 1) * 4 + (x + 2));
  if( check(x+2, y+1, 1, k) )
    push_movement((y + 1) * 4 + (x + 2));

  sort_movements();
}

void move_select_piece() {
  int n = 0;
  for(int i = 0; i < 16; i++)
    if(hippodrome.board[i] == NONE) {
      n = i;
      break;
    }

  int select = hippodrome.movements[hippodrome.position_movements];
  hippodrome.color[n] = hippodrome.color[select];
  hippodrome.board[n] = hippodrome.board[select];
  hippodrome.board[select] = NONE;

  hippodrome.movements_done = min(hippodrome.movements_done+1, 999);

  check_movements();
}

bool check_end() {
  for(int i = 0; i < 4; i++)
    if(hippodrome.board[i] != KNIGHT)
      return false;
  return true;
}

void initialize_board() {
  for(int i = 0; i < 16; i++) {
    hippodrome.board[i] = init_board[i];
    hippodrome.color[i] = init_color[i];  
  }

  for(int i = 0; i < 12; i++) {
    int n = random(0, 12);
    int color = hippodrome.color[n];
    PIECE piece = hippodrome.board[n];
    hippodrome.color[n] = hippodrome.color[i];
    hippodrome.board[n] = hippodrome.board[i];
    hippodrome.color[i] = color;
    hippodrome.board[i] = piece;
  }

  for(int i = 12; i < 16; i++) {
    int n = random(12, 16);
    int color = hippodrome.color[n];
    PIECE piece = hippodrome.board[n];
    hippodrome.color[n] = hippodrome.color[i];
    hippodrome.board[n] = hippodrome.board[i];
    hippodrome.color[i] = color;
    hippodrome.board[i] = piece;
  }

  int n = random(0, 12);
  hippodrome.board[n] = NONE;
}

void initialize_hippodrome() {
  hippodrome.n_movements = 0;
  while(hippodrome.n_movements == 0) {
    initialize_board();
    hippodrome.movements_done = 0;
    check_movements();
    hippodrome.time_init = millis();
    //hippodrome.color_init = random(0, 2);
  }
}

bool add_score(int seconds, int milliseconds, int movs) {
  bool is_new = false;
  int n = n_scores++;
  score_board[n].movs = movs;
  score_board[n].seconds = seconds;
  score_board[n].milliseconds = milliseconds;
  sort_scores();

  for(int i = 0; i < min(n_scores, 5) && !is_new; i++)
    if(score_board[i].movs == movs && score_board[i].seconds == seconds && score_board[i].milliseconds == milliseconds)
      is_new = true;

  n_scores = min(n_scores, 5);
  if(is_new)
    save_score();
  return is_new;
}

void sort_scores() {
  score aux;
  for(int i = 0; i < n_scores; i++) {
    for(int j = i+1; j < n_scores; j++) {
      if(score_board[i].seconds > score_board[j].seconds) {
        aux = score_board[i];
        score_board[i] = score_board[j];
        score_board[j] = aux;
      }
      else if(score_board[i].seconds == score_board[j].seconds) {
        if(score_board[i].milliseconds > score_board[j].milliseconds) {
          aux = score_board[i];
          score_board[i] = score_board[j];
          score_board[j] = aux;
        }
        else if(score_board[i].milliseconds == score_board[j].milliseconds) {
          if(score_board[i].movs > score_board[j].movs) {
            aux = score_board[i];
            score_board[i] = score_board[j];
            score_board[j] = aux;
          }
        }
      }
     }
  }  
}

void initialize_pieces_menu() {
  for(int i = 0; i < 8; i++) {
    int n = random(0, N_PIECES);
    PIECE piece = all_pieces[n];
    pieces_menu[i] = piece;
    colors_menu[i] = random(0, 2);
    visible_pieces[i] = false;
  }
  set_true = true;
}


bool pressed_a() {
  if(!is_pressed_a && arduboy.pressed(A_BUTTON)) {
    is_pressed_a = true;
    return true;
  }
  return false;
}

bool pressed_b() {
  if(!is_pressed_b && arduboy.pressed(B_BUTTON)) {
    is_pressed_b = true;
    return true;
  }
  return false;
}

bool pressed_up() {
  if(!is_pressed_up && arduboy.pressed(UP_BUTTON)) {
    is_pressed_up = true;
    return true;
  }
  return false;
}

bool pressed_down() {
  if(!is_pressed_down && arduboy.pressed(DOWN_BUTTON)) {
    is_pressed_down = true;
    return true;
  }
  return false;
}

bool pressed_left() {
  if(!is_pressed_left && arduboy.pressed(LEFT_BUTTON)) {
    is_pressed_left = true;
    return true;
  }
  return false;
}

bool pressed_right() {
  if(!is_pressed_right && arduboy.pressed(RIGHT_BUTTON)) {
    is_pressed_right = true;
    return true;
  }
  return false;
}

bool check_achievements() {

  bool completed = false;
  byte a[] = {achievements1, achievements2, achievements3, achievements4};

  // Achievements 1

  // Get 5 scores less than 30 seconds
  completed = n_scores >= 5;
  for(int i = 0; i < n_scores && completed; i++)
    completed = score_board[i].seconds < 30;

  if(completed)
    achievements1 = write_bit_1(0, achievements1);


  // Finish a game with each knight in its color
  completed = true;
  for(int i = 0; i < 4 && completed; i++)
    completed = hippodrome.color[i] == get_color(0, i, hippodrome.color_init);

  if(completed)
    achievements1 = write_bit_1(1, achievements1);

  
  // Finish a game with the 4 bishops in a row and in its color
  for(int i = 0; i < 4; i++) {
    completed = true;
    for(int j = 0; j < 4 && completed; j++) {
      completed = hippodrome.board[i*4+j] == BISHOP && hippodrome.color[i*4+j] == get_color(i, j, hippodrome.color_init);
    }
    if(completed) break;
  }

  if(completed)
    achievements1 = write_bit_1(2, achievements1);

  
  // Finish a game with the 4 rooks in a row and in its color
  for(int i = 0; i < 4; i++) {
    completed = true;
    for(int j = 0; j < 4 && completed; j++) {
      completed = hippodrome.board[i*4+j] == ROOK && hippodrome.color[i*4+j] == get_color(i, j, hippodrome.color_init);
    }
    if(completed) break;
  }

  if(completed)
    achievements1 = write_bit_1(3, achievements1);


  // Finish a game in less than 25 moves
  completed = last_score.movs < 25;
  for(int i = 0; i < n_scores && !completed; i++)
    completed = score_board[i].movs < 25;

  if(completed)
    achievements1 = write_bit_1(4, achievements1);


  // Finish a game in less than 25 seconds
  completed = false;
  for(int i = 0; i < n_scores && !completed; i++)
    completed = score_board[i].seconds < 25;

  if(completed)
    achievements1 = write_bit_1(5, achievements1);


  // Get 5 scores less than 25 seconds
  completed = n_scores >= 5;
  for(int i = 0; i < n_scores && completed; i++)
    completed = score_board[i].seconds < 25;

  if(completed)
    achievements2 = write_bit_1(0, achievements2);

  
  // Finish a game with 2 kings and 2 queens in the starting squares
  completed = true;
  for(int i = 12; i < 16 && completed; i++)
    completed = hippodrome.board[i] == QUEEN || hippodrome.board[i] == KING;

  if(completed)
    achievements2 = write_bit_1(1, achievements2);

  
  // Finish a game with the 4 bishops forming a square and in its color
  completed = false;
  for(int i = 0; i < 16; i++) {
    if(hippodrome.board[i] == BISHOP) {
      int x = i % 4;
      int y = i / 4;
      if(x > 2 || y > 2) break;
      completed = hippodrome.color[i] == get_color(x, y, hippodrome.color_init);
      completed = completed && hippodrome.board[i+1] == BISHOP && hippodrome.color[i+1] == get_color(x+1, y, hippodrome.color_init);
      completed = completed && hippodrome.board[i+4] == BISHOP && hippodrome.color[i+4] == get_color(x, y+1, hippodrome.color_init);
      completed = completed && hippodrome.board[i+5] == BISHOP && hippodrome.color[i+5] == get_color(x+1, y+1, hippodrome.color_init);
      break;
    }
  }
  
  if(completed)
    achievements2 = write_bit_1(2, achievements2);

  
  // Finish a game with the 4 rooks forming a square and in its color
  completed = false;
  for(int i = 0; i < 16; i++) {
    if(hippodrome.board[i] == ROOK) {
      int x = i % 4;
      int y = i / 4;
      if(x > 2 || y > 2) break;
      completed = hippodrome.color[i] == get_color(x, y, hippodrome.color_init);
      completed = completed && hippodrome.board[i+1] == ROOK && hippodrome.color[i+1] == get_color(x+1, y, hippodrome.color_init);
      completed = completed && hippodrome.board[i+4] == ROOK && hippodrome.color[i+4] == get_color(x, y+1, hippodrome.color_init);
      completed = completed && hippodrome.board[i+5] == ROOK && hippodrome.color[i+5] == get_color(x+1, y+1, hippodrome.color_init);
      break;
    }
  }
  
  if(completed)
    achievements2 = write_bit_1(3, achievements2);

  
  // Finish a game in less than 22 moves
  completed = last_score.movs < 22;
  for(int i = 0; i < n_scores && !completed; i++)
    completed = score_board[i].movs < 22;

  if(completed)
    achievements2 = write_bit_1(4, achievements2);

  
  // Finish a game in less than 22 seconds
  completed = false;
  for(int i = 0; i < n_scores && !completed; i++)
    completed = score_board[i].seconds < 22;

  if(completed)
    achievements2 = write_bit_1(5, achievements2);



  // Achievements 2

  // Unlock all basic achievements
  completed = achievements1 == 0b111111 && achievements2 == 0b111111;
  if(completed)
    achievements3 = write_bit_1(0, achievements3);

  
  // Finish a game between 150 and 151 seconds
  completed = last_score.seconds == 150 && last_score.milliseconds > 0;
  if(completed)
    achievements3 = write_bit_1(1, achievements3);

  
  // Finish a game with 4 rooks and 4 bishops in their color
  int count = 0;
  completed = true;
  for(int i = 0; i < 16 && completed; i++) {
    PIECE piece = hippodrome.board[i];
    int x = i % 4;
    int y = i / 4;
    if(piece != ROOK && piece != BISHOP) continue;
    count++;
    if(get_color(x, y, hippodrome.color_init) != hippodrome.color[i])
      completed = false;
  }
  if(completed && count == 8)
    achievements3 = write_bit_1(2, achievements3);

  
  // Finish a game with the 4 rooks and the 4 bishops in squares
  completed = true;
  for(int i = 0; i < 16 && completed; i++) {
    if(hippodrome.board[i] == ROOK) {
      int x = i % 4;
      int y = i / 4;
      if(x > 2 || y > 2) { completed = false; break; }
      completed = hippodrome.board[i+1] == ROOK;
      completed = completed && hippodrome.board[i+4] == ROOK;
      completed = completed && hippodrome.board[i+5] == ROOK;
      break;
    }
  }
  for(int i = 0; i < 16 && completed; i++) {
    if(hippodrome.board[i] == BISHOP) {
      int x = i % 4;
      int y = i / 4;
      if(x > 2 || y > 2) { completed = false; break; }
      completed = hippodrome.board[i+1] == BISHOP;
      completed = completed && hippodrome.board[i+4] == BISHOP;
      completed = completed && hippodrome.board[i+5] == BISHOP;
      break;
    }
  }
  if(completed)
    achievements3 = write_bit_1(3, achievements3);
  
  
  // Finish a game in less than 20 moves
  completed = last_score.movs < 20;
  if(completed)
    achievements3 = write_bit_1(4, achievements3);

  
  // Finish a game in less than 20 seconds
  completed = last_score.seconds < 20;
  if(completed)
    achievements3 = write_bit_1(5, achievements3);

  
  // Finish a game with number of movements equal to the seconds
  completed = last_score.seconds == last_score.movs;
  if(completed)
    achievements4 = write_bit_1(0, achievements4);

  
  // Finish a game in 150 moves
  completed = last_score.movs == 150;
  if(completed)
    achievements4 = write_bit_1(1, achievements4);

  
  // Finish a game with all the pieces in the opposite color
  completed = true;
  for(int i = 0; i < 16 && completed; i++) {
    if(hippodrome.board[i] == NONE) continue;
    int x = i % 4;
    int y = i / 4;
    completed = hippodrome.color[i] != get_color(x, y, hippodrome.color_init);
  }
  if(completed)
    achievements4 = write_bit_1(2, achievements4);

  
  // Finish a game with 4 rooks in a row and 4 bishops in another
  bool bishop = false;
  bool rook = false;
  for(int i = 0; i < 4; i++) {
    if(hippodrome.board[i*4] == ROOK)
      rook = hippodrome.board[i*4+1] == ROOK && hippodrome.board[i*4+2] == ROOK && hippodrome.board[i*4+3] == ROOK;
    else if(hippodrome.board[i*4] == BISHOP)
      bishop = hippodrome.board[i*4+1] == BISHOP && hippodrome.board[i*4+2] == BISHOP && hippodrome.board[i*4+3] == BISHOP;
  }
  completed = bishop && rook;
  if(completed)
    achievements4 = write_bit_1(3, achievements4);

  
  // Finish a game in less than 18 moves
  completed = last_score.movs < 18;
  if(completed)
    achievements4 = write_bit_1(4, achievements4);

  
  // Finish a game in less than 18 seconds
  completed = last_score.seconds < 18;
  if(completed)
    achievements4 = write_bit_1(5, achievements4);



  return a[0] != achievements1 || a[1] != achievements2 || a[2] != achievements3 || a[3] != achievements4;
}










// Draw functions

void draw_piece(const unsigned char* back, const unsigned char* fore, int x, int y, int color) {
  arduboy.drawBitmap(x, y, back, 16, 16, color == WHITE ? BLACK : WHITE);
  arduboy.drawBitmap(x, y, fore, 16, 16, color);
}

void draw_knight(int x, int y, int color) {
  draw_piece(knight_back, knight_fore, x, y, color);
}

void draw_king(int x, int y, int color) {
  draw_piece(king_back, king_fore, x, y, color);
}

void draw_bishop(int x, int y, int color) {
  draw_piece(bishop_back, bishop_fore, x, y, color);
}

void draw_pawn(int x, int y, int color) {
  draw_piece(pawn_back, pawn_fore, x, y, color);
}

void draw_rook(int x, int y, int color) {
  draw_piece(rook_back, rook_fore, x, y, color);
}

void draw_queen(int x, int y, int color) {
  draw_piece(queen_back, queen_fore, x, y, color);
}

void draw_field(int x, int y, int color) {
  arduboy.drawBitmap(x, y, square_field, 16, 16, color);
}

void draw_select(int x, int y, int color) {
  arduboy.drawBitmap(x, y, select_bmp, 16, 16, color);
}

void draw_none(int x, int y, int color) {
  return;
  arduboy.drawBitmap(x, y, none, 16, 16, color);
}

void draw_possible(int x, int y, int color) {
  arduboy.drawBitmap(x, y, possible, 16, 16, color);
}

void draw_title(int x, int y, int color) {
  arduboy.drawBitmap(x, y, title, 75, 12, color);
}

void draw_note(int x, int y, int color) {
  arduboy.drawBitmap(x, y, note_back, 8, 8, color);
  arduboy.drawBitmap(x, y, note_fore, 8, 8, negate_color(color));
}

void draw_piece_type(PIECE piece, int x, int y, int color) {
  switch(piece) {
    case NONE:
      //draw_none(x, y, color);
      break;
    case QUEEN:
      draw_queen(x, y, color);
      break;
    case KNIGHT:
      draw_knight(x, y, color);
      break;
    case ROOK:
      draw_rook(x, y, color);
      break;
    case PAWN:
      draw_pawn(x, y, color);
      break;
    case BISHOP:
      draw_bishop(x, y, color);
      break;
    case KING:
      draw_king(x, y, color);
      break;
  }
}

void draw_board() {
  int color = hippodrome.color_init;
  int init = color;
  for(int i = 0; i < 4; i++)
    for(int j = 0; j < 4; j++)
      draw_field(j * 16, i * 16, get_color(j, i, color));
}

void draw_pieces() {
  int n = 0;
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
        n = i * 4 + j;
        if(hippodrome.board[n] != NONE)
          draw_piece_type(hippodrome.board[n], j * 16, i * 16, hippodrome.color[n]);
        else
          draw_none(j * 16, i * 16, negate_color(get_color(j, i, hippodrome.color_init)));
    }  
  }
}

void draw_end() {
  draw_board();
  draw_pieces();
  draw_name_piece();
}

void draw_game() {
  draw_board();
  draw_pieces();
  draw_name_piece();
  draw_movements();
  draw_time();

  for(int i = 0; i < hippodrome.n_movements; i++) {
    int n = hippodrome.movements[i];
    int x = n % 4;
    int y = n / 4;
  
    draw_possible(x * 16, y * 16, negate_color(get_color(x, y, hippodrome.color_init)));
  }

  int n = hippodrome.movements[hippodrome.position_movements];
  int x = n % 4;
  int y = n / 4;

  draw_select(x * 16, y * 16, negate_color(get_color(x, y, hippodrome.color_init)));
}

void draw_name_piece() {
  int n = hippodrome.movements[hippodrome.position_movements];
  PIECE piece = hippodrome.board[n];
  
  arduboy.setCursor(80, 5);

  switch(piece) {
    case ROOK:
      arduboy.print(F("Rook"));
      break;
    case BISHOP:
      arduboy.print(F("Bishop"));
      break;
    case KNIGHT:
      arduboy.print(F("Knight"));
      break;
    case KING:
      arduboy.print(F("King"));
      break;
    case QUEEN:
      arduboy.print(F("Queen"));
      break;
  }
}

void draw_time(int seconds, int milliseconds) {
  char buff[20];

  sprintf(buff, "%4d:%02d", seconds, milliseconds);

  arduboy.setCursor(80, 40);
  arduboy.print("Time");
  arduboy.setCursor(80, 50);
  arduboy.print(buff);
}

void draw_time() {
  unsigned long elapsed = millis() - hippodrome.time_init;
  int seconds = min(elapsed / 1000, 9999);
  int milliseconds = (elapsed % 1000) / 10;
  draw_time(seconds, milliseconds);
}

void draw_movements() {
  char buff[20];
  sprintf(buff, "%7d", hippodrome.movements_done);

  arduboy.setCursor(80, 20);
  arduboy.print("N movs");

  arduboy.setCursor(80, 30);
  arduboy.print(buff);
}




















// EEPROM functions

void load_memory() {
  byte n = 0;
  EEPROM_read(0, n);
  if(n != HIPPODROME_MARK_MEMORY) {
    initialize_memory();
    return;
  }

  EEPROM_read(1, achievements1);
  EEPROM_read(2, achievements2);
  EEPROM_read(3, achievements3);
  EEPROM_read(4, achievements4);
  EEPROM_read(5, n_scores);
  
  for(int i = 0; i < n_scores; i++)
    EEPROM_read(6+sizeof(score)*i, score_board[i]);
}

void initialize_memory() {
  EEPROM_write(0, HIPPODROME_MARK_MEMORY);
  EEPROM_write(1, (byte)0); // Achievements 1
  EEPROM_write(2, (byte)0); // Achievements 2
  EEPROM_write(3, (byte)0); // Achievements 3
  EEPROM_write(4, (byte)0); // Achievements 4
  EEPROM_write(5, (byte)0); // N scores
  arduboy.audio.on();
  arduboy.audio.saveOnOff();
}

void save_settings() {
  EEPROM_write(1, achievements1);
  EEPROM_write(2, achievements2);
  EEPROM_write(3, achievements3);
  EEPROM_write(4, achievements4);
}

void save_score() {
  EEPROM_write(5, n_scores);
  for(int i = 0; i < n_scores; i++)
    EEPROM_write(6+sizeof(score)*i, score_board[i]);
}

template <class T> int EEPROM_write(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.update(EEPROM_STORAGE_SPACE_START + 10 + ee++, *p++);
  return i;
}

template <class T> int EEPROM_read(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(EEPROM_STORAGE_SPACE_START + 10 + ee++);
  return i;
}













// Initializers game modes

void initialize_menu() {
  initialize_pieces_menu();
  option = 0;
  mode_wait = false;
  last_milliseconds = -1;
  key_menu = 0b11111111;
  sound.tones(music_menu);
}


void initialize_mode_game() {
  mode_wait = false;
  initialize_hippodrome();
  wait_end_game = false;
  last_milliseconds_game = -1;
}


void initialize_records() {}


void initialize_end_game() {
  for(int i = 0; i < N_PIECES; i++)
    colors_end_game[i] = BLACK;
  actual_piece_end_game = -1;
  menu_n_piece = -1;
  sound.tones(music_end_game);
}


void initialize_achievements() {
  int a1 = achievements1;
  int a2 = achievements2;
  int offset = 0;
  if(!key_basic()) {
    a1 = achievements3;
    a2 = achievements4;
    offset = 12;
  }
  
  for(int i = 0; i < 6; i++) {
    achievements_completed[i] = read_bit(i, a1);
  }

  for(int i = 0; i < 6; i++) {
    achievements_completed[i+6] = read_bit(i, a2);
  }

  achievements_position = 0;
  strcpy_P(achievement_description, (char *)pgm_read_ptr(&(achievements_descriptions[achievements_position+offset])));
}


void initialize_info() {
  info_piece = NONE;
  last_milliseconds = millis();
}










// Modes

void mode_end_game() {
  char buff[30];
  
  sprintf(buff, "%d movs in %d:%02ds", last_score.movs, last_score.seconds, last_score.milliseconds);
  arduboy.setCursor((21 - strlen(buff)+1)/2 * 6.09 + 3, 6);
  arduboy.print(buff);

  arduboy.setCursor(35, 20);
  if(is_new_record)
    arduboy.print("New record");
  else
    arduboy.print("Good game!");

  for(int i = 0; i < N_PIECES; i++)
    draw_piece_type(all_pieces[i], 18 + i * 16, 35, colors_end_game[i]);

  if(arduboy.everyXFrames(3)) {
      actual_piece_end_game = (actual_piece_end_game + 1) % N_PIECES;
      colors_end_game[actual_piece_end_game] = !colors_end_game[actual_piece_end_game];
  }

  if(!is_pressed_a && arduboy.pressed(A_BUTTON) || !is_pressed_b && arduboy.pressed(B_BUTTON)) {
    sound.tone(0, 0);
    if(is_new_record) {
      from_game = true;
      change_mode_records();
    }
    else {
      change_mode_game();
    }
  }
}

void mode_info() {
  arduboy.setTextWrap(true);
  arduboy.setCursor(3, 10);
  arduboy.print("      Hippodrome     ");
  arduboy.setCursor(0, 20);
  arduboy.print("      GPL license    ");
  arduboy.setCursor(1, 35);
  arduboy.print("Programmed by        ");
  arduboy.setCursor(1, 45);
  arduboy.print("Miguel Riaza Valverde");
  arduboy.setCursor(1, 55);
  arduboy.print("                 2019");
  
  draw_piece_type(info_piece, 10, 12, info_color);
  
  if(info_piece != NONE && millis() - last_milliseconds >= 300 || info_piece == NONE && millis() - last_milliseconds >= 1000) {
    int n = random(0, N_PIECES);
    info_piece = all_pieces[n];
    info_color = random(0, 2);
    sound.tone(blues_notes[n], info_color ? random(200, 400) : 150);
    last_milliseconds = millis();
  }
  
  if(pressed_a() || pressed_b())
    change_mode_menu();
}

void mode_records() {
  char buff[30];
  for(int i = 0; i < n_scores && i < 5; i++) {
    arduboy.setCursor(4, (i % 5) * 10 + 10);
    sprintf(buff, "%2d. %4d:%02d %3dmovs", i+1, score_board[i].seconds, score_board[i].milliseconds, score_board[i].movs);
    arduboy.print(buff);
  }

  if(n_scores == 0) {
    arduboy.setCursor(12, 20);
    arduboy.print("Empty score board");
  }

  if(pressed_a() || pressed_b())
    if(from_game) {
      from_game = false;
      change_mode_game();
    }
    else
      change_mode_menu();
}

void mode_game() {

  if(wait_end_game) {
    int elapsed = millis() - last_milliseconds_game;
    arduboy.fillRect(66, 0, 8, 64.0 / MILLIS_END_GAME * elapsed, WHITE);
    if(arduboy.buttonsState()) {
      unsigned long aux = millis();
      arduboy.waitNoButtons();
      last_milliseconds_game += millis() - aux;
    }
    draw_time(last_score.seconds, last_score.milliseconds);
    if(elapsed >= MILLIS_END_GAME) {
      change_mode_end_game();
      clear_screen = true;
    }
    return;
  }
  
  draw_game();

  if(pressed_up() || pressed_left()) {
    hippodrome.position_movements -=  1;
    if(hippodrome.position_movements < 0)
      hippodrome.position_movements = hippodrome.n_movements - 1;
    if(hippodrome.n_movements == 1)
      sound.tone(220, 150);
    else
      sound.tone(440, 100);
  }

  if(pressed_down() || pressed_right()) {
    hippodrome.position_movements = (hippodrome.position_movements + 1) % hippodrome.n_movements;
    if(hippodrome.n_movements == 1)
      sound.tone(220, 150);
    else
      sound.tone(440, 100);
  }


  if(pressed_a()) {
    move_select_piece();
    sound.tone(660, 100);
  }

  if(pressed_b()) {
    mode_wait = true;
    last_milliseconds = millis();
    initialize_hippodrome();
    sound.tone(110, 200);
  }
  else if(mode_wait && !arduboy.pressed(B_BUTTON))
    mode_wait = false;

  if(mode_wait && (millis() - last_milliseconds) >= MILLIS_BACK_MENU / 2)
    arduboy.fillRect(66, 0, 8, 64.0 / (MILLIS_BACK_MENU / 2) * (millis() - last_milliseconds - (MILLIS_BACK_MENU / 2)), WHITE);

  if(mode_wait && millis() - last_milliseconds >= MILLIS_BACK_MENU)
    change_mode_menu();

  if(check_end()) {
    unsigned long elapsed = millis() - hippodrome.time_init;
    int seconds = elapsed / 1000;
    int milliseconds = (elapsed % 1000) / 10;
    last_score.seconds = seconds, 9999;
    last_score.milliseconds = milliseconds;
    last_score.movs = hippodrome.movements_done;
    is_new_record = add_score(seconds, milliseconds, hippodrome.movements_done);
    if(check_achievements()) {
      save_settings();
      sound.tones(music_new_achievement);
    }
    else
      sound.tones(music_end);
    last_milliseconds_game = millis();
    wait_end_game = true;
    clear_screen = false;
    draw_end();
    draw_time(seconds, milliseconds);
  }

}

void mode_menu() {
  draw_title(24, 8, WHITE);
  
  int offset = 10;
  int actual = 0;
  int last_piece = menu_n_piece;

  draw_note(114, 12, active_sound);

  if(!mode_wait) {
    for(int i = 2; i < 4; i++) {
      for(int j = 0; j < 4; j++) {
        if(arduboy.everyXFrames(6) && random(0, 100) < 50) {
          int n = random(0, N_PIECES);
          PIECE piece = all_pieces[n];
          pieces_menu[actual] = piece;
          colors_menu[actual] = random(0, 2);
        }
        if(arduboy.everyXFrames(3))
          colors_menu[actual] = random(0, 2);
  
        if(visible_pieces[actual])
          draw_piece_type(pieces_menu[actual], offset+j*16, i*16 - 5, colors_menu[actual]);
        actual++;
      }
    }

    if(arduboy.everyXFrames(3)) {
      bool onp = true;
      bool offp = true;
      for(int i = 0; i < 8; i++)
        if(!visible_pieces[i])
          onp = false;
        else
          offp = false;
          
      if(onp) set_true = false;
      if(offp) set_true = true;
  
      int n = random(0, 8);
      for(int i = 0; i < 8; i++) {
        int p = (i + n) % 8;
        if(!set_true == visible_pieces[p]) {
          visible_pieces[p] = !visible_pieces[p];
          break;
        }
      }
    }
  }

  else {
      for(int i = 2; i < 4; i++) {
      for(int j = 0; j < 4; j++) {
        if(arduboy.everyXFrames(3))
          colors_menu[actual] = random(0, 2);
        draw_piece_type(all_pieces[menu_n_piece], offset+j*16, i*16 - 5, colors_menu[actual]);
        actual++;
      }
    }
  }

  if(mode_wait && millis() - last_milliseconds >= 1000) {
    int aux = option;
    initialize_menu();
    option = aux;
  }

  

  for(int i = 0; i < 3; i++) {
    arduboy.setCursor(90, 30 + i*10);
    arduboy.print(options[i]);
  }

  arduboy.setCursor(80, 30 + option*10);
  arduboy.print(">");


  if(pressed_down()) {
    mode_wait = true;
    last_milliseconds = millis();
    while(menu_n_piece == last_piece) menu_n_piece = random(0, N_PIECES);
    option = (option + 1) % 3;
    sound.tone(440, 100);
    if(add_key_menu(DOWN))
      change_mode_achievements();
  }

  if(pressed_up()) {
    mode_wait = true;
    last_milliseconds = millis();
    while(menu_n_piece == last_piece) menu_n_piece = random(0, N_PIECES);
    option = (option + 2) % 3;
    sound.tone(440, 100);
  }

  if(pressed_right()) {
    arduboy.audio.on();
    arduboy.audio.saveOnOff();
    active_sound = true;
  }

  if(pressed_left()) {
    arduboy.audio.off();
    arduboy.audio.saveOnOff();
    active_sound = false;
  }

  if(pressed_a()) {
    sound.tone(660, 300);
    if(option == 0) change_mode_game();
    if(option == 1) change_mode_records();
    if(option == 2) change_mode_info();
  }

  if(pressed_b()) {
    mode_wait = true;
    last_milliseconds = millis();
    while(menu_n_piece == last_piece) menu_n_piece = random(0, N_PIECES);
    sound.tone(220, 100);
    if(add_key_menu(B))
      change_mode_achievements();
  }
}

void mode_achievements() {
  int actual = 0;
  bool some_pressed = false;
  char buff[22];
  int len;
  char* aux;
  int offset = 0;
  if(!key_basic())
    offset = 12;
  
  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 6; j++) {
      if(achievements_completed[actual])
        draw_field(16+j*16, i*16, WHITE);
      if(achievements_position == actual)
        draw_select(16+j*16, i*16, negate_color(achievements_completed[actual]));
      draw_piece_type(all_pieces[j], 16+j*16, i*16, i);
      actual++;
    }

    for(int k = 0; k < 8; k++)
      draw_field(k*16, i*16+32, WHITE);
  }

  if(pressed_down() && (some_pressed = true))
    if(achievements_position < 6)
      achievements_position = (achievements_position + 6) % 12;

  if(pressed_up() && (some_pressed = true))
    if(achievements_position >= 6)
      achievements_position = (achievements_position + 12 - 6) % 12;

  if(pressed_right() && (some_pressed = true))
    if(achievements_position != 11)
      achievements_position++;

  if(pressed_left() && (some_pressed = true))
    if(achievements_position != 0)
      achievements_position--;

  if(some_pressed && last_achievement_read != achievements_position)
    sound.tone(440, 100);
  else if(some_pressed)
    sound.tone(220, 200);

  if(last_achievement_read != achievements_position) {
    last_achievement_read = achievements_position;
    strcpy_P(achievement_description, (char *)pgm_read_ptr(&(achievements_descriptions[achievements_position+offset])));
  }

  arduboy.setTextColor(BLACK);
  arduboy.setTextBackground(WHITE);
  len = strlen(achievement_description);
  for(int i = 0; i < len; i += 21) {
    int m = min(21, len - i);
    aux = &(achievement_description[i]);
    aux = strncpy(buff, aux, m);
    buff[m] = 0;
    arduboy.setCursor(1, 33 + 8 * (i/21));
    arduboy.print(buff);
  }
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);

  if(pressed_a() || pressed_b())
    change_mode_menu();
}





// Change modes

void change_mode_info() {
  mode = INFO;
  initialize_info();
}

void change_mode_records() {
  mode = RECORDS;
  initialize_records();
}

void change_mode_end_game() {
  mode = END_GAME;
  initialize_end_game();
}

void change_mode_menu() {
  mode = MENU;
  initialize_menu();
}

void change_mode_game() {
  mode = GAME;
  initialize_mode_game();
}

void change_mode_achievements() {
  mode = ACHIEVEMENTS;
  initialize_achievements();
}







// Setup and loop functions


bool add_key_menu(byte key);
bool key_basic();
int read_bit(int n, unsigned char x);
unsigned char write_bit_1(int n, unsigned char x);
int negate_color(int color);
int get_color(int x, int y, int color_init);
void sort_movements();
bool check(int x, int y, int n, PIECE* pieces);
void push_movement(int n);
void check_movements();
void move_select_piece();
bool check_end();
void initialize_board();
void initialize_hippodrome();
bool add_score(int seconds, int milliseconds, int movs);
void sort_scores();
void initialize_pieces_menu();
bool pressed_a();
bool pressed_b();
bool pressed_up();
bool pressed_down();
bool pressed_left();
bool pressed_right();
bool check_achievements();
void setup() {
  arduboy.begin();
  arduboy.safeMode();
  arduboy.setFrameRate(24);
  arduboy.initRandomSeed();
  arduboy.setTextWrap(true);
  load_memory();
  active_sound = arduboy.audio.enabled();
  change_mode_menu();
}

void loop() {
  if (!(arduboy.nextFrame()))
    return;

  if(clear_screen)
    arduboy.clear();
  

  switch(mode) {
    case MENU:
      mode_menu();
      break;
    case GAME:
      mode_game();
      break;
    case RECORDS:
      mode_records();
      break;
    case INFO:
      mode_info();
      break;
    case END_GAME:
      mode_end_game();
      break;
    case ACHIEVEMENTS:
      mode_achievements();
      break;
  }

  if(!is_pressed_a && arduboy.pressed(A_BUTTON))
    is_pressed_a = true;
  if(!is_pressed_b && arduboy.pressed(B_BUTTON))
    is_pressed_b = true;
  if(is_pressed_a && !arduboy.pressed(A_BUTTON))
    is_pressed_a = false;
  if(is_pressed_b && !arduboy.pressed(B_BUTTON))
    is_pressed_b = false;
  if(is_pressed_left && !arduboy.pressed(LEFT_BUTTON))
    is_pressed_left = false;
  if(is_pressed_right && !arduboy.pressed(RIGHT_BUTTON))
    is_pressed_right = false;
  if(is_pressed_up && !arduboy.pressed(UP_BUTTON))
    is_pressed_up = false;
  if(is_pressed_down && !arduboy.pressed(DOWN_BUTTON))
    is_pressed_down = false;


  arduboy.display();
}
