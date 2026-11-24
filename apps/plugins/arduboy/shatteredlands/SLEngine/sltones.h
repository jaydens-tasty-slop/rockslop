/* The original SLTones (a cut-down ArduboyTones on AVR timer 3) is replaced
 * by the layer's ArduboyTones, which has the same tone()/noTone() calls. */
#ifndef SLTONES_H
#define SLTONES_H
#include <ArduboyTones.h>
extern "C" void exit(int status); /* QUIT GAME leaves the plugin */
typedef ArduboyTones SLTones;
#endif
