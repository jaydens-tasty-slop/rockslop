#ifndef RUNSTATE
#define RUNSTATE

#include "BarcaSprite.h"
#include "OctopusSprite.h"
#include "SubSprite.h"
#include "Global.h"

// -------------------------------------------------------------------------
// Run state
// -------------------------------------------------------------------------

class RunState {

  public:

    void update() {
      if (life == 0) {
        gameState = GameState::gameOver;
        if (score > highScore) {
          highScore = score;
        }
      }
      else {
        // -------------------------------------------------------------------------
        // User interaction
        // -------------------------------------------------------------------------

        moveTick++;

        if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
          gameState = GameState::pauseScreen;
          // gb.sound.play("sons\Pause.wav");
          gb.sound.playCancel();
        }

        // -------------------------------------------------------------------------
        // Anim objects
        // -------------------------------------------------------------------------
        if ((gb.buttons.pressed(BUTTON_LEFT)) && (catchGold == false) && (moveSubButton == true) && (hited == false)) {
          if (subMove >= 2) {
            // if sub don't have gold
            --subMove;
            gb.sound.playTick();
          } else if (subGold == true && subMove >= 1) {
            //if sub have gold
            --subMove;
            gb.sound.playTick();
          }

        }
      }

      if ((gb.buttons.pressed(BUTTON_RIGHT)) && (catchGold == false) && (moveSubButton == true) && (hited == false)) {
        if (subMove < 5 && subGold == false) {
          ++subMove;
          gb.sound.playTick();
        } else if (subMove == 5 && subGold == false) {
          catchGold = true;
          subMove = 6;
          gb.sound.playTick();
        } else if (subMove == 5 && subGold == true) {
          catchGold = true;
          subMove = 6;
          gb.sound.playTick();
        } else if (subMove < 5 && subGold == true) {
          ++subMove;
          gb.sound.playTick();
        }
      }


      if (subMove == 0 && subGold == true && inGold == false) {
        inGold = true;
      } else {
        inGold = false;
      }

      if (subMove > 0 && life == 3 && subGold == false) {
        barcaState = 1;
      } else if (subMove == 0 && life == 3 && subGold == true) {
        barcaState = 2;
      } else if (subMove > 0 && life == 2 && subGold == false) {
        barcaState = 4;
      } else if (subMove == 0 && life == 2 && subGold == true) {
        barcaState = 5;
      } else if (subMove > 0 && life == 1 && subGold == false) {
        barcaState = 7;
      } else if (subMove == 0 && life == 1 && subGold == true) {
        barcaState = 8;
      }

      // if ((debug == true) && (subMove > 0) && (oldVal != subMove)) {
      if ((debug == true) && (subMove > 0)) {
        // SerialUSB.print("11111 - Avant --");
        // SerialUSB.print("subMove = ");
        // SerialUSB.println(subMove);
        // SerialUSB.print("diverToShow = ");
        // SerialUSB.println(diverToShow);
        // oldVal = subMove;
        SerialUSB.print("MoveTick :");
        SerialUSB.print(moveTick);
        SerialUSB.print("  -  ");
        SerialUSB.print("MaxTick :");
        SerialUSB.print(maxTick);
        SerialUSB.print("  -  ");
        SerialUSB.print("animFrame :");
        SerialUSB.println(animFrame);
      }
      Octopus();
      Barca();
      if (subMove < 6) {
        Sub();
      } else {
        animSubGold();
      }
      OctopusCollision();
      RunState::draw();

      if (moveTick > maxTick) moveTick = 0;
    }

    template< size_t size >
    void extractDigits(uint8_t (&buffer)[size], uint16_t value) {

      for (uint8_t i = 0; i < size; ++i) {
        buffer[i] = value % 10;
        value /= 10;
      }
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


        // draw divers on barca
        if (showSpriteDiver1)drawSprite(spriteDiver1, sliceY, buffer);
        if (showSpriteDiver1Arm1)drawSprite(spriteDiver1Arm1, sliceY, buffer);
        if (showSpriteDiver1Arm2)drawSprite(spriteDiver1Arm2, sliceY, buffer);
        if (showSpriteDiver2)drawSprite(spriteDiver2, sliceY, buffer);
        if (showSpriteDiver3)drawSprite(spriteDiver3, sliceY, buffer);


        // draw the 3 parts of the diver if needed
        if (diverToShow > 0) drawSprite(diver[diverToShow - 1], sliceY, buffer);
        if (diverArmToShow > 0) drawSprite(diverArm[diverArmToShow - 1], sliceY, buffer);
        if (diverBagToShow > 0) drawSprite(diverBag[diverBagToShow - 1], sliceY, buffer);


        // draw octopus legs
        for (size_t indexLeg = 1; indexLeg < 5; ++indexLeg) {
          if ((octopusLeg[indexLeg] > 0) && (octopusLeg[indexLeg]  < (octopusLegLength[indexLeg] + 1))) {
            for (size_t index = 0; index < (octopusLegLength[indexLeg]); ++index) {
              if (indexLeg == 1) {
                if (showOctopusLeg1[index])drawSprite(spriteOctopusLeg1[index], sliceY, buffer);
              } else if (indexLeg == 2) {
                if (showOctopusLeg2[index])drawSprite(spriteOctopusLeg2[index], sliceY, buffer);
              } else if (indexLeg == 3) {
                if (showOctopusLeg3[index])drawSprite(spriteOctopusLeg3[index], sliceY, buffer);
              } else if (indexLeg == 4) {
                if (showOctopusLeg4[index])drawSprite(spriteOctopusLeg4[index], sliceY, buffer);
              }
            }
          }
        }


        // Draw score
        const uint8_t nbDigits = 6;
        uint8_t digits[nbDigits] = {};
        extractDigits(digits, score);
        for (uint8_t i = 0; i < nbDigits; i++)
          drawDigitOnSlice(digits[nbDigits - i], nbDigits - i - 1, sliceY, buffer);


        // Verify that previous buffer has been sent to the DMA controller
        if (sliceIndex != 0) waitForPreviousDraw();
        // Then send the current buffer
        customDrawBuffer(0, sliceY, buffer, screenWidth, sliceHeight);
      }

      // Wait until the DMA transfer is completed before entering the next cycle
      waitForPreviousDraw();
    }
};

RunState runState;

#endif
