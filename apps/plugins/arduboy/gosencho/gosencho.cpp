#include <Arduino.h>
#include "common.h"

/*  Typedefs  */

typedef struct {
    void(*initFunc)(void);
    MODE_T(*updateFunc)(void);
    void(*drawFunc)(void);
} MODULE_FUNCS;

/*  Local Functions (macros)  */

#define callInitFunc(idx) (moduleTable[idx].initFunc())
#define callUpdateFunc(idx) (moduleTable[idx].updateFunc())
#define callDrawFunc(idx) (moduleTable[idx].drawFunc())

/*  Local Constants  */

PROGMEM static const MODULE_FUNCS moduleTable[] = {
    { initLogo,     updateLogo,     drawLogo },
    { initTitle,    updateTitle,    drawTitle },
    { initGame,     updateGame,     drawGame },
};

/*  Local Variables  */

static MODE_T mode;

/*---------------------------------------------------------------------------*/

void setup()
{
    ab.beginNoLogo();
    ab.setFrameRate(FPS);
    ab.initAudio(1);
    readRecord();
    callInitFunc(mode);
}

void loop()
{
    if (!(ab.nextFrame())) return;
    handleDPad();
    MODE_T nextMode = callUpdateFunc(mode);
    callDrawFunc(mode);
    ab.display();
    if (mode != nextMode) {
        mode = nextMode;
        callInitFunc(mode);
    }
}
