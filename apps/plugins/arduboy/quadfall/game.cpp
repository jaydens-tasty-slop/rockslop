#include "game.h"
#include "sound.h"
#include <string.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// ピース形状データ (PROGMEM)
// 各エントリは4×4グリッドの16ビットマスク
// ビット15が(col=0,row=0)、ビット0が(col=3,row=3)
// ---------------------------------------------------------------------------
const uint16_t PIECES[7][4] PROGMEM = {
    // I
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    // O
    { 0x6600, 0x6600, 0x6600, 0x6600 },
    // T
    { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
    // S
    { 0x06C0, 0x4620, 0x06C0, 0x4620 },
    // Z
    { 0x0C60, 0x2640, 0x0C60, 0x2640 },
    // J
    { 0x08E0, 0x6440, 0x0E20, 0x44C0 },
    // L
    { 0x02E0, 0x4460, 0x0E80, 0xC440 },
};

// 落下間隔 (level 1〜10)
const uint8_t FALL_INTERVAL[MAX_LEVEL] PROGMEM = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6
};

// スコアテーブル（消去ライン数 0〜4 に対応）
static const uint16_t SCORE_TABLE[5] PROGMEM = {0, 100, 300, 500, 800};

// フィールド1行が全て埋まったときのビットマスク（RAM 節約のため #define）
#define FULL_LINE ((uint16_t)((1u << FIELD_W) - 1))
#define EEPROM_MAGIC        0x4F
#define EEPROM_HIGHSCORE_ADDR (EEPROM_STORAGE_SPACE_START + 1)

// ---------------------------------------------------------------------------
// グローバル変数
// ---------------------------------------------------------------------------
uint16_t  field[FIELD_H];
Piece     cur;
Piece     next;
uint32_t  score;
uint32_t  highScore;
bool      isNewBest;
uint8_t   level;
uint16_t  linesCleared;
GameState gameState;
uint8_t   fallTimer;

// ---------------------------------------------------------------------------
// ヘルパー: ピースマスクの指定セルが立っているか
// ---------------------------------------------------------------------------
static inline bool maskBit(uint16_t mask, uint8_t col, uint8_t row) {
    return (mask >> (15 - (row * 4 + col))) & 1;
}

// ---------------------------------------------------------------------------
// 衝突判定
// ---------------------------------------------------------------------------
bool pieceCanPlace(const Piece &p) {
    uint16_t mask = pgm_read_word(&PIECES[p.type][p.rot]);
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (!maskBit(mask, col, row)) continue;
            int8_t fx = p.x + col;
            int8_t fy = p.y + row;
            if (fx < 0 || fx >= FIELD_W) return false;
            if (fy >= FIELD_H) return false;
            if (fy < 0) continue; // フィールド上部は許可
            if (field[fy] & (1u << (FIELD_W - 1 - fx))) return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// ゴーストY座標
// ---------------------------------------------------------------------------
int8_t ghostY(const Piece &p) {
    Piece g = p;
    while (g.y < FIELD_H) {
        g.y++;
        if (!pieceCanPlace(g)) { g.y--; break; }
    }
    return g.y;
}

// ---------------------------------------------------------------------------
// 移動
// ---------------------------------------------------------------------------
bool pieceMove(int8_t dx, int8_t dy) {
    Piece tmp = cur;
    tmp.x += dx;
    tmp.y += dy;
    if (!pieceCanPlace(tmp)) return false;
    cur = tmp;
    return true;
}

// ---------------------------------------------------------------------------
// 回転 (+1=右, -1=左)
// ---------------------------------------------------------------------------
bool pieceRotate(int8_t dir) {
    Piece tmp = cur;
    tmp.rot = (tmp.rot + 4 + dir) % 4;

    // SRS簡易キック: (0,0), (-1,0), (+1,0), (0,-1)
    static const int8_t kicks[4][2] = {{0,0},{-1,0},{1,0},{0,-1}};
    for (uint8_t i = 0; i < 4; i++) {
        Piece k = tmp;
        k.x += kicks[i][0];
        k.y += kicks[i][1];
        if (pieceCanPlace(k)) { cur = k; return true; }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ライン消去
// ---------------------------------------------------------------------------
static uint8_t clearLines() {
    uint8_t cleared = 0;
    for (int8_t row = FIELD_H - 1; row >= 0; row--) {
        if (field[row] == FULL_LINE) {
            // このラインを消去し、上のラインを下にシフト
            for (int8_t r = row; r > 0; r--) {
                field[r] = field[r - 1];
            }
            field[0] = 0;
            cleared++;
            row++; // 同じ行を再チェック
        }
    }
    if (cleared > 0) {
        uint8_t oldLevel = level;
        // cleared は最大4（テトロミノは最大4ブロックのため同時消去は最大4ライン）。
        // そのため SCORE_TABLE[cleared] の範囲外アクセスは仕様上発生しない。
        score += (uint32_t)pgm_read_word(&SCORE_TABLE[cleared]) * level;
        linesCleared += cleared;
        if (linesCleared >= (uint16_t)level * 10 && level < MAX_LEVEL) {
            level++;
        }
        if (level > oldLevel) {
            soundPlay_levelUp();
        } else {
            soundPlay_lineClear(cleared);
        }
    }
    return cleared;
}

// ---------------------------------------------------------------------------
// ロック
// ---------------------------------------------------------------------------
void pieceLock() {
    uint16_t mask = pgm_read_word(&PIECES[cur.type][cur.rot]);
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (!maskBit(mask, col, row)) continue;
            int8_t fy = cur.y + row;
            int8_t fx = cur.x + col;
            if (fy >= 0 && fy < FIELD_H && fx >= 0 && fx < FIELD_W) {
                field[fy] |= (1u << (FIELD_W - 1 - fx));
            }
        }
    }
    uint8_t cleared = clearLines();
    if (cleared == 0) soundPlay_lock();
    spawnNext();
}

// ---------------------------------------------------------------------------
// 次ピースをスポーン
// ---------------------------------------------------------------------------
// 乱数状態（線形合同法）。gameStart() で millis() ベースのシードを設定する。
static uint16_t rng = 1;
static uint8_t bag[7];
static uint8_t bagIdx = 7; // 7 = 空（初回で fillBag() を呼ばせる）

static void fillBag() {
    for (uint8_t i = 0; i < 7; i++) bag[i] = i;
    for (uint8_t i = 6; i > 0; i--) {
        rng = rng * 25173 + 13849;
        uint8_t j = rng % (i + 1);
        uint8_t tmp = bag[i]; bag[i] = bag[j]; bag[j] = tmp;
    }
    bagIdx = 0;
}

static uint8_t nextType() {
    if (bagIdx >= 7) fillBag();
    return bag[bagIdx++];
}

void spawnNext() {
    cur = next;
    cur.x = (FIELD_W - 4) / 2;
    cur.y = -1;
    next.type = nextType();
    next.rot  = 0;
    next.x    = 0;
    next.y    = 0;
    fallTimer = 0;
    if (!pieceCanPlace(cur)) {
        gameState = STATE_GAMEOVER;
        if (score > highScore) {
            highScore = score;
            isNewBest = true;
            EEPROM.write(EEPROM_STORAGE_SPACE_START, EEPROM_MAGIC);
            EEPROM.put(EEPROM_HIGHSCORE_ADDR, highScore);
            soundPlay_levelUp();
        } else {
            isNewBest = false;
            soundPlay_gameOver();
        }
    }
}

// ---------------------------------------------------------------------------
// ゲーム初期化
// ---------------------------------------------------------------------------
void gameInit() {
    gameState = STATE_TITLE;
    if (EEPROM.read(EEPROM_STORAGE_SPACE_START) == EEPROM_MAGIC) {
        EEPROM.get(EEPROM_HIGHSCORE_ADDR, highScore);
    } else {
        highScore = 0;
    }
    isNewBest = false;
}

void gameStart() {
    memset(field, 0, sizeof(field));
    score        = 0;
    level        = 1;
    linesCleared = 0;
    fallTimer    = 0;
    isNewBest    = false;
    rng          = (uint16_t)millis() ^ 0xA5A5;
    bagIdx       = 7; // 新しいシードで fillBag() させる
    next.type    = nextType();
    next.rot     = 0;
    next.x       = 0;
    next.y       = 0;
    gameState    = STATE_PLAYING;
    spawnNext();
}

// ---------------------------------------------------------------------------
// ゲーム更新（落下のみ、入力はinput.cppから呼ばれる）
// ---------------------------------------------------------------------------
void gameUpdate() {
    if (gameState != STATE_PLAYING) return;
    fallTimer++;
    uint8_t interval = pgm_read_byte(&FALL_INTERVAL[level - 1]);
    if (fallTimer >= interval) {
        fallTimer = 0;
        if (!pieceMove(0, 1)) {
            pieceLock();
        }
    }
}
