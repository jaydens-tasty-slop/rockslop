/* Prototypes the Arduino build generates for the sketch's functions. */
void initGame();
void update();
void animShark();
void animHelicopter();
void animFlooded();
void testBoat(uint8_t colonne, uint8_t para_courant);
void animPara();
void drawScreen();
void draw();
void drawSprite(Sprite sprite, uint8_t sliceY, uint16_t* buffer);
void drawText(Sprite sprite, uint8_t sliceY, uint16_t* buffer, uint8_t x, uint8_t y);
void drawScore(uint16_t displayScore, uint8_t sliceY, uint16_t* buffer);
void customDrawBuffer(uint8_t x, uint8_t y, uint16_t* buffer, uint8_t w, uint8_t h);
void waitForPreviousDraw();
