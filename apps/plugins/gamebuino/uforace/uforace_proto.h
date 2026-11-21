/* Prototypes the Arduino build generates for the sketch's functions. */
void initHighscore();
void saveHighscore(unsigned int score);
void drawHighScores();
boolean drawNewHighscore(unsigned int score);
void drawMenu();
void initPlayer();
void updatePlayer();
void drawPlayer();
void initTime();
void updateTime();
void drawTime();
void initGame();
void play();
byte getTile(byte x, byte y);
void drawWorld();
void drawMap();
