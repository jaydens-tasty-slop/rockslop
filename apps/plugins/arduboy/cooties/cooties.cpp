#include <stdlib.h>
#include <math.h>
#include "general.h"

#include "Levels/Level0.h"

void updateTitleScreen()
{
  anim();
  sprite.drawOverwrite(3, 1, title_sp, title_frame);
  sprite.drawOverwrite(14, 46, push_sp, push_frame);
}

void updateGame()
{
  drawRoom();

  drawCooties();
  moveEnemy();
  
  moveBullets();
  drawBullets();

  setLife();

  if (waitCount != 0) {
    --waitCount;
  }

  if (hero.iframe != 0){
    --hero.iframe;
  }

  // Draw hero
  sprite.drawPlusMask(hero.x, hero.y, hero_sp, hero.frame);

  // Life
  arduboy.fillRect(0, 0, 128, 10, BLACK);
  arduboy.fillRect(0, 10, 128, 1, WHITE);
  sprite.drawPlusMask(arduboy.width() - 28, 0, hearts_sp, hearts_frame);
  arduboy.fillRect(arduboy.width() - 28, 0, hide_w, 8, INVERT);

  checkCollisions();

  // Points
  font3x5.setCursor(0, 0);
  font3x5.print("Points: ");
  font3x5.setCursor(3 * 10, 0);
  font3x5.print(hitCount);

  // Level
  font3x5.setCursor((arduboy.width() / 2) - 6, 0);
  font3x5.print("Level: ");
  font3x5.setCursor(((arduboy.width() / 2) - 6) + 8 * 3, 0);
  font3x5.print(level);
}

void updateGameOver()
{
  // Game over screen
  sprite.drawOverwrite(4, 15, over, 0);
  sprite.drawOverwrite(14, 35, push_sp, push_frame);

  arduboy.setCursor(48, 52);
  arduboy.print("Score: ");
  arduboy.setCursor(48 + 5 * 7, 52);
  arduboy.print(hitCount * 10);
}

void setup()
{
  arduboy.begin();
  arduboy.setFrameRate(60);

  initBullets();
  initEnemies();
  loadRoom(room0);

  hero.x = 64;
  hero.y = 32;
  hero.width = 8;
  hero.height = 8;

  hitCount = 0;

  level = 1;

  State::Title;
}

void loop()
{
  if (!arduboy.nextFrame())
    return;

  arduboy.clear();
  input();

  switch (state) {
    case State::Title:
      updateTitleScreen();
      break;

    case State::Game:
      updateGame();
      break;

    case State::GameOver:
      updateGameOver();
      break;
  }

  arduboy.display();
}
