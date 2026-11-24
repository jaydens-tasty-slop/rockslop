#include <Gamebuino-Meta.h>
#include "constants.h"
#include "Application.h"

void setup() {
    // initialization of serial communication
    // SerialUSB.begin(115200);
    // Gamebuino initialization 
    gb.begin();
    gb.setFrameRate(FRAME_RATE);
    // RAM buffer initialization with zero dimensions
    gb.display.init(0, 0, ColorMode::rgb565);
    // game engine initialization
    Application::init();
}

void loop() {
    while(!gb.update());

    // transfer the control flow to the game engine
    Application::tick();

    // sends performance indicators to the serial monitor 
    // if (!(gb.frameCount % FRAME_RATE)) {
    //     SerialUSB.printf("CPU: %i, RAM: %i\n", gb.getCpuLoad(), gb.getFreeRam());
    // }
}