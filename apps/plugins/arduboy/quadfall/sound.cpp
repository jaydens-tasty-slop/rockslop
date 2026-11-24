#include "sound.h"
#include <ArduboyTones.h>
#include <avr/pgmspace.h>

// Quadfall.ino で定義されたグローバルインスタンスを参照
extern Arduboy2 arduboy;

static bool audioEnabled() { return arduboy.audio.enabled(); }

ArduboyTones tones(audioEnabled);

// ---------------------------------------------------------------------------
// 音データ (PROGMEM)
// フォーマット: 周波数(Hz), 継続時間(ms), ..., TONES_END
// ---------------------------------------------------------------------------

// ピースロック: 低い短音
static const uint16_t SND_LOCK[] PROGMEM = {
    180, 55,
    TONES_END
};

// ライン消去（1〜3ライン）: 上昇2音
static const uint16_t SND_LINE[] PROGMEM = {
    600, 80,
    900, 100,
    TONES_END
};

// テトリス（4ライン）: 上昇4音
static const uint16_t SND_TETRIS[] PROGMEM = {
    600, 60,
    750, 60,
    900, 60,
    1100, 120,
    TONES_END
};

// ハードドロップ: 低音一発
static const uint16_t SND_HARDDROP[] PROGMEM = {
    120, 70,
    TONES_END
};

// レベルアップ: 3音ファンファーレ
static const uint16_t SND_LEVELUP[] PROGMEM = {
    523, 100,   // C5
    659, 100,   // E5
    784, 150,   // G5
    TONES_END
};

// ゲームオーバー: 下降3音
static const uint16_t SND_GAMEOVER[] PROGMEM = {
    400, 150,
    280, 150,
    180, 250,
    TONES_END
};

// ---------------------------------------------------------------------------
// 初期化（Arduboy2::begin() の後に呼ぶ）
// ---------------------------------------------------------------------------
void soundInit(Arduboy2 &ab) {
    ab.audio.begin();   // EEPROMからON/OFF設定を読み込む
}

// ---------------------------------------------------------------------------
// ON/OFFトグル（タイトル画面のBボタン）
// ---------------------------------------------------------------------------
void soundToggle(Arduboy2 &ab) {
    ab.audio.toggle();
    ab.audio.saveOnOff();
}

// ---------------------------------------------------------------------------
// 再生関数
// ---------------------------------------------------------------------------
void soundPlay_lock() {
    tones.tones(SND_LOCK);
}

void soundPlay_lineClear(uint8_t lines) {
    if (lines >= 4) {
        tones.tones(SND_TETRIS);
    } else {
        tones.tones(SND_LINE);
    }
}

void soundPlay_hardDrop() {
    tones.tones(SND_HARDDROP);
}

void soundPlay_levelUp() {
    tones.tones(SND_LEVELUP);
}

void soundPlay_gameOver() {
    tones.tones(SND_GAMEOVER);
}
