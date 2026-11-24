#pragma once

enum GameState : uint8_t {
	SplashScreen_Init,
	SplashScreen,
	Title_Init,
	Title,
	Game_Init,
	Game,
    Stats_Init,
    Stats
};
    
enum GameMode : uint8_t {
	English,
    French,
};

enum AnimationSequence : uint8_t {
	Think_L,
    Think_R,
    Walk_LR,
    Walk_RL,
};

enum class Direction : uint8_t {
    Up,
    Down,
    Left,
    Right
};

enum class KeyState : uint8_t {
    Visible,
    Invisible,
};

enum class GuessState : uint8_t {
    Dashed,
    Incorrect,
    Correct,
    WrongPosition,
    Duplicate,
};

enum class CheckState : uint8_t {
    Normal,
    InvalidWord,
    CorrectWord,
    RealWord,
    TooManyAttempts,
    Quit,
};

enum KeyboardState : uint8_t {
    Showing,
    StartShowing,
    StartHiding,
    Hiding
};

enum class StatisticsMode : uint8_t {
    PageOne,
    PageTwo,
};


// Extract individual digits of a uint8_t -------------------------------------

template< size_t size > void extractDigits(uint8_t (&buffer)[size], uint8_t value) {

    for(uint8_t i = 0; i < size; ++i) {
        buffer[i] = value % 10;
        value /= 10;
    }

}

// Extract individual digits of a uint16_t
template< size_t size > void extractDigits(uint8_t (&buffer)[size], uint16_t value) {

    for(uint8_t i = 0; i < size; ++i) {
        buffer[i] = value % 10;
        value /= 10;
    }
  
}
/* int arguments (32-bit here, 16-bit on AVR) are ambiguous between the overloads */
template< size_t size > void extractDigits(uint8_t (&buffer)[size], int value) { extractDigits(buffer, (uint16_t)value); }
