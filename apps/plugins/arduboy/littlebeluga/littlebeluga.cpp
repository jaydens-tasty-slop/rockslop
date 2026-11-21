#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "sound.h"
#include "sprites.h"

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
constexpr uint8_t BELUGA_WIDTH = 15;
constexpr uint8_t BELUGA_HEIGHT = 15;
constexpr int16_t BELUGA_X = 18;
constexpr uint8_t OBSTACLE_WIDTH = 8;
constexpr uint8_t GAP_HEIGHT = 24;
constexpr uint8_t OBSTACLE_SPACING = 40;
constexpr uint8_t OBSTACLE_COUNT = 4;
constexpr uint8_t SWIM_SPEED = 2;
constexpr int16_t MIN_GAP_Y = 8;
constexpr int16_t MAX_GAP_Y = HEIGHT - GAP_HEIGHT - 8;
constexpr uint8_t SPEED_START_TENTHS = 7;
constexpr uint8_t SPEED_MAX_TENTHS = 22;
constexpr uint8_t SPEED_STEP_TENTHS = 1;
constexpr uint16_t SPEED_STEP_INTERVAL_FRAMES = 180;
constexpr uint16_t LEVEL_DISTANCE_UNITS = 900;
constexpr uint8_t PROGRESS_BAR_X = 30;
constexpr uint8_t PROGRESS_BAR_Y = 58;
constexpr uint8_t PROGRESS_BAR_W = 96;
constexpr uint8_t PROGRESS_BAR_H = 6;
constexpr uint8_t ALL_BUTTONS = UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON |
                                RIGHT_BUTTON | A_BUTTON | B_BUTTON;

enum class GameMode : uint8_t {
  Title,
  Running,
  GameOver,
  LevelComplete
};

struct Obstacle {
  int16_t x;
  int16_t gapY;
  bool scored;
};

GameMode gameMode = GameMode::Title;
Obstacle obstacles[OBSTACLE_COUNT];
int16_t belugaY = (HEIGHT - BELUGA_HEIGHT) / 2;
uint16_t score = 0;
uint16_t distanceRemaining = LEVEL_DISTANCE_UNITS;
uint16_t speedStepCounter = 0;
uint16_t scrollAccumulatorTenths = 0;
uint8_t currentSpeedTenths = SPEED_START_TENTHS;

bool anyButtonJustPressed() {
  return arduboy.justPressed(ALL_BUTTONS);
}

void enterTitle() {
  gameMode = GameMode::Title;
  sound.noTone();
  sound.tones(titleScreenScore);
}

void resetRunner() {
  belugaY = (HEIGHT - BELUGA_HEIGHT) / 2;
  score = 0;
  distanceRemaining = LEVEL_DISTANCE_UNITS;
  speedStepCounter = 0;
  scrollAccumulatorTenths = 0;
  currentSpeedTenths = SPEED_START_TENTHS;

  for (uint8_t i = 0; i < OBSTACLE_COUNT; ++i) {
    obstacles[i].x = WIDTH + (i * OBSTACLE_SPACING);
    obstacles[i].gapY = random(MIN_GAP_Y, MAX_GAP_Y + 1);
    obstacles[i].scored = false;
  }
}

void startRunner() {
  resetRunner();
  gameMode = GameMode::Running;
  sound.noTone();
}

bool collidesWithObstacle(const Obstacle &obstacle) {
  const bool overlapsX = (BELUGA_X + BELUGA_WIDTH > obstacle.x) &&
                         (BELUGA_X < obstacle.x + OBSTACLE_WIDTH);
  if (!overlapsX) {
    return false;
  }

  const bool insideGap = (belugaY >= obstacle.gapY) &&
                         (belugaY + BELUGA_HEIGHT <= obstacle.gapY + GAP_HEIGHT);
  return !insideGap;
}

void updateRunner() {
  if (arduboy.pressed(UP_BUTTON)) {
    belugaY -= SWIM_SPEED;
  }
  if (arduboy.pressed(DOWN_BUTTON)) {
    belugaY += SWIM_SPEED;
  }

  if (belugaY < 0) {
    belugaY = 0;
  }
  if (belugaY > HEIGHT - BELUGA_HEIGHT) {
    belugaY = HEIGHT - BELUGA_HEIGHT;
  }

  int16_t farthestX = obstacles[0].x;
  for (uint8_t i = 1; i < OBSTACLE_COUNT; ++i) {
    if (obstacles[i].x > farthestX) {
      farthestX = obstacles[i].x;
    }
  }

  ++speedStepCounter;
  if (speedStepCounter >= SPEED_STEP_INTERVAL_FRAMES) {
    speedStepCounter = 0;
    if (currentSpeedTenths < SPEED_MAX_TENTHS) {
      currentSpeedTenths += SPEED_STEP_TENTHS;
    }
  }

  scrollAccumulatorTenths += currentSpeedTenths;
  const uint16_t scrollPixels = scrollAccumulatorTenths / 10;
  scrollAccumulatorTenths %= 10;

  if (scrollPixels == 0) {
    return;
  }

  if (distanceRemaining > scrollPixels) {
    distanceRemaining -= scrollPixels;
  } else {
    distanceRemaining = 0;
  }

  for (uint8_t i = 0; i < OBSTACLE_COUNT; ++i) {
    obstacles[i].x -= scrollPixels;

    if (!obstacles[i].scored && obstacles[i].x + OBSTACLE_WIDTH < BELUGA_X) {
      obstacles[i].scored = true;
      ++score;
      sound.tones(bubbleGenerate);
    }

    if (obstacles[i].x + OBSTACLE_WIDTH < 0) {
      obstacles[i].x = farthestX + OBSTACLE_SPACING;
      obstacles[i].gapY = random(MIN_GAP_Y, MAX_GAP_Y + 1);
      obstacles[i].scored = false;
      farthestX = obstacles[i].x;
    }

    if (collidesWithObstacle(obstacles[i])) {
      gameMode = GameMode::GameOver;
      sound.noTone();
      sound.tones(bubblePop);
      return;
    }
  }

  if (distanceRemaining == 0) {
    gameMode = GameMode::LevelComplete;
    sound.tones(bubbleGenerate);
  }
}

void drawRunner() {
  for (uint8_t i = 0; i < OBSTACLE_COUNT; ++i) {
    arduboy.fillRect(obstacles[i].x, 0, OBSTACLE_WIDTH, obstacles[i].gapY, WHITE);
    arduboy.fillRect(obstacles[i].x, obstacles[i].gapY + GAP_HEIGHT, OBSTACLE_WIDTH,
                     HEIGHT - (obstacles[i].gapY + GAP_HEIGHT), WHITE);
  }

  arduboy.drawBitmap(BELUGA_X, belugaY, beluga + 2, BELUGA_WIDTH, BELUGA_HEIGHT, WHITE);
  arduboy.setCursor(0, 0);
  arduboy.print(F("Score:"));
  arduboy.print(score);
  arduboy.setCursor(78, 0);
  arduboy.print(currentSpeedTenths / 10);
  arduboy.print('.');
  arduboy.print(currentSpeedTenths % 10);

  arduboy.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_W, PROGRESS_BAR_H, WHITE);
  const uint8_t innerWidth = PROGRESS_BAR_W - 2;
  uint8_t fillWidth = 0;
  if (LEVEL_DISTANCE_UNITS > 0) {
    fillWidth = (uint32_t)innerWidth * distanceRemaining / LEVEL_DISTANCE_UNITS;
  }
  arduboy.fillRect(PROGRESS_BAR_X + 1, PROGRESS_BAR_Y + 1, fillWidth, PROGRESS_BAR_H - 2, WHITE);
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.audio.on();
  randomSeed(arduboy.generateRandomSeed());
  resetRunner();
  enterTitle();
}

void loop() {
  if (!arduboy.nextFrame()) {
      return;
  }

  arduboy.pollButtons();

  if (gameMode == GameMode::Title) {
    if (arduboy.buttonsState() != 0) {
      startRunner();
    }

    arduboy.drawBitmap(0, 0, titleScreen, WIDTH, HEIGHT, WHITE);
    arduboy.display(CLEAR_BUFFER);
    return;
  }

  if (gameMode == GameMode::Running) {
    updateRunner();
    drawRunner();
  } else if (gameMode == GameMode::GameOver) {
    arduboy.setCursor(22, 20);
    arduboy.print(F("Nighty-night!"));
    arduboy.setCursor(22, 32);
    arduboy.print(F("Score: "));
    arduboy.print(score);
    arduboy.setCursor(22, 46);
    arduboy.print(F("Press any key"));
  } else {
    arduboy.setCursor(18, 20);
    arduboy.print(F("You made it to mama!"));
    arduboy.setCursor(22, 32);
    arduboy.print(F("Score: "));
    arduboy.print(score);
    arduboy.setCursor(22, 46);
    arduboy.print(F("Press any key"));
  }

  if ((gameMode == GameMode::GameOver || gameMode == GameMode::LevelComplete) &&
      anyButtonJustPressed()) {
    startRunner();
  }

  arduboy.display(CLEAR_BUFFER);
}
