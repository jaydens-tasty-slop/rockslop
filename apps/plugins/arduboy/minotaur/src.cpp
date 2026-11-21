#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "sprites.h"

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

// Game mode enumeration for state machine
enum class GameMode
{
  Menu,
  Help,
  Splash,
  Game,
  Win,
  Lose
};

enum class MenuOption
{
  Play,
  Help,
  Credits
};

// Player state tracking
struct PlayerState
{
  bool isHuman;
  bool isArmed;
};

enum class FacingDirection
{
  Left,
  Right,
  Up,
  Down
};

struct LevelConfig
{
  int left;
  int right;
  int top;
  int bottom;
  int spawnX;
  int spawnY;
};

enum class ItemType
{
  Sword,
  String,
  Key
};

struct WorldItem
{
  ItemType type;
  uint8_t screen; // which of the SCREEN_COUNT screens this item lives on
  int x;
  int y;
  bool collected;
};

struct InventoryState
{
  bool hasSword;
  bool hasString;
  bool hasKey;
};

struct GameState
{
  GameMode mode;
  PlayerState player;
  InventoryState inventory;
  int playerX;
  int playerY;
  int menuCursorY;
  int splashIndex;
  FacingDirection facing;
  uint8_t currentScreen;
  bool winJinglePlayed;
  bool loseJinglePlayed;
  bool minotaurAlive;
};

// Runtime state lives in one container for easier maintenance.
GameState game = {
    GameMode::Menu,
    {true, false},
    {false, false, false},
    16,
    16,
    28,
    6,
    FacingDirection::Down,
    0,    // currentScreen
    false,
    false,
    true // minotaurAlive
};

  const uint16_t winJingle[] PROGMEM = {
    NOTE_C5, 90,
    NOTE_E5, 90,
    NOTE_G5, 120,
    NOTE_C6, 240,
    TONES_END};

  // Short cheerful pickup sound
  const uint16_t pickupSound[] PROGMEM = {
    NOTE_D5, 100,
    NOTE_G5, 100,
    TONES_END};

  // Epic kill sound: three rising notes
  const uint16_t killSound[] PROGMEM = {
    NOTE_E5, 80,
    NOTE_G5, 80,
    NOTE_C6, 200,
    TONES_END};

  // Dark lose sound: descending notes
  const uint16_t loseSound[] PROGMEM = {
    NOTE_A4, 150,
    NOTE_G4, 150,
    NOTE_E4, 200,
    TONES_END};

// Single playfield config (one bounded area, shared by all screens)
const LevelConfig level = {16, 112, 16, 48, 32, 24};

constexpr uint8_t ITEM_COUNT = 3;
const WorldItem initialItems[ITEM_COUNT] = {
  {ItemType::Sword, 1, 32, 24, false},
  {ItemType::String, 2, 64, 24, false},
  {ItemType::Key, 3, 80, 24, false}};
WorldItem levelItems[ITEM_COUNT];

// Collision boundaries (playable area)
constexpr int PLAYER_WIDTH = 16;
constexpr int PLAYER_HEIGHT = 16;
constexpr int WALL_THICKNESS = 16;
// Screen grid: 2×2 rooms navigable by walking to the edge
constexpr uint8_t SCREEN_COLS = 2;
constexpr uint8_t SCREEN_ROWS = 2;
constexpr uint8_t SCREEN_COUNT = SCREEN_COLS * SCREEN_ROWS;

// Menu layout constants
constexpr int MENU_CURSOR_X = 48;
constexpr int MENU_CURSOR_TOP = 28;
constexpr int MENU_CURSOR_BOTTOM = 44;
constexpr int MENU_CURSOR_STEP = 8;

// Bottom HUD constants
constexpr int HUD_SWORD_X = 16;
constexpr int HUD_STRING_X = 32;
constexpr int HUD_KEY_X = 48;
constexpr int HUD_ITEMS_Y = 56;

// Mini-map constants (2x2 rooms)
constexpr int MINIMAP_X = 2;
constexpr int MINIMAP_Y = 49;
constexpr int MINIMAP_SIZE = 16;
constexpr int MINIMAP_CELL_SIZE = 8;
constexpr int MINIMAP_MARKER_SIZE = 2;

// Splash screen configuration
constexpr uint8_t SPLASH_COUNT = 7;
const uint8_t *const splashScreens[SPLASH_COUNT] = {
    mx_0,
    mx_1,
    mx_2,
    mx_3,
    mx_4,
    mx_5,
    mx_6};

bool hasAdjacentScreen(FacingDirection dir);

struct MovementBounds
{
  int left;
  int right;
  int top;
  int bottom;
};

constexpr int TILE_SIZE = 16;
constexpr int TILE_COLS = WIDTH / TILE_SIZE;
constexpr int TILE_ROWS = HEIGHT / TILE_SIZE;
constexpr uint8_t MINOTAUR_SCREEN = 1;
constexpr int MINOTAUR_X = 64;
constexpr int MINOTAUR_Y = 24;
constexpr uint8_t EXIT_DOOR_SCREEN = 3;
constexpr int EXIT_DOOR_TILE_X = TILE_COLS - 1;
constexpr int EXIT_DOOR_TILE_Y = 2;

int exitDoorPixelX();
int exitDoorPixelY();
bool isExitDoorTile(int tileX, int tileY);
bool isExitDoorOpen();
bool hasPlayerEscaped();
bool overlapsExitDoorTile(int x, int y);
bool isSolidWallSprite(const uint8_t *sprite);
bool collidesWithSolidWallSprite(int x, int y);
bool canMoveTo(int x, int y);
bool rectanglesOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
void setInventoryItem(ItemType type, bool value);
void resetItems();
void collectItemsAtPlayerPosition();
void drawLevelItems();
void restrictPlayerPosition();
void startGame();
void drawMinotaurNpc();
void handleMinotaurCollision();
bool tryReturnToMenu();
bool hasAdjacentScreen(FacingDirection dir);
void drawWallVariant(int x, int y, const uint8_t *sprite, uint8_t rotation);
void drawExitDoor();
void drawOuterLabyrinthWalls();
void checkScreenTransition(FacingDirection dir);
void drawGameplayBackground();
void handlePlayerStateInput();
void drawGameplayHud();
void handlePlayerMovement();
void handleHelpScreen();
void handleSplashScreen();
void handleGameplay();
void handleWinScreen();
void handleLoseScreen();
void handleMenu();
void drawPlayerSprite();

int exitDoorPixelX()
{
  return EXIT_DOOR_TILE_X * TILE_SIZE;
}

int exitDoorPixelY()
{
  return EXIT_DOOR_TILE_Y * TILE_SIZE;
}

bool isExitDoorTile(int tileX, int tileY)
{
  return game.currentScreen == EXIT_DOOR_SCREEN &&
         tileX == EXIT_DOOR_TILE_X &&
         tileY == EXIT_DOOR_TILE_Y;
}

bool isExitDoorOpen()
{
  if (!game.inventory.hasKey || game.currentScreen != EXIT_DOOR_SCREEN)
  {
    return false;
  }

  // Open the door when the player is touching or overlapping it.
  return rectanglesOverlap(game.playerX, game.playerY, PLAYER_WIDTH, PLAYER_HEIGHT,
                           exitDoorPixelX() - 1, exitDoorPixelY() - 1,
                           PLAYER_WIDTH + 2, PLAYER_HEIGHT + 2);
}

bool hasPlayerEscaped()
{
  return isExitDoorOpen() &&
         game.currentScreen == EXIT_DOOR_SCREEN &&
         game.playerX == exitDoorPixelX() &&
         game.playerY == exitDoorPixelY();
}

bool overlapsExitDoorTile(int x, int y)
{
  if (game.currentScreen != EXIT_DOOR_SCREEN)
  {
    return false;
  }

  return rectanglesOverlap(x, y, PLAYER_WIDTH, PLAYER_HEIGHT,
                           exitDoorPixelX(), exitDoorPixelY(), TILE_SIZE, TILE_SIZE);
}

MovementBounds getMovementBounds()
{
  return {
      hasAdjacentScreen(FacingDirection::Left) ? 0 : WALL_THICKNESS,
      hasAdjacentScreen(FacingDirection::Right) ? WIDTH : WIDTH - WALL_THICKNESS,
      hasAdjacentScreen(FacingDirection::Up) ? 0 : WALL_THICKNESS,
      hasAdjacentScreen(FacingDirection::Down) ? HEIGHT : HEIGHT - WALL_THICKNESS};
}

const uint8_t *resolveWallVariant(const uint8_t *sprite, uint8_t rotation)
{
  uint8_t rot = rotation & 0x03;

  if (sprite == straight)
  {
    return (rot % 2 == 0) ? straight : straight_h;
  }

  if (sprite == angle)
  {
    if (rot == 1)
      return angle_r1;
    if (rot == 2)
      return angle_r2;
    if (rot == 3)
      return angle_r3;
  }

  return sprite;
}

bool isSolidWallSprite(const uint8_t *sprite)
{
  return sprite == end ||
         sprite == angle ||
         sprite == dot ||
         sprite == wallt ||
         sprite == closed ||
         sprite == straight ||
         sprite == straight_h ||
         sprite == angle_r1 ||
         sprite == angle_r2 ||
         sprite == angle_r3 ||
         sprite == cross;
}

const uint8_t *outerWallSpriteAtTile(int tileX, int tileY)
{
  if (isExitDoorTile(tileX, tileY))
  {
    return isExitDoorOpen() ? output : closed;
  }

  const bool hasLeft = hasAdjacentScreen(FacingDirection::Left);
  const bool hasRight = hasAdjacentScreen(FacingDirection::Right);
  const bool hasUp = hasAdjacentScreen(FacingDirection::Up);
  const bool hasDown = hasAdjacentScreen(FacingDirection::Down);

  // Corners first, then straights.
  if (tileX == 0 && tileY == 0 && !hasLeft && !hasUp)
    return resolveWallVariant(angle, 0);
  if (tileX == TILE_COLS - 1 && tileY == 0 && !hasRight && !hasUp)
    return resolveWallVariant(angle, 1);
  if (tileX == 0 && tileY == TILE_ROWS - 1 && !hasLeft && !hasDown)
    return resolveWallVariant(angle, 3);
  if (tileX == TILE_COLS - 1 && tileY == TILE_ROWS - 1 && !hasRight && !hasDown)
    return resolveWallVariant(angle, 2);

  if (tileX == 0)
  {
    if (!hasLeft)
      return resolveWallVariant(straight, 0);
    if (!hasUp && tileY == 0)
      return resolveWallVariant(straight, 1);
    if (!hasDown && tileY == TILE_ROWS - 1)
      return resolveWallVariant(straight, 1);
  }

  if (tileX == TILE_COLS - 1)
  {
    if (!hasRight)
      return resolveWallVariant(straight, 0);
    if (!hasUp && tileY == 0)
      return resolveWallVariant(straight, 1);
    if (!hasDown && tileY == TILE_ROWS - 1)
      return resolveWallVariant(straight, 1);
  }

  if (tileY == 0)
  {
    if (!hasUp && tileX >= 1 && tileX <= TILE_COLS - 2)
      return resolveWallVariant(straight, 1);
    if (!hasLeft && tileX == 0)
      return resolveWallVariant(straight, 0);
    if (!hasRight && tileX == TILE_COLS - 1)
      return resolveWallVariant(straight, 0);
  }

  if (tileY == TILE_ROWS - 1)
  {
    if (!hasDown && tileX >= 1 && tileX <= TILE_COLS - 2)
      return resolveWallVariant(straight, 3);
    if (!hasLeft && tileX == 0)
      return resolveWallVariant(straight, 0);
    if (!hasRight && tileX == TILE_COLS - 1)
      return resolveWallVariant(straight, 0);
  }

  return nullptr;
}

bool collidesWithSolidWallSprite(int x, int y)
{
  int minTileX = x / TILE_SIZE;
  int maxTileX = (x + PLAYER_WIDTH - 1) / TILE_SIZE;
  int minTileY = y / TILE_SIZE;
  int maxTileY = (y + PLAYER_HEIGHT - 1) / TILE_SIZE;

  if (minTileX < 0)
    minTileX = 0;
  if (minTileY < 0)
    minTileY = 0;
  if (maxTileX >= TILE_COLS)
    maxTileX = TILE_COLS - 1;
  if (maxTileY >= TILE_ROWS)
    maxTileY = TILE_ROWS - 1;

  for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
  {
    for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
    {
      const uint8_t *tileSprite = outerWallSpriteAtTile(tileX, tileY);
      if (tileSprite && isSolidWallSprite(tileSprite))
      {
        int tilePx = tileX * TILE_SIZE;
        int tilePy = tileY * TILE_SIZE;
        if (x < tilePx + TILE_SIZE &&
            x + PLAYER_WIDTH > tilePx &&
            y < tilePy + TILE_SIZE &&
            y + PLAYER_HEIGHT > tilePy)
        {
          return true;
        }
      }
    }
  }

  return false;
}

// Collision detection functions
bool canMoveTo(int x, int y)
{
  MovementBounds bounds = getMovementBounds();
  bool insideBounds = x >= bounds.left &&
                      x + PLAYER_WIDTH <= bounds.right &&
                      y >= bounds.top &&
                      y + PLAYER_HEIGHT <= bounds.bottom;

  if (!insideBounds)
  {
    // Allow entering the opened exit door tile even though it sits on an outer wall.
    bool canStepIntoOpenedDoor = isExitDoorOpen() &&
                                 overlapsExitDoorTile(x, y) &&
                                 x <= exitDoorPixelX();
    if (!canStepIntoOpenedDoor)
    {
      return false;
    }
  }

  return !collidesWithSolidWallSprite(x, y);
}

bool rectanglesOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
  return x1 < x2 + w2 &&
         x1 + w1 > x2 &&
         y1 < y2 + h2 &&
         y1 + h1 > y2;
}

const uint8_t *spriteForItem(ItemType type)
{
  if (type == ItemType::Sword)
    return sword;
  if (type == ItemType::String)
    return string;
  return key;
}

void setInventoryItem(ItemType type, bool value)
{
  if (type == ItemType::Sword)
    game.inventory.hasSword = value;
  else if (type == ItemType::String)
    game.inventory.hasString = value;
  else
    game.inventory.hasKey = value;
}

void resetItems()
{
  for (uint8_t i = 0; i < ITEM_COUNT; ++i)
  {
    levelItems[i] = initialItems[i];
  }
}

void collectItemsAtPlayerPosition()
{
  // In minotaur form, items are ignored and can be crossed without pickup.
  if (!game.player.isHuman)
  {
    return;
  }

  for (uint8_t i = 0; i < ITEM_COUNT; ++i)
  {
    if (levelItems[i].collected || levelItems[i].screen != game.currentScreen)
    {
      continue;
    }
    if (rectanglesOverlap(game.playerX, game.playerY, PLAYER_WIDTH, PLAYER_HEIGHT,
                          levelItems[i].x, levelItems[i].y, PLAYER_WIDTH, PLAYER_HEIGHT))
    {
      levelItems[i].collected = true;
      sound.tones(pickupSound);
      setInventoryItem(levelItems[i].type, true);
      if (levelItems[i].type == ItemType::Sword)
      {
        game.player.isArmed = true;
      }
    }
  }
}

void drawLevelItems()
{
  for (uint8_t i = 0; i < ITEM_COUNT; ++i)
  {
    if (!levelItems[i].collected && levelItems[i].screen == game.currentScreen)
    {
      Sprites::drawOverwrite(levelItems[i].x, levelItems[i].y, spriteForItem(levelItems[i].type), 0);
    }
  }
}

void restrictPlayerPosition()
{
  MovementBounds bounds = getMovementBounds();

  if (game.playerX < bounds.left)
    game.playerX = bounds.left;
  if (game.playerX + PLAYER_WIDTH > bounds.right)
    game.playerX = bounds.right - PLAYER_WIDTH;
  if (game.playerY < bounds.top)
    game.playerY = bounds.top;
  if (game.playerY + PLAYER_HEIGHT > bounds.bottom)
    game.playerY = bounds.bottom - PLAYER_HEIGHT;
}

void startGame()
{
  game.currentScreen = 0;
  game.playerX = level.spawnX;
  game.playerY = level.spawnY;
  game.facing = FacingDirection::Down;
  game.player.isHuman = true;
  game.player.isArmed = false;
  game.winJinglePlayed = false;
  game.loseJinglePlayed = false;
  game.minotaurAlive = true;
  game.inventory = {false, false, false};
  resetItems();
  restrictPlayerPosition();
}

void drawMinotaurNpc()
{
  if (game.currentScreen != MINOTAUR_SCREEN)
  {
    return;
  }

  if (game.minotaurAlive)
  {
    Sprites::drawOverwrite(MINOTAUR_X, MINOTAUR_Y, minleft, 0);
  }
  else
  {
    // Corpse sprite is a 90-degree lying variant.
    Sprites::drawOverwrite(MINOTAUR_X, MINOTAUR_Y, min_dead, 0);
  }
}

void handleMinotaurCollision()
{
  if (!game.minotaurAlive || game.currentScreen != MINOTAUR_SCREEN)
  {
    return;
  }

  if (!rectanglesOverlap(game.playerX, game.playerY, PLAYER_WIDTH, PLAYER_HEIGHT,
                         MINOTAUR_X, MINOTAUR_Y, PLAYER_WIDTH, PLAYER_HEIGHT))
  {
    return;
  }

  if (game.player.isArmed)
  {
    game.minotaurAlive = false;
    sound.tones(killSound);
  }
  else
  {
    game.mode = GameMode::Lose;
  }
}

bool tryReturnToMenu()
{
  if (arduboy.pressed(A_BUTTON) && arduboy.pressed(B_BUTTON))
  {
    game.mode = GameMode::Menu;
    return true;
  }
  return false;
}

bool hasAdjacentScreen(FacingDirection dir)
{
  uint8_t col = game.currentScreen % SCREEN_COLS;
  uint8_t row = game.currentScreen / SCREEN_COLS;

  switch (dir)
  {
  case FacingDirection::Right:
    return col < SCREEN_COLS - 1;
  case FacingDirection::Left:
    return col > 0;
  case FacingDirection::Down:
    return row < SCREEN_ROWS - 1;
  case FacingDirection::Up:
    return row > 0;
  }

  return false;
}

// Draw a wall sprite variant selected by rotation.
// rotation: 0=0°, 1=90° CW, 2=180°, 3=270° CW.
void drawWallVariant(int x, int y, const uint8_t *sprite, uint8_t rotation)
{
  Sprites::drawOverwrite(x, y, resolveWallVariant(sprite, rotation), 0);
}

void drawExitDoor()
{
  if (game.currentScreen != EXIT_DOOR_SCREEN)
  {
    return;
  }

  Sprites::drawOverwrite(exitDoorPixelX(), exitDoorPixelY(), isExitDoorOpen() ? output : closed, 0);
}

void drawOuterLabyrinthWalls()
{
  const bool hasLeft = hasAdjacentScreen(FacingDirection::Left);
  const bool hasRight = hasAdjacentScreen(FacingDirection::Right);
  const bool hasUp = hasAdjacentScreen(FacingDirection::Up);
  const bool hasDown = hasAdjacentScreen(FacingDirection::Down);

  // Draw only the external walls of the global 2x2 labyrinth using wall sprites.
  if (!hasLeft)
  {
    for (int y = 0; y < HEIGHT; y += 16)
    {
      drawWallVariant(0, y, straight, 0);
    }
  }
  else
  {
    // Transition edge: keep markers only where this side touches outer borders.
    if (!hasUp)
    {
      drawWallVariant(0, 0, straight, 1);
    }
    if (!hasDown)
    {
      drawWallVariant(0, HEIGHT - 16, straight, 1);
    }
  }

  if (!hasRight)
  {
    for (int y = 0; y < HEIGHT; y += 16)
    {
      drawWallVariant(WIDTH - 16, y, straight, 0);
    }
  }
  else
  {
    // Transition edge: keep markers only where this side touches outer borders.
    if (!hasUp)
    {
      drawWallVariant(WIDTH - 16, 0, straight, 1);
    }
    if (!hasDown)
    {
      drawWallVariant(WIDTH - 16, HEIGHT - 16, straight, 1);
    }
  }

  if (!hasUp)
  {
    for (int x = level.left; x <= level.right - 16; x += 16)
    {
      drawWallVariant(x, 0, straight, 1);
    }
  }
  else
  {
    // Transition edge: keep markers only where this side touches outer borders.
    if (!hasLeft)
    {
      drawWallVariant(0, 0, straight, 0);
    }
    if (!hasRight)
    {
      drawWallVariant(WIDTH - 16, 0, straight, 0);
    }
  }

  if (!hasDown)
  {
    for (int x = level.left; x <= level.right - 16; x += 16)
    {
      drawWallVariant(x, HEIGHT - 16, straight, 3);
    }
  }
  else
  {
    // Transition edge: keep markers only where this side touches outer borders.
    if (!hasLeft)
    {
      drawWallVariant(0, HEIGHT - 16, straight, 0);
    }
    if (!hasRight)
    {
      drawWallVariant(WIDTH - 16, HEIGHT - 16, straight, 0);
    }
  }

  // Corner caps
  if (!hasLeft && !hasUp)
  {
    drawWallVariant(0, 0, angle, 0);
  }
  if (!hasRight && !hasUp)
  {
    drawWallVariant(WIDTH - 16, 0, angle, 1);
  }
  if (!hasLeft && !hasDown)
  {
    drawWallVariant(0, HEIGHT - 16, angle, 3);
  }
  if (!hasRight && !hasDown)
  {
    drawWallVariant(WIDTH - 16, HEIGHT - 16, angle, 2);
  }

  drawExitDoor();
}

// Attempt to transition to an adjacent screen in the given direction.
// If no adjacent screen exists, the wall simply blocks movement.
void checkScreenTransition(FacingDirection dir)
{
  switch (dir)
  {
  case FacingDirection::Right:
    if (hasAdjacentScreen(FacingDirection::Right))
    {
      game.currentScreen++;
      game.playerX = 0;
    }
    break;
  case FacingDirection::Left:
    if (hasAdjacentScreen(FacingDirection::Left))
    {
      game.currentScreen--;
      game.playerX = WIDTH - PLAYER_WIDTH;
    }
    break;
  case FacingDirection::Down:
    if (hasAdjacentScreen(FacingDirection::Down))
    {
      game.currentScreen += SCREEN_COLS;
      game.playerY = 0;
    }
    break;
  case FacingDirection::Up:
    if (hasAdjacentScreen(FacingDirection::Up))
    {
      game.currentScreen -= SCREEN_COLS;
      game.playerY = HEIGHT - PLAYER_HEIGHT;
    }
    break;
  }

  restrictPlayerPosition();
}

void drawGameplayBackground()
{
  // Fill only the currently walkable area with the default empty tile.
  MovementBounds bounds = getMovementBounds();
  for (int x = bounds.left; x < bounds.right; x += 16)
  {
    for (int y = bounds.top; y < bounds.bottom; y += 16)
    {
      Sprites::drawOverwrite(x, y, empty, 0);
    }
  }

  // Keep only the external labyrinth walls over the default background.
  drawOuterLabyrinthWalls();
}

void handlePlayerStateInput()
{
  if (arduboy.justPressed(B_BUTTON) && game.inventory.hasSword)
  {
    game.player.isArmed = !game.player.isArmed;
  }
}

void drawGameplayHud()
{
  // Inventory labels
  if (game.inventory.hasSword)
  {
    arduboy.setCursor(HUD_SWORD_X, HUD_ITEMS_Y);
    arduboy.print("S");
  }
  if (game.inventory.hasString)
  {
    arduboy.setCursor(HUD_STRING_X, HUD_ITEMS_Y);
    arduboy.print("Y");
  }
  if (game.inventory.hasKey)
  {
    arduboy.setCursor(HUD_KEY_X, HUD_ITEMS_Y);
    arduboy.print("K");
  }

  // Mini-map: solid black 16x16, split into 4 cells
  arduboy.fillRect(MINIMAP_X, MINIMAP_Y, MINIMAP_SIZE, MINIMAP_SIZE, BLACK);
  arduboy.drawRect(MINIMAP_X, MINIMAP_Y, MINIMAP_SIZE, MINIMAP_SIZE, WHITE);
  arduboy.drawFastVLine(MINIMAP_X + MINIMAP_CELL_SIZE, MINIMAP_Y, MINIMAP_SIZE, WHITE);
  arduboy.drawFastHLine(MINIMAP_X, MINIMAP_Y + MINIMAP_CELL_SIZE, MINIMAP_SIZE, WHITE);

  uint8_t screenCol = game.currentScreen % SCREEN_COLS;
  uint8_t screenRow = game.currentScreen / SCREEN_COLS;
  int markerX = MINIMAP_X + screenCol * MINIMAP_CELL_SIZE + (MINIMAP_CELL_SIZE - MINIMAP_MARKER_SIZE) / 2;
  int markerY = MINIMAP_Y + screenRow * MINIMAP_CELL_SIZE + (MINIMAP_CELL_SIZE - MINIMAP_MARKER_SIZE) / 2;
  arduboy.fillRect(markerX, markerY, MINIMAP_MARKER_SIZE, MINIMAP_MARKER_SIZE, WHITE);
}

void handlePlayerMovement()
{
  // Process only one direction per frame to avoid double transitions.
  // Priority order when multiple inputs are held: LEFT > RIGHT > UP > DOWN.
  if (arduboy.pressed(LEFT_BUTTON))
  {
    game.facing = FacingDirection::Left;
    int newX = game.playerX - 1;
    if (canMoveTo(newX, game.playerY))
      game.playerX = newX;
    else
      checkScreenTransition(FacingDirection::Left);
  }
  else if (arduboy.pressed(RIGHT_BUTTON))
  {
    game.facing = FacingDirection::Right;
    int newX = game.playerX + 1;
    if (canMoveTo(newX, game.playerY))
      game.playerX = newX;
    else
      checkScreenTransition(FacingDirection::Right);
  }
  else if (arduboy.pressed(UP_BUTTON))
  {
    game.facing = FacingDirection::Up;
    int newY = game.playerY - 1;
    if (canMoveTo(game.playerX, newY))
      game.playerY = newY;
    else
      checkScreenTransition(FacingDirection::Up);
  }
  else if (arduboy.pressed(DOWN_BUTTON))
  {
    game.facing = FacingDirection::Down;
    int newY = game.playerY + 1;
    if (canMoveTo(game.playerX, newY))
      game.playerY = newY;
    else
      checkScreenTransition(FacingDirection::Down);
  }
}

MenuOption selectedMenuOption()
{
  uint8_t optionIndex = (game.menuCursorY - MENU_CURSOR_TOP) / MENU_CURSOR_STEP;
  if (optionIndex == 0)
  {
    return MenuOption::Play;
  }
  if (optionIndex == 1)
  {
    return MenuOption::Help;
  }
  return MenuOption::Credits;
}

void setup()
{
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.clear();
}

void loop()
{
  if (!arduboy.nextFrame())
  {
    return;
  }
  arduboy.clear();
  arduboy.pollButtons();

  switch (game.mode)
  {
  case GameMode::Help:
    handleHelpScreen();
    break;
  case GameMode::Splash:
    handleSplashScreen();
    break;
  case GameMode::Game:
    handleGameplay();
    break;
  case GameMode::Win:
    handleWinScreen();
    break;
  case GameMode::Lose:
    handleLoseScreen();
    break;
  case GameMode::Menu:
    handleMenu();
    break;
  }
}

void handleHelpScreen()
{
  if (tryReturnToMenu())
  {
    return;
  }

  Sprites::drawOverwrite(0, 0, background, 0);

  // Overlay controls legend to distinguish Help from Menu
  arduboy.setCursor(2, 2);
  arduboy.print(F("-- CONTROLS --"));
  arduboy.setCursor(2, 14);
  arduboy.print(F("Arrows : move"));
  arduboy.setCursor(2, 22);
  arduboy.print(F("B      : equip sword"));
  arduboy.setCursor(2, 30);
  arduboy.print(F("A+B    : menu"));

  arduboy.display();
}

void handleSplashScreen()
{
  if (tryReturnToMenu())
  {
    return;
  }

  Sprites::drawOverwrite(0, 0, splashScreens[game.splashIndex], 0);

  if (arduboy.justPressed(A_BUTTON))
  {
    game.splashIndex = (game.splashIndex + 1) % SPLASH_COUNT;
  }

  arduboy.display();
}

void handleGameplay()
{
  if (tryReturnToMenu())
  {
    return;
  }

  drawGameplayBackground();
  handlePlayerStateInput();
  drawGameplayHud();

  // Collect items first, then draw so pickup takes effect in the same frame
  collectItemsAtPlayerPosition();
  drawLevelItems();
  drawMinotaurNpc();

  handlePlayerMovement();
  handleMinotaurCollision();

  if (game.mode == GameMode::Lose)
  {
    return;
  }

  if (hasPlayerEscaped())
  {
    game.mode = GameMode::Win;
    return;
  }

  // Single display call per frame covers all state changes (movement, HUD, items)
  drawPlayerSprite();
  arduboy.display();
}

void handleWinScreen()
{
  if (tryReturnToMenu())
  {
    return;
  }

  if (!game.winJinglePlayed)
  {
    sound.tones(winJingle);
    game.winJinglePlayed = true;
  }

  arduboy.setCursor(22, 28);
  arduboy.print(F("You are free !"));
  arduboy.display();
}

void handleLoseScreen()
{
  if (tryReturnToMenu())
  {
    return;
  }

  if (!game.loseJinglePlayed)
  {
    sound.tones(loseSound);
    game.loseJinglePlayed = true;
  }

  arduboy.setCursor(24, 28);
  arduboy.print(F("You're dead..."));
  arduboy.display();
}

void handleMenu()
{
  if (arduboy.justPressed(UP_BUTTON))
  {
    if (game.menuCursorY > MENU_CURSOR_TOP)
    {
      game.menuCursorY = game.menuCursorY - MENU_CURSOR_STEP;
    }
  }
  else if (arduboy.justPressed(DOWN_BUTTON))
  {
    if (game.menuCursorY < MENU_CURSOR_BOTTOM)
    {
      game.menuCursorY = game.menuCursorY + MENU_CURSOR_STEP;
    }
  }
  else if (arduboy.justPressed(B_BUTTON))
  {
    MenuOption option = selectedMenuOption();
    if (option == MenuOption::Play)
    {
      startGame();
      game.mode = GameMode::Game;
    }
    else if (option == MenuOption::Help)
    {
      game.mode = GameMode::Help;
    }
    else if (option == MenuOption::Credits)
    {
      game.splashIndex = 0;
      game.mode = GameMode::Splash;
    }
  }

  Sprites::drawOverwrite(0, 0, background, 0);
  Sprites::drawOverwrite(MENU_CURSOR_X, game.menuCursorY, select, 0);
  arduboy.display();
}

void drawPlayerSprite()
{
  const uint8_t *sprite = nullptr;

  if (!game.player.isHuman)
  {
    if (game.facing == FacingDirection::Left)
      sprite = minleft;
    else if (game.facing == FacingDirection::Right)
      sprite = minright;
    else if (game.facing == FacingDirection::Up)
      sprite = minback;
    else
      sprite = minfront;
  }
  else if (game.player.isArmed)
  {
    if (game.facing == FacingDirection::Left)
      sprite = armed_left;
    else if (game.facing == FacingDirection::Right)
      sprite = armed_right;
    else if (game.facing == FacingDirection::Up)
      sprite = armed_back;
    else
      sprite = armed_front;
  }
  else
  {
    if (game.facing == FacingDirection::Left)
      sprite = left;
    else if (game.facing == FacingDirection::Right)
      sprite = right;
    else if (game.facing == FacingDirection::Up)
      sprite = back;
    else
      sprite = front;
  }

  Sprites::drawOverwrite(game.playerX, game.playerY, sprite, 0);
}
