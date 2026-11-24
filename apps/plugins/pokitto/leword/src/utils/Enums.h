#pragma once

enum class FadeMode : uint8_t {
    None,
    FadeIn,
    FadeOut,
    Transition
};

enum class SoundSettings : uint8_t {
    Both,
    Music,
    SFX,
    None
};

inline SoundSettings &operator++(SoundSettings &c ) {
    c = static_cast<SoundSettings>( (static_cast<uint8_t>(c) + 1) % 4 );
    return c;
}

inline SoundSettings operator++(SoundSettings &c, int) {
    SoundSettings result = c;
    ++c;
    return result;
}

inline SoundSettings &operator--(SoundSettings &c ) {
    if (static_cast<uint8_t>(c) > 0) {
        c = static_cast<SoundSettings>( (static_cast<uint8_t>(c) - 1) % 4 );
    }
    else {
        c = static_cast<SoundSettings>( 3 );
    }
    return c;
}

inline SoundSettings operator--(SoundSettings &c, int) {
    SoundSettings result = c;
    --c;
    return result;
}

enum GameState : uint8_t {
	SplashScreen_Init,
	SplashScreen,
	Title_Init,
	Title,
	Game_Init,
	Game_Play,
    Stats_Init,
    Stats,
    Pause,
    Pause_Init,
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
    Correct,
    WrongPosition,
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
    InvalidWord_Hanski,
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

enum class TitleScreenMode : uint8_t {
    English,
    French,
    EnglishStats,
    FrenchStats,
    SoundEffects,
};

inline TitleScreenMode &operator++(TitleScreenMode &c ) {
    c = static_cast<TitleScreenMode>( (static_cast<uint8_t>(c) + 1) % 5 );
    return c;
}

inline TitleScreenMode operator++(TitleScreenMode &c, int) {
    TitleScreenMode result = c;
    ++c;
    return result;
}

inline TitleScreenMode &operator--(TitleScreenMode &c ) {
    if (static_cast<uint8_t>(c) > 0) {
        c = static_cast<TitleScreenMode>( (static_cast<uint8_t>(c) - 1) % 5 );
    }
    else {
        c = static_cast<TitleScreenMode>( 4 );
    }
    return c;
}

inline TitleScreenMode operator--(TitleScreenMode &c, int) {
    TitleScreenMode result = c;
    --c;
    return result;
}