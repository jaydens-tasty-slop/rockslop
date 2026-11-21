#include "renderer.h"
#include "game.h"
#include <avr/pgmspace.h>

// ヘルパー: セルを描画（CELL_SIZE×CELL_SIZEの塗り潰し正方形）
static void drawCell(Arduboy2 &ab, int8_t fx, int8_t fy, uint8_t color) {
    int16_t px = FIELD_X + fx * CELL_SIZE;
    int16_t py = FIELD_Y + fy * CELL_SIZE;
    ab.fillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, color);
}

// ヘルパー: ピース全体を描画
static void drawPiece(Arduboy2 &ab, const Piece &p, int8_t baseX, int8_t baseY, uint8_t color) {
    uint16_t mask = pgm_read_word(&PIECES[p.type][p.rot]);
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (!((mask >> (15 - (row * 4 + col))) & 1)) continue;
            int8_t fx = baseX + col;
            int8_t fy = baseY + row;
            if (fy < 0 || fy >= FIELD_H) continue;
            drawCell(ab, fx, fy, color);
        }
    }
}

// フィールド描画
static void drawField(Arduboy2 &ab) {
    // 枠（外周）
    ab.drawRect(0, 0, FIELD_W * CELL_SIZE + 2, FIELD_H * CELL_SIZE, WHITE);

    // フィールド内セル
    for (uint8_t row = 0; row < FIELD_H; row++) {
        for (uint8_t col = 0; col < FIELD_W; col++) {
            if (field[row] & (1 << (FIELD_W - 1 - col))) {
                drawCell(ab, col, row, WHITE);
            }
        }
    }
}

// ゴースト描画（点線風: XOR描画で白黒反転）
static void drawGhost(Arduboy2 &ab) {
    int8_t gy = ghostY(cur);
    if (gy == cur.y) return; // 既に接地中

    uint16_t mask = pgm_read_word(&PIECES[cur.type][cur.rot]);
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (!((mask >> (15 - (row * 4 + col))) & 1)) continue;
            int8_t fy = gy + row;
            int8_t fx = cur.x + col;
            if (fy < 0 || fy >= FIELD_H) continue;
            int16_t px = FIELD_X + fx * CELL_SIZE;
            int16_t py = FIELD_Y + fy * CELL_SIZE;
            ab.drawRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, WHITE);
        }
    }
}

// サイドパネル描画
static void drawPanel(Arduboy2 &ab) {
    // SCORE
    ab.setCursor(PANEL_X, 0);
    ab.print(F("SCORE"));
    ab.setCursor(PANEL_X, 9);
    // 6桁ゼロ埋め
    char buf[7];
    uint32_t s = score;
    for (int8_t i = 5; i >= 0; i--) {
        buf[i] = '0' + (s % 10);
        s /= 10;
    }
    buf[6] = '\0';
    ab.print(buf);

    // LEVEL
    ab.setCursor(PANEL_X, 24);
    ab.print(F("LEVEL"));
    ab.setCursor(PANEL_X, 33);
    if (level < 10) ab.print('0');
    ab.print(level);

    // NEXT
    ab.setCursor(PANEL_X, 48);
    ab.print(F("NEXT"));

    // ネクストピース描画（小さく）
    uint16_t mask = pgm_read_word(&PIECES[next.type][next.rot]);
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (!((mask >> (15 - (row * 4 + col))) & 1)) continue;
            int16_t px = PANEL_X + col * (CELL_SIZE - 1);
            int16_t py = 56 + row * (CELL_SIZE - 1);
            ab.fillRect(px, py, CELL_SIZE - 2, CELL_SIZE - 2, WHITE);
        }
    }
}

// タイトル画面
static void drawTitle(Arduboy2 &ab) {
    ab.setCursor(20, 4);
    ab.setTextSize(2);
    ab.print(F("QUADFALL"));
    ab.setTextSize(1);
    ab.setCursor(22, 26);
    ab.print(F("BEST:"));
    char buf[7];
    uint32_t h = highScore;
    for (int8_t i = 5; i >= 0; i--) { buf[i] = '0' + (h % 10); h /= 10; }
    buf[6] = '\0';
    ab.print(buf);
    ab.setCursor(22, 39);
    ab.print(F("PRESS A TO START"));
    ab.setCursor(28, 52);
    ab.print(F("[B] SOUND:"));
    ab.print(ab.audio.enabled() ? F("ON") : F("OFF"));
}

// ポーズ画面
static void drawPaused(Arduboy2 &ab) {
    ab.setCursor(28, 22);
    ab.setTextSize(2);
    ab.print(F("PAUSED"));
    ab.setTextSize(1);
    ab.setCursor(16, 46);
    ab.print(F("A+B TO RESUME"));
}

// ゲームオーバー時の右パネル
static void drawGameOverPanel(Arduboy2 &ab) {
    char buf[7];

    // 現在のスコア
    ab.setCursor(PANEL_X, 0);
    ab.print(F("SCORE"));
    ab.setCursor(PANEL_X, 9);
    uint32_t s = score;
    for (int8_t i = 5; i >= 0; i--) { buf[i] = '0' + (s % 10); s /= 10; }
    buf[6] = '\0';
    ab.print(buf);

    // ベストスコア
    ab.setCursor(PANEL_X, 24);
    ab.print(F("BEST"));
    ab.setCursor(PANEL_X, 33);
    uint32_t h = highScore;
    for (int8_t i = 5; i >= 0; i--) { buf[i] = '0' + (h % 10); h /= 10; }
    buf[6] = '\0';
    ab.print(buf);

    // 更新時
    if (isNewBest) {
        ab.setCursor(PANEL_X, 44);
        ab.print(F("NEW!"));
    }
}

// ゲームオーバー画面
static void drawGameOver(Arduboy2 &ab) {
    drawField(ab);
    drawGameOverPanel(ab);

    // オーバーレイ
    ab.fillRect(4, 22, 36, 20, BLACK);
    ab.drawRect(4, 22, 36, 20, WHITE);
    ab.setCursor(7, 26);
    ab.print(F("GAME"));
    ab.setCursor(7, 34);
    ab.print(F("OVER"));
}

// ---------------------------------------------------------------------------
// メイン描画エントリ
// ---------------------------------------------------------------------------
void renderFrame(Arduboy2 &ab) {
    ab.clear();

    switch (gameState) {
        case STATE_TITLE:
            drawTitle(ab);
            break;

        case STATE_PLAYING:
            drawField(ab);
            drawGhost(ab);
            drawPiece(ab, cur, cur.x, cur.y, WHITE);
            drawPanel(ab);
            break;

        case STATE_PAUSED:
            drawPaused(ab);
            break;

        case STATE_GAMEOVER:
            drawGameOver(ab);
            break;
    }

    ab.display();
}
