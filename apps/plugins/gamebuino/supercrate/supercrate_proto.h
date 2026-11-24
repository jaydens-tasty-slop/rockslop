/* Prototypes the Arduino build generates for the sketch's functions. */
int toScreenX(int x);
int toScreenY(int y);
void initGame();
void drawAll();
void loadEEPROM();
void saveEEPROM();
unsigned int EEPROMreadInt(unsigned int i);
void EEPROMwriteInt(unsigned int i, unsigned int value);
void cleanEEPROM();
void printCentered(const char* text);
void printCentered(char* text);
void popup(const char* text, uint8_t duration);
void popup(const char* text);
void updatePopup();
void drawCompass();
