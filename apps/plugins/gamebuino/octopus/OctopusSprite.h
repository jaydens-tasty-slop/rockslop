#ifndef OCTOPUS_SPRITE
#define OCTOPUS_SPRITE

#include "Global.h"
#include "Sprites.h"

uint8_t octopusLegAnim[4] = {1, 1, 1, 1,};
bool octopusLegReverse[4] = {false, false, false, false,};
const uint8_t octopusLegFrame[4][4] {
  { 30,  60,  90, 300},
  { 25,  50,  75, 100},
  { 50,  90, 300, 300},
  { 10,  35,  55,  85},
};

bool isLegActive(size_t index_leg)
{
  return ((octopusLeg[index_leg] > 0) && (octopusLeg[index_leg]  < (octopusLegLength[index_leg] + 1)));
}

void Octopusleg() {

  for (size_t indexLeg = 1; indexLeg < 5; ++indexLeg) {
    // Active sprites of leg1
    if (isLegActive(indexLeg))
      for (size_t index = 0; index < (octopusLegLength[indexLeg] - 1); ++index) {
        if (indexLeg == 1) showOctopusLeg1[index] = showOctopusLeg1Settings[octopusLegAnim[indexLeg - 1]][index];
        if (indexLeg == 2) showOctopusLeg2[index] = showOctopusLeg2Settings[octopusLegAnim[indexLeg - 1]][index];
        if (indexLeg == 3) showOctopusLeg3[index] = showOctopusLeg3Settings[octopusLegAnim[indexLeg - 1]][index];
        if (indexLeg == 4) showOctopusLeg4[index] = showOctopusLeg4Settings[octopusLegAnim[indexLeg - 1]][index];
      }
  }
}

void Octopus() {

  for (size_t indexLeg = 0; indexLeg < 4; ++indexLeg) {
    if (moveTick == octopusLegFrame[indexLeg][0] || moveTick == octopusLegFrame[indexLeg][1] || moveTick == octopusLegFrame[indexLeg][2] || moveTick == octopusLegFrame[indexLeg][3]) {

      gb.sound.playTick();

      if (octopusLegAnim[indexLeg] < (octopusLegLength[indexLeg + 1] + 1) && octopusLegReverse[indexLeg] == false) {
        ++octopusLegAnim[indexLeg];
      } else if (octopusLegAnim[indexLeg] > 0 && octopusLegReverse[indexLeg] == true) {
        --octopusLegAnim[indexLeg];
      }
      if (octopusLegAnim[indexLeg] == 0) {
        octopusLegReverse[indexLeg] = false;
      } else if (octopusLegAnim[indexLeg] == (octopusLegLength[indexLeg + 1] + 1)) {
        octopusLegReverse[indexLeg] = true;
      }
    }
  }

  Octopusleg();

}


void OctopusCollision() {
  for (size_t indexLeg = 0; indexLeg < 4; ++indexLeg) {
    if ((octopusLegAnim[indexLeg] == (octopusLegLength[indexLeg + 1] + 1)) && (subMove == 2 + indexLeg)) {
      getPoint = 0;
      hited = true;
    }
  }
}

#endif
