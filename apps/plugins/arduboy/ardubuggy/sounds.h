#ifndef SOUNDS_H
#define SOUNDS_H

void sounds() {
  if (bfire || cfire)sound.tone(100 * (speedFire2), 100);
  if (bombExp || alienExp || rockExp) {
    sound.tone(random(100, 160), random(10, 20));
  }
}

#endif
