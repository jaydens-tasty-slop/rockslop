#include "input.h"
#include "game.h"
#include "sound.h"

// DAS状態
static uint8_t dasLeft  = 0;
static uint8_t dasRight = 0;

// ソフトドロップDAS
static uint8_t dasDown  = 0;

void inputUpdate(Arduboy2 &arduboy) {
    if (gameState == STATE_TITLE) {
        if (arduboy.justPressed(A_BUTTON)) { gameStart(); }
        if (arduboy.justPressed(B_BUTTON)) { soundToggle(arduboy); }
        return;
    }

    if (gameState == STATE_GAMEOVER) {
        if (arduboy.justPressed(A_BUTTON)) {
            gameState = STATE_TITLE;
        }
        return;
    }

    if (gameState == STATE_PAUSED) {
        if ((arduboy.justPressed(A_BUTTON) && arduboy.pressed(B_BUTTON)) ||
            (arduboy.justPressed(B_BUTTON) && arduboy.pressed(A_BUTTON))) {
            gameState = STATE_PLAYING;
        }
        return;
    }

    // --- STATE_PLAYING ---

    // A+B 同時押し → ポーズ
    if ((arduboy.justPressed(A_BUTTON) && arduboy.pressed(B_BUTTON)) ||
        (arduboy.justPressed(B_BUTTON) && arduboy.pressed(A_BUTTON))) {
        gameState = STATE_PAUSED;
        dasLeft = dasRight = dasDown = 0;
        return;
    }

    // 左移動 (DAS)
    if (arduboy.pressed(LEFT_BUTTON)) {
        if (dasLeft == 0) { pieceMove(-1, 0); }
        dasLeft++;
        if (dasLeft >= DAS_DELAY + DAS_INTERVAL) { dasLeft = DAS_DELAY; }
        if (dasLeft >= DAS_DELAY) {
            if ((dasLeft - DAS_DELAY) % DAS_INTERVAL == 0) {
                pieceMove(-1, 0);
            }
        }
    } else {
        dasLeft = 0;
    }

    // 右移動 (DAS)
    if (arduboy.pressed(RIGHT_BUTTON)) {
        if (dasRight == 0) { pieceMove(1, 0); }
        dasRight++;
        if (dasRight >= DAS_DELAY + DAS_INTERVAL) { dasRight = DAS_DELAY; }
        if (dasRight >= DAS_DELAY) {
            if ((dasRight - DAS_DELAY) % DAS_INTERVAL == 0) {
                pieceMove(1, 0);
            }
        }
    } else {
        dasRight = 0;
    }

    // ソフトドロップ (DAS)
    if (arduboy.pressed(DOWN_BUTTON)) {
        if (dasDown == 0) {
            if (pieceMove(0, 1)) score += 1;
        }
        dasDown++;
        if (dasDown >= DAS_DELAY + DAS_INTERVAL) { dasDown = DAS_DELAY; }
        if (dasDown >= DAS_DELAY) {
            if ((dasDown - DAS_DELAY) % DAS_INTERVAL == 0) {
                if (pieceMove(0, 1)) score += 1;
            }
        }
    } else {
        dasDown = 0;
    }

    // 反時計回り
    if (arduboy.justPressed(A_BUTTON)) {
        pieceRotate(1);
    }

    // 時計回り
    if (arduboy.justPressed(B_BUTTON)) {
        pieceRotate(-1);
    }

    // ハードドロップ
    if (arduboy.justPressed(UP_BUTTON)) {
        int8_t gy = ghostY(cur);
        int8_t dropped = gy - cur.y;
        cur.y = gy;
        if (dropped > 0) score += (uint32_t)dropped * 2;
        soundPlay_hardDrop();
        pieceLock();
        dasLeft = dasRight = dasDown = 0;
    }
}
