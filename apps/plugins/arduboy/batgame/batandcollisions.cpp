//bat move and shoot demo
#include <Arduboy2.h>

#include "images.h"
#include "Collide.h"

Arduboy2 arduboy;

//define constants
constexpr uint8_t maxSonics = 5; //max number of active player bullets

// Max number of active background segments
constexpr uint8_t maxBackgrounds = 10;

// Start point of new background segment
constexpr uint8_t backgroundStart = 128;

// Speed of bullets
constexpr int backgroundSpeed = 1;

// Limit of the x position of backgrounds
constexpr int backgroundXLimit = -31;

constexpr int enemiesCount = 12;

constexpr int delayModifier = 4;

constexpr int restartDelay = 300;

constexpr uint8_t maxGhostSpeed = 5;


// Declare global variables here
// Player x position (L<R)
int playerx = 5;
// Player y position (U<D), is float due to fractional rate of descent
float playery = 10;
// Player x limits
int pxupperlim = 120;
int pxlowerlim = 0;
// Player y limits
int pyupperlim = 57;
int pylowerlim = 0;
// Player x speed
int pvx = 2;
// Player flap impetus
int vflap = -2;
// Grav force
float grav = .2;
// Rate at whic bat glides... use .5; for 60 FPS
float vglide = .8;
// Player vertical speed...this is giving issues at 60 FPS
float pvy = 1;
// Player dive speed
int vdive = 4;

// 0 glide, 1 flap, 2 dive... used to animate
int batsprite = 0;
int currentnumsonic = 0;
// Speed of bullets
int sonicspeed = 3;
// Limit as to x position of sonics
int sonicxlim = 130;
int currentdelay = 60;
int currentenemyindex = 0;
int exlowerlim = -32;

int gamestate = 0; //0 title, 1 play game, 2 death

// Declare whatever else
struct Sonic
{
  int x;
  int y;
  bool enabled;
};

struct Background
{
  int x;
  int y;
  int spriteIndex;
};

// First "Sonic" refers to structure, second "sonic" is name of array
Sonic sonic[maxSonics]
{
  // Refer to Sonic structure. setting initial values in array of sonics. x=0, y=0, enabled = false (not active)
  { 0, 0, false},
  { 0, 0, false},
  { 0, 0, false},
  { 0, 0, false},
  { 0, 0, false},
};

enum class EnemyType : uint8_t
{
  Crow,
  Skull,
  Illegible,
  Wisp,
  Ghost,
  FlyingSnake,
  Eyeball,
  Owl,
};

//add enemy sprite frame to this Struct and array.
struct Enemy
{
  EnemyType enemytype;
  int x;
  int y;
  int delaynext; //should count DOWN one unit per frame
  bool enabled;
  int modA; //changes function based on etype
  int modB;
};

Enemy enemyList[enemiesCount] 
{
  { EnemyType::Crow, 127, 32, 300, false , 0, 32 },
  { EnemyType::Crow, 127, 15, 150, false, 0, 15 },
  { EnemyType::Crow, 127, 48, 122, false, 0, 48 },
  { EnemyType::Wisp, 127, 32, 80, false , 1, 0 },
  { EnemyType::Crow, 127, 15, 3, false, 0, 15 },
  { EnemyType::Wisp, 127, 48, 60, false, 1, 0 },
  { EnemyType::Crow, 127, 32, 10, false , 0, 32 },
  { EnemyType::Wisp, 127, 15, 0, false, 1, 0 },
  { EnemyType::Wisp, 127, 48, 122, false, 1, 0 },
};

// First "Background" refers to structure, second "background" is name of array
Background background[maxBackgrounds]
{
  // Refer to Background structure. setting initial values in array of backgrounds. x=0, y=0, srpite index, flip = false (true for ceiling only)
  // Total of 10 for maxBackgrounds (5 floor, 5 ceiling)
  { 0, 55, 0},
  { 32, 55, 0},
  { 64, 55, 0},
  { 96, 55, 0},
  { 128, 55, 0},
  { 0, 0, 0},
  { 32, 0, 0},
  { 64, 0, 0},
  { 96, 0, 0},
  { 128, 0, 0},
};

// Runs once on startup
void bat();
void drawbat();
void fireSonics(int sonicnumber);
void batfire();
void updateSonics();
void updateBackgrounds();
void drawBackgrounds();
void enemyspawn();
void updateenemies();
bool collisionSonic (int enemyX, int enemyY, const uint8_t *enemySprite);
bool collisionWithEnemy (int enemyX, int enemyY, const uint8_t *enemySprite);
void enemyReset(int i);
void destroyEnemy(int i);
void drawcrow(int i, int crowsprite);
void crow(int i);
void increaseDifficulty();
void gameOver();
void titleScreen();
void playGame();
void drawWisp(int i, int wispSprite);
void wisp(int i);

void setup()
{
  arduboy.begin();
  arduboy.clear();
  arduboy.setFrameRate(30); //TBD if 60 frames or 30
}

// Runs continuously
void loop()
{
  // Prevent the Arduboy from running too fast
  if (!arduboy.nextFrame()) {
    return;
  }

  arduboy.clear();

  // Needed for justPressed etc.
  arduboy.pollButtons();

  switch(gamestate)
  {
    case 0:
      titleScreen();
      break;

    case 1:
      playGame();
      break;

    case 2:
      gameOver();
      break;
  }

  arduboy.display();
}

void bat()
{
  //set batspr to glide, here to prevent sticky sprites
  batsprite = 0;

  // Move left
  if (arduboy.pressed(LEFT_BUTTON))
  {
    playerx = playerx - pvx;
    if (playerx < pxlowerlim)
    {
        //prevent movement offscreen
        playerx = pxlowerlim;
    }
  }

  // Move right
  if (arduboy.pressed(RIGHT_BUTTON))
  {
    playerx = playerx + pvx;
    if (playerx > pxupperlim)
    {
        playerx = pxupperlim;
    }
  }

  // If vertical speed is LESS THAN glide speed
  if (pvy < vglide)
  {
    // Increase vertical speed by grav force (positive is down)
    pvy = pvy + grav;
  }

  // If vertical speed GREATER THAN glide speed
  if (pvy > vglide)
  {
    // Slow to glide sped instantly
    pvy = vglide;
  }

  // Flapping, note JUSTPRESSED
  if (arduboy.justPressed(A_BUTTON))
  {
    // Make vert speed equal flap speed
    pvy = vflap;
    // Change sprite
    batsprite = 1;
  }

  //dive
  if (arduboy.pressed(DOWN_BUTTON))
  {
    //note vdive is greater than vglide
    pvy = vdive;
    batsprite = 3;
  }

  // Must be AFTER dive check, or dive would be negated by glide checks
  playery = playery + pvy;
  if (playery > pyupperlim)
  {
    // Prevent moving offscreen
    playery = pyupperlim;
  }
  else if (playery < pylowerlim)
  {
    // Prevent moving offscreen
     playery = pylowerlim;
  }
}

void drawbat()
{
  //Draw player sprite
  //For whatever reason, could not get a switch statement to work!!!
  if (batsprite == 0)
  {
    Sprites::drawSelfMasked(playerx, playery, batglide, 0);
  }
  else if (batsprite == 1)
  {
    Sprites::drawSelfMasked(playerx, playery, batflap, 0);
  }
  else if (batsprite == 3)
  {
    // This sprite is differently shaped, hence position modifiers
    Sprites::drawSelfMasked(playerx + 4, playery - 9, batdive, 0);
  }
}

void fireSonics(int sonicnumber)
{
  sonic[sonicnumber].y = playery + 3;
  sonic[sonicnumber].x = playerx + 8;
  sonic[sonicnumber].enabled = true;
}

void batfire()
{
  // Shooting, note JUSTPRESSED
  if (arduboy.justPressed(B_BUTTON))
  {
    // If less bulets active than max
    if (currentnumsonic < maxSonics)
    {
      // This loop finds first inactive slot in sonic array
      for (uint8_t i = 0; i < maxSonics; i++)
      {
        // If disabled,
        if (!sonic[i].enabled)
        {
          // Fire new bullet (item i in array)
          fireSonics(i);
          break;
        }
        // Add to count of bullets
        currentnumsonic++;
      }
    }
  }
}

void updateSonics()
{
  for (byte i = 0; i < maxSonics; i++)
  {
    if (sonic[i].enabled == true)
    {
      Sprites::drawSelfMasked(sonic[i].x, sonic[i].y, sonicbullet, 0);
      // Move sonic to right
      sonic[i].x = sonic[i].x + sonicspeed;
    }

    if (sonic[i].x > sonicxlim)
    {
      sonic[i].enabled = false;
      currentnumsonic--;
    }
  }
}

// Corrected by Pharap. Thank you!!!
void updateBackgrounds()
{
  for (uint8_t index = 0; index < maxBackgrounds; ++index)
  {
      // Move background left
      background[index].x -= backgroundSpeed;

      if (background[index].x < backgroundXLimit)
      {
        int imageIndex = random(0, 3);
        background[index].x = backgroundStart;
        background[index].spriteIndex = imageIndex;
      }
  }
}

// Could maybe implement sprite rotation for savings?
void drawBackgrounds()
{
  for (uint8_t index = 0; index < maxBackgrounds; ++index)
  {
    if (index < (maxBackgrounds / 2))
    {
      if (background[index].spriteIndex == 0)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, flat, 0);
      }
      else if (background[index].spriteIndex == 1)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, spikeleft, 0);
      }
      else if (background[index].spriteIndex == 2)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, spikeright, 0);
      }
    }
    else
    {
      if (background[index].spriteIndex == 0)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, flatup, 0);
      }
      else if (background[index].spriteIndex == 1)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, spikeleftup, 0);
      }
      else if (background[index].spriteIndex == 2)
      {
        // Change flat to sprite index
        Sprites::drawOverwrite(background[index].x, background[index].y, spikerightup, 0);
      }
    }
  }
}

void enemyspawn()
{
  if (currentdelay > 0)
  {
    currentdelay--;
  }
  // Must be == 0. -1 will halt list progression until boss defeated
  else if (currentdelay == 0)
  {
    enemyList[currentenemyindex].enabled = true;
    currentdelay = enemyList[currentenemyindex].delaynext;
    currentenemyindex++;
  }
}

void updateenemies()
{
  for (uint8_t i =0; i <enemiesCount; i++)
  {
    if (enemyList[i].enabled == true)
    {
      if (enemyList[i].enemytype == EnemyType::Crow)
      {
        crow(i);
      }
      if (enemyList[i].enemytype == EnemyType::Wisp)
      {
        wisp(i);
      }
    }
  }
}

// Adapted from filmote
bool collisionSonic (int enemyX, int enemyY, const uint8_t *enemySprite)
{
  for (uint8_t i = 0; i < maxSonics; i++)
  {
    if (sonic[i].enabled == true)
    {
      if (collide(enemyX, enemyY , enemySprite, sonic[i].x, sonic[i].y , sonicbullet))
      {
        sonic[i].enabled = false;
        return true;
      }
    }
  }

  return false;
}

// Adapted from filmote
bool collisionWithEnemy (int enemyX, int enemyY, const uint8_t *enemySprite)
{
  if (collide(playerx + 6, playery + 3, batCollisionMask, enemyX, enemyY , enemySprite))
  {
    return true;
  }
  return false;
}

void enemyReset(int i)
{
  enemyList[i].x = 130;
  enemyList[i].y = 8*random(1,8);
  if (enemyList[i].enemytype == EnemyType::Crow)
      {
        enemyList[i].modB = enemyList[i].y;
      }
      if (enemyList[i].enemytype == EnemyType::Wisp)
      {
        enemyList[i].modA = 1;
      }
  
  //use of delayModifier needs to change so that delays can go back to original values upon death. probably place this in the spawn enemy/delay function.
  enemyList[i].delaynext = enemyList[i].delaynext; //  / delayModifier; 
  enemyList[i].enabled = false;
}

// Can be updated to include a death anim?
void destroyEnemy(int i)
{
  enemyReset(i);
}

void drawcrow(int i, int crowsprite)
{
  //Draw player sprite
  if (crowsprite == 0)
  {
    Sprites::drawSelfMasked(enemyList[i].x, enemyList[i].y, crowglide, 0);
  }
  else if (crowsprite == 1)
  {
    Sprites::drawSelfMasked(enemyList[i].x, enemyList[i].y, crowflap, 0);
  }

}

void crow(int i)
{
  int crowsprite = 1;
  int crowvx = 1;
  int crowvglide = 1;
  int grav = 1;
  int crowvflap = -4;

  // Set crowspr to glide, here to prevent sticky sprites ---could be local sprite
  if (enemyList[i].modA > 0)
  {
    crowsprite = 0;
  }

  // Move left
  enemyList[i].x = enemyList[i].x - crowvx;

  // If vert speed is LESS THAN glide speed---crow vglide could be local
  if (enemyList[i].modA < crowvglide)
  {
    // Increase vert speed by grav force (positive is down)---grav could be local
    enemyList[i].modA = enemyList[i].modA + grav;
  }

  // Flapping, note JUSTPRESSED
  if (enemyList[i].y > enemyList[i].modB)
  {
    // Make vertical speed equal flap speed -- crowvflap local?
    enemyList[i].modA = crowvflap;
    // Change sprite
    crowsprite = 1;
  }

  // Must be AFTER dive check, or dive would be negated by glide checks
  enemyList[i].y = enemyList[i].y + enemyList[i].modA;
  if (crowsprite == 1)
  {
    if (collisionSonic(enemyList[i].x, enemyList[i].y, crowflap)) //replacing with sprite frame in array would eliminate the if-else
    {
      destroyEnemy(i);
    }
    if (collisionWithEnemy(enemyList[i].x, enemyList[i].y, crowflap))
    {
      gamestate = 2;
    }
  }
  else
  {
    if(collisionSonic(enemyList[i].x, enemyList[i].y, crowglide))
    {
      destroyEnemy(i);
    }
    if (collisionWithEnemy(enemyList[i].x, enemyList[i].y, crowglide))
    {
      gamestate = 2;
    }
  }
  drawcrow(i, crowsprite);

  // Reset enemy when it moves offscreen
  if (enemyList[i].x < exlowerlim)
  {
    enemyReset(i);
  }
}

void increaseDifficulty()
{
  if (currentenemyindex == enemiesCount)
  {
    currentdelay = restartDelay;
    currentenemyindex = 0;
  }
}

void gameOver()
{
  //add display of death screen
  if (arduboy.justPressed(B_BUTTON))
  {
    gamestate = 0;
    for (uint8_t i =0; i <enemiesCount; i++)
    {
      enemyReset(i);
    }
    playerx = 5;
    playery = 10;
    for (uint8_t i =0; i <maxSonics; i++)
    {
      sonic[i].enabled = false;
    }

    currentdelay = 60;
    currentenemyindex = 0;
    
  }
}

void titleScreen()
{
  if (arduboy.justPressed(B_BUTTON))
  {
    gamestate = 1;
  }
  //Add display of titlescreen
}

void playGame()
{
  updateBackgrounds();
  drawBackgrounds();
  enemyspawn();
  updateenemies();
  batfire();
  updateSonics();
  bat();
  drawbat();
  increaseDifficulty();
}



void drawWisp(int i, int wispSprite){
  //Draw Wisp sprite
  if (wispSprite == 0){ 
      Sprites::drawSelfMasked(enemyList[i].x, enemyList[i].y, ghosta, 0);
  }
  else if (wispSprite == 1){
      Sprites::drawSelfMasked(enemyList[i].x, enemyList[i].y, ghostb, 0);
  }
  
}

void wisp(int i){ //modA = x velocity, modB = animation timer
  int wispSprite = 1;
  int acceleration = 1;
  if (enemyList[i].x < 100)
  {
    if (enemyList[i].modA < maxGhostSpeed)
    {
      enemyList[i].modA++; //set crowspr to glide, here to prevent sticky sprites ---could be local sprite
    }
  }
  enemyList[i].x = enemyList[i].x-enemyList[i].modA; //move left
  
  if (enemyList[i].modB < 2)
  {
    wispSprite = 0;
    if (enemyList[i].modB < 0)
    {
      enemyList[i].modB = 4;
    }
  }
  enemyList[i].modB--;
  drawWisp(i, wispSprite);

  if (enemyList[i].x < exlowerlim)
  { //reset enemy when it moves offscreen
    enemyReset(i);
  }

  if (wispSprite == 1)
  {
    if (collisionWithEnemy(enemyList[i].x, enemyList[i].y, ghostb))
    {
      gamestate = 2;
      enemyReset(i);
    }
  }
  else
  {
    if (collisionWithEnemy(enemyList[i].x, enemyList[i].y, ghosta))
    {
      gamestate = 2;
      enemyReset(i);
    }
  }
}
