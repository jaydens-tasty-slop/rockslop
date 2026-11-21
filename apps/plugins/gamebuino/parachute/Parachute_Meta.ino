// -------------------------------------------------------------------------
// Game & Watch Parachute © 2019 Jicehel (https://gamebuino.com/@jicehel)
// Based on Nintendo Game & Watch Parachute
// -------------------------------------------------------------------------
// Many thanks to Andy (https://gamebuino.com/@aoneill) for the routines
// related to the DMA controller to use High Definition 160x128 on the META
// and to Steph (https://gamebuino.com/@steph) for his help and for the
// structure of games like Nintendo's Game & Watch console series, in High
// Definition on the META.
// -------------------------------------------------------------------------
// Special thanks to Pharap for his help to improve the source
// -------------------------------------------------------------------------

// includes the official library
#include <Gamebuino-Meta.h>

// includes other parts of pogram
#include "Global.h"
#include "pictures.h"
#include "Sprites.h"
#include "parachute_proto.h"

// -------------------------------------------------------------------------
// Player definition
// -------------------------------------------------------------------------

struct Player {
  uint8_t  spriteIndex;
};

Player player = {
  0      // spriteIndex 0, the position at the left
};

// -------------------------------------------------------------------------
// Initialization related to the DMA controller
// -------------------------------------------------------------------------

namespace Gamebuino_Meta {

#define DMA_DESC_COUNT 3
extern volatile uint32_t dma_desc_free_count;

static inline void wait_for_transfers_done(void) {
  while (dma_desc_free_count < DMA_DESC_COUNT);
}
static SPISettings tftSPISettings = SPISettings(24000000, MSBFIRST, SPI_MODE0);
};

// rendering buffers
uint16_t buffer1[screenWidth * sliceHeight];
uint16_t buffer2[screenWidth * sliceHeight];

// flag for an ongoing data transfer
bool drawPending = false;

// -------------------------------------------------------------------------
// Initialization of the META
// -------------------------------------------------------------------------

void setup() {
  // initializes the META
  gb.begin();
  SerialUSB.begin(9600);
  // default screen buffer won't be used
  // so it must be set to 0x0 pixels
  gb.display.init(0, 0, ColorMode::rgb565);
  gameState = GameState::stateHome;
}


// -------------------------------------------------------------------------
// Initialization of the game
// -------------------------------------------------------------------------

void initGame() {

  gb.lights.fill(BLACK);
  gameState = GameState::stateRun;
  gb.sound.playTick();
  misses = 0;
  score = 0;
  player.spriteIndex = 0;
  spawnDelay = 4;       // Delay before next possibility to launch a para
  sharkAnimation = -2;  
  floodedAnimation = -2;
  speedMax = 25;   // Number of cycles before update
  helicopterAnimation = 0;
  player.spriteIndex = 0;
  speedBlades = 5;
  moveTick = speedMax;
  spawnCount = spawnDelay;
  nbParachutesLaunched = 0;
  for (auto &value : parachutes) value = -1;
}


// -------------------------------------------------------------------------
// Main control loop
// -------------------------------------------------------------------------

void loop() {

  gb.waitForUpdate();

  switch (gameState) {

    case GameState::stateHome: // Start screen
      if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
        initGame();
      } else {
        drawScreen();
      }
      break;

    case GameState::stateRun: // Game running
      // a classic sequence :)
      if (misses == 3) gameState = GameState::stateGameOver;
      else {
        update();
        draw();
      }
      break;

    case GameState::stateGameOver:  // GameOver screen
      if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
        gameState = GameState::stateHome;
        gb.sound.play("gameOver.wav");
      } else {
        drawScreen();
      }
      break;

    case GameState::statePause: // Pause Game
      if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
        gameState = GameState::stateRun;
        gb.sound.play("pause.wav");
      } else {
        drawScreen();
      }
      break;

  }
}


// -------------------------------------------------------------------------
// Game logic
// -------------------------------------------------------------------------

void update() {

  // -------------------------------------------------------------------------
  // User interaction
  // -------------------------------------------------------------------------

  if ((gb.buttons.pressed(BUTTON_LEFT)) && (player.spriteIndex > 0))  {
    --player.spriteIndex;
    gb.sound.playTick();
  } else if ((gb.buttons.pressed(BUTTON_RIGHT)) && (player.spriteIndex < 2)) {
    ++player.spriteIndex;
    gb.sound.playTick();
  }
  if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
    gameState = GameState::statePause;
    gb.sound.play("pause.wav");
  }

  // -------------------------------------------------------------------------
  // Anim objects
  // -------------------------------------------------------------------------

  animShark();
  animHelicopter();
  animPara();       // Animate paratrooper
  // anim_helico(); // Animtie helicopter
  if (floodedAnimation > -2) animFlooded(); // Animate Flooded if needed
  if (moveTick > 0) {
    --moveTick;
  } else {
    --spawnCount;
    if ((spawnCount < 1) && (random(6 - (score / 200)) < 4)) {   // Check if we launch a new paatrooper
      uint8_t temp = random(0, 3);
      if (temp == 0) parachutes[nbParachutesLaunched] = firstSpriteColonne0;
      else if (temp == 1) parachutes[nbParachutesLaunched] = firstSpriteColonne1;
      else if (temp == 2) parachutes[nbParachutesLaunched] = firstSpriteColonne2;
      ++nbParachutesLaunched;
      spawnCount = spawnDelay - int(score / 60);
    }
    moveTick = speedMax - (score / 75);
  }
  if ((score > 0)  && (score % 500 == 0)) {
    if (misses > 0) {
      --misses;
      gb.sound.play("Chance.wav");
    }
  }
}

/* void debug(char* message) {
  SerialUSB.print(message);
  SerialUSB.print(" - nbParachutesLaunched : ");
  SerialUSB.print(nbParachutesLaunched);
  SerialUSB.print(" - moveTick: ");
  SerialUSB.print(moveTick);
  SerialUSB.print(" - spawnCount: ");
  SerialUSB.println(spawnCount);

  uint8_t compteur;
  SerialUSB.print("Parachute : ");
  for (compteur = 0 ; compteur < nbParachutesLaunched ; compteur++)  {
    SerialUSB.print("compteur: ");
    SerialUSB.print(compteur);
    SerialUSB.print(" - ");
    SerialUSB.print(parachutes[compteur]);
    SerialUSB.print("; ");
  }
  SerialUSB.println("");
  } */

void animShark() {
  if (moveTick <= 0) {
    if ((sharkAnimation > -2) || ((sharkAnimation == -2) && (random(0, 50) < 3))) {
      ++sharkAnimation;
      if (sharkAnimation > -1) gb.sound.playTick();
    }
    if (sharkAnimation > 4) {
      sharkAnimation = -2;
      if (floodedAnimation > 4) floodedAnimation = -2;
    }
  }
}

void animHelicopter() {
  if (helicopterAnimation >  2) helicopterAnimation = -1;
  helicopterAnimation++;
}

void animFlooded() {
  if ((moveTick <= 0) && (floodedAnimation < 6)) floodedAnimation++;
}

void testBoat(uint8_t colonne, uint8_t para_courant) {
  // Test if boat is unde the paratrooper => score or tansform the para as swimmer ...
  for (size_t count = para_courant; count < nbParachutesLaunched ; count++) {
    parachutes[count] = parachutes[count + 1];
    nbParachutesLaunched --;
  }
  if (colonne == 1 + player.spriteIndex ) {
    ++score;
    gb.sound.playOK();
  } else {
    ++misses;
    gb.sound.playCancel();
    floodedAnimation = 2 - colonne;
    sharkAnimation = 2 - colonne;
  }
}

void animPara() {
  if (moveTick <= 0) {
    for (size_t compteur = 0 ; compteur < nbParachutesLaunched ; ++compteur)  {
      if (parachutes[compteur] > -1) {
        if ((parachutes[compteur] < (firstSpriteColonne1-1)) || ((parachutes[compteur] >= firstSpriteColonne1) && (parachutes[compteur] < (firstSpriteColonne1-1))) || ((parachutes[compteur] >= firstSpriteColonne2) && (parachutes[compteur] < lastSpriteColonne2)))  {
          ++parachutes[compteur];  // Next step fo para fall
          gb.sound.playTick();
        } else if (parachutes[compteur] == (firstSpriteColonne1 - 1)) {
          testBoat(1, compteur);   // Test if paratrooper is at last step of colonne 0
        } else if (parachutes[compteur] == (firstSpriteColonne2 - 1)) {
          testBoat(2, compteur);   // Test if paratrooper is at last step of colonne 1
        } else if (parachutes[compteur] == lastSpriteColonne2) {
          testBoat(3, compteur);   // Test if paratrooper is at last step of colonne 2
        }
      }
    }
  }
}


// -------------------------------------------------------------------------
// Graphic rendering
// -------------------------------------------------------------------------

void drawScreen() {
  // declares a pointer that will alternate between the two memory buffers
  uint16_t* buffer;
  // declares the top border of current slice
  uint8_t sliceY;
  // then we go through each slice one by one
  for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
    // buffers are switched according to the parity of sliceIndex
    buffer = sliceIndex % 2 == 0 ? buffer1 : buffer2;
    // the top border of the current slice is calculated
    sliceY = sliceIndex * sliceHeight;

    // starts by drawing the background
    switch (gameState) {

      case GameState::stateHome: // Start screen
        if (gb.buttons.released(BUTTON_A) || gb.buttons.released(BUTTON_B)) {
          initGame();
          return;
        } else {
          picRead(buffer, PIC_SPLATCH, 0, sliceY, screenWidth * sliceHeight);
        }
        break;

      case GameState::stateGameOver: // GameOver screen
        if (gb.buttons.released(BUTTON_A) || gb.buttons.released(BUTTON_B)) {
          gameState = GameState::stateHome;
          return;
        } else {
          picRead(buffer, PIC_BACKGROUND, 0, sliceY, screenWidth * sliceHeight);
          // and finally draws the pause sprite
          drawText(spriteGameOver, sliceY, buffer, 53, 60);
        }
        break;

      case GameState::statePause:  // Pause screen
        if (gb.buttons.released(BUTTON_A) || gb.buttons.released(BUTTON_B)) {
          gameState = GameState::stateRun;
          return;
        } else {
          picRead(buffer, PIC_BACKGROUND, 0, sliceY, screenWidth * sliceHeight);
          // and finally draws the pause sprite
          drawText(spritePause, sliceY, buffer, 66, 60);
        }
        break;
    }

    // then we make sure that the sending of the previous buffer
    // to the DMA controller has taken place
    if (sliceIndex != 0) waitForPreviousDraw();
    // after which we can then send the current buffer
    customDrawBuffer(0, sliceY, buffer, screenWidth, sliceHeight);
  }

  // always wait until the DMA transfer is completed
  // for the last slice before entering the next cycle
  waitForPreviousDraw();
}

void draw() {
  // Declares a pointer that will alternate between the two memory buffers
  uint16_t* buffer;
  // Declares the top border of current slice
  uint8_t sliceY;
  // Go through each slice one by one
  for (uint8_t sliceIndex = 0; sliceIndex < slices; sliceIndex++) {
    // Buffers are switched according to the parity of sliceIndex
    buffer = sliceIndex % 2 == 0 ? buffer1 : buffer2;
    // Top border of the current slice is calculated
    sliceY = sliceIndex * sliceHeight;

    // Starts by drawing the background
    picRead(buffer, PIC_BACKGROUND, 0, sliceY, screenWidth * sliceHeight);


    // then draws helicopter (a static sprite)
    drawSprite(helicopter, sliceY, buffer);

    // then draw blades
    if ((helicopterAnimation == 1) || (helicopterAnimation == 3)) {
      drawSprite(blades[0], sliceY, buffer);
      drawSprite(blades[1], sliceY, buffer);
    }
    if (helicopterAnimation == 2) {
      drawSprite(blades[2], sliceY, buffer);
      drawSprite(blades[3], sliceY, buffer);
    }

    if (misses > 0) {
      drawSprite(miss[0], sliceY, buffer);
      drawSprite(miss[1], sliceY, buffer);
      if (misses > 1) {
        drawSprite(miss[2], sliceY, buffer);
        if (misses > 2) {
          drawSprite(miss[3], sliceY, buffer);
        }
      }
    }

    // Draw parachutes
    uint8_t compteur;
    for (compteur = 0 ; compteur < nbParachutesLaunched ; compteur++) drawSprite(para[parachutes[compteur]], sliceY, buffer);

    // Draw flooded
    if ((floodedAnimation > -1) && (floodedAnimation < 6)) drawSprite(flooded[floodedAnimation], sliceY, buffer);

    // Draw shark if needed
    if ((sharkAnimation > -1) && (floodedAnimation < 6)) drawSprite(shark[sharkAnimation], sliceY, buffer);

    // Draws the boat of the player
    drawSprite(boat[player.spriteIndex], sliceY, buffer);

    // Draw score
    drawScore(score, sliceY, buffer);

    // Verify that previous buffer has been sent to the DMA controller
    if (sliceIndex != 0) waitForPreviousDraw();
    // Then send the current buffer
    customDrawBuffer(0, sliceY, buffer, screenWidth, sliceHeight);
  }

  // Wait until the DMA transfer is completed before entering the next cycle
  waitForPreviousDraw();
}

void drawSprite(Sprite sprite, uint8_t sliceY, uint16_t* buffer) {
  // Check if sprite has one part to show on the current slice
  if (sliceY < sprite.y + sprite.h && sprite.y < sliceY + sliceHeight) {
    // Determines the boundaries of the sprite surface within the current slice
    uint8_t  xMin = sprite.x;
    uint8_t  xmax = sprite.x + sprite.w - 1;
    uint8_t  yMin = sprite.y < sliceY ? sliceY : sprite.y;
    uint8_t  yMax = sprite.y + sprite.h >= sliceY + sliceHeight ? sliceY + sliceHeight - 1 : sprite.y + sprite.h - 1;
    uint8_t  px, py;
    uint16_t color;
    // Display the sprite pixels to be drawn
    uint16_t row[screenWidth];
    for (py = yMin; py <= yMax; py++) {
      // Picks the pixel colors of this row from the spritesheet
      picRead(row, sprite.spritesheet == idSpritesheetA ? PIC_SHEET_A : PIC_SHEET_B,
              xMin, py, xmax - xMin + 1);
      for (px = xMin; px <= xmax; px++) {
        color = row[px - xMin];
        // It colo is different from the transparency color
        if (color != transColor) {
          // Copies the color code into the rendering buffer
          buffer[px + (py - sliceY) * screenWidth] = color;
        }
      }
    }
  }
}

void drawText(Sprite sprite, uint8_t sliceY, uint16_t* buffer, uint8_t x, uint8_t y) {
  if (sliceY < y + sprite.h && y < sliceY + sliceHeight) {
    uint8_t xMin = x;
    uint8_t xmax = x + sprite.w - 1;
    uint8_t yMin = y < sliceY ? sliceY : y;
    uint8_t yMax = y + sprite.h >= sliceY + sliceHeight ? sliceY + sliceHeight - 1 : y + sprite.h - 1;

    uint8_t  px, py, sx, sy;
    uint16_t color;

    uint16_t row[screenWidth];
    for (py = yMin; py <= yMax; ++py) {
      sy = py - y + sprite.y;
      picRead(row, PIC_SHEET_A, sprite.x, sy, sprite.w);
      for (px = xMin; px <= xmax; ++px) {
        color = row[px - xMin];
        if (color != transColor) {
          buffer[px + (py - sliceY) * screenWidth] = color;
        }
      }
    }
  }
}

void drawScore(uint16_t displayScore, uint8_t sliceY, uint16_t* buffer) {
  // we check first of all that the intersection between
  // the sprite and the current slice is not empty
  if (sliceY < 20 && sliceY + sliceHeight > 11 ) {
    // determines the boundaries of the sprite surface within the current slice
    uint8_t  xMin = 12;
    uint8_t  yMin = (12 < sliceY) ? sliceY : 12;
    uint8_t  yMax = (19 >= sliceY + sliceHeight) ? sliceY + sliceHeight - 1 : 18;
    uint16_t color;
    uint16_t remainder = displayScore;

    // Draw each digit of the score
    for (uint16_t divisor = 1000; divisor > 0; divisor /= 10)     {  // The limit is 4 because 10000 is pow(10, 4)
      uint16_t quotient = (remainder / divisor);
      remainder = (remainder % divisor);
      xMin += 6;
      // Go through the sprite pixels to be drawn
      for (uint8_t py = yMin; py <= yMax; ++py)   {
        uint16_t digit[5];
        picRead(digit, PIC_SHEET_A, 1 + 6 * quotient, py - 11, 5);
        for (uint8_t px = 1; px <= 5; ++px)   {

          // Pick the pixel colour from the spritesheet
          uint16_t colour = digit[px - 1];

          // If it is not the transparency colour
          if (colour != transColor)     {
            // Calculate the colour offset
            size_t bufferIndex = (xMin + px + ((py - sliceY) * screenWidth));
            // Copy the colour code into the rendering buffer
            buffer[bufferIndex] = colour;
          }
        }
      }
    }
  }
}

// -------------------------------------------------------------------------
// Memory transfer to DMA controller
// -------------------------------------------------------------------------

// initiates memory forwarding to the DMA controller....
void customDrawBuffer(uint8_t x, uint8_t y, uint16_t* buffer, uint8_t w, uint8_t h) {
  drawPending = true;
  gb.tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
  SPI.beginTransaction(Gamebuino_Meta::tftSPISettings);
  gb.tft.dataMode();
  gb.tft.sendBuffer(buffer, w * h);
}

// waits for the memory transfer to be completed
// and close the transaction with the DMA controller
void waitForPreviousDraw() {
  if (drawPending) {
    Gamebuino_Meta::wait_for_transfers_done();
    gb.tft.idleMode();
    SPI.endTransaction();
    drawPending = false;
  }
}
