#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

// フィールドサイズ
#define FIELD_W 10
#define FIELD_H 16

// 表示設定
#define CELL_SIZE 4
#define FIELD_X   1
#define FIELD_Y   0
#define PANEL_X   46

// DAS設定
#define DAS_DELAY    16
#define DAS_INTERVAL  6

// レベル数
#define MAX_LEVEL 10

// ゲーム状態
enum GameState : uint8_t {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
};

// ピース構造体
struct Piece {
    int8_t  x;
    int8_t  y;
    uint8_t type;
    uint8_t rot;
};

// ピース形状: [type][rot] の16ビットマスク（4×4グリッド、上位ビットが左上）
// ビット15=左上(0,0), ビット14=(1,0), ..., ビット0=(3,3)
// 行優先: row0[col0..3] row1[col0..3] row2[col0..3] row3[col0..3]
extern const uint16_t PIECES[7][4] PROGMEM;

// 落下間隔テーブル (フレーム数, index 0=level1)
extern const uint8_t FALL_INTERVAL[MAX_LEVEL] PROGMEM;

// グローバルゲーム変数
extern uint16_t  field[FIELD_H];
extern Piece     cur;
extern Piece     next;
extern uint32_t  score;
extern uint32_t  highScore;
extern bool      isNewBest;
extern uint8_t   level;
extern uint16_t  linesCleared;
extern GameState gameState;
extern uint8_t   fallTimer;

// 関数プロトタイプ
void gameInit();
void gameStart();
void gameUpdate();

bool pieceCanPlace(const Piece &p);
void pieceLock();
bool pieceRotate(int8_t dir);   // +1=反時計回り, -1=時計回り
bool pieceMove(int8_t dx, int8_t dy);
int8_t ghostY(const Piece &p);

void spawnNext();
