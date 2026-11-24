# Gamebuino META layer for Rockbox

Runs Gamebuino META (Arduino C++) games as ordinary Rockbox plugins: one
compatibility layer (`lib/`) linked into one `.rock` per game
(`gamebuino.make`, one directory per game, like `arduboy/`). Output is
`rocks/games/gamebuino/<game>.rock`.

## Layer (`lib/`)
Own implementation of the public API of
https://github.com/Gamebuino/Gamebuino-Meta (LGPL-3.0), no code copied.
Only what the ported games use: `gb.begin/update`, `gb.display` (colour and
indexed modes, text with a 3x5 font, primitives, `drawImage` with scaling,
flipping, sub-rects), `Image` (frames, RGB565 / index / pal4 data), `gb.buttons`,
`gb.sound.fx` and streamed WAV music, `gb.save`, GMV animations, drawable
`Image` surfaces, `gb.gui.popup`, a no-op `gb.lights`, an Arduino-like
`String` and a small heap in the free part of the plugin arena.
- `gb_glue.c`: the only file that includes `plugin.h`: entry, start/pause
  menu, keys, view, save file, mixer output.
- `gb.cpp`: the API. `Image` objects are constant-initialised and decode
  their header lazily, so no static constructor depends on the layer being
  set up.
- `gbclassic.h`: Classic Gamebuino (84x48) names for the Classic sketches:
  `LCDWIDTH`, `BTN_*`, `byte`, `EEPROM` on the save file, `malloc`/`free` on
  the layer's heap.
- `tools/img2pal4.py`: rewrites RGB565 image arrays (<= 16 colours) as 4-bit
  palette images (the layer's colour mode 2); halves the data size.

## Display
The game screen is a plain buffer owned by the game (`gb_game.fb`).
`GB_VIEW_FIT` scales it nearest-neighbour to 128 px wide (80x64 -> 128x102,
a 1.6x scale), centred; `GB_VIEW_ROT90` turns a 160x128 game into 128x160 and
the D-pad is remapped (hold the player turned clockwise).

## Keys
SELECT = A, SELECT+DOWN = B, SELECT+LEFT = MENU, arrows as is; a game can
supply its own rule table (`struct gb_rule`). SELECT is also the power key,
so SYS_POWEROFF is dropped in play; the SELECT+UP chord held 0.5 s opens the
Rockbox pause menu (Resume, Settings, Help, About, Playback Control, Quit).
Every game opens on the same menu with Play instead of Resume.

## Save data
`gb.save` lives in RAM and is written to
`/.rockbox/rocks/games/gamebuino/<game>.sav` 3 s after a change, at the menu
and on exit. Sound on/off is in `<game>.cfg`.

## Games
- `metawarfare/`: Meta Warfare, sentinel87, MIT,
  https://github.com/sentinel87/MetaWarfare (b58d512). Sketch files
  unchanged except: `mw_proto.h` (the prototypes the Arduino IDE generates)
  is included before `setup()`, two functions got the `return` they lack, and
  `assets/sprites.h` was converted with `tools/img2pal4.py`. Scaled 1.6x.
- `glitch/`: gLiTcH, Pierre Jarriges, MIT, https://github.com/Codnpix/Glitch_META
  (HEAD at port time). The eight 240x128 backgrounds are in `decor.dat`
  (made by `tools/mkdecor.py`, card file) and loaded one at a time
  (`graphicAssets.h`, `glitch.cpp` has the descriptor).

- `parachute/`: Parachute (Game & Watch), Jicehel, MIT (c) Stephane
  Calderoni, https://github.com/jicehel/Parachute_Meta (8b04301). Streams
  160x8 slices (portrait via `GB_VIEW_ROT90`, hold the player sideways).
  Changes: `parachute_proto.h` (generated prototypes), the four 160x128
  pictures are read from `parachute.dat` on the card (`pictures.h`,
  `lib/gb_pics.h`, `tools/mkpics16.py`) instead of linked in; the three WAV clips
  stream from the card (a 4 KB ring in `parachute.cpp`). The
  layer gained `playTick/playOK/playCancel`.
- `octopus/`: Octopus (Game & Watch), Jean-Charles Lebeau, MIT (c) Stephane
  Calderoni, https://github.com/jicehel/Octopus-for-META (d03b879). Same
  HD slice technique and picture file as Parachute (`octopus.dat`, three
  pictures); `DIGIT_COLORMAP` kept in `pictures.h`. No WAV clips upstream; beeps only.
- `gbsokoban/` (named so to not clash with Rockbox's own sokoban): Sokoban,
  Jean-Charles Lebeau, MIT, https://github.com/jicehel/Sokoban (4b8ed41).
  `Sokoban.ino` first, then the others (as the Arduino IDE); one prototype in
  `gbsokoban_proto.h`; WAV clips (`Sound/*.wav`) stream from the card, tones via the layer's
  `Sound::tone`. French in-game help text unchanged. The layer gained
  `Display::printf(x, y, fmt, ...)`, `Sound::tone`.
- `gbminesweeper/` (named so to not clash with Rockbox's own minesweeper):
  Minesweeper, Steph (Stephane Calderoni), MIT,
  https://github.com/m1cr0lab-gamebuino/Minesweeper (bb33d52). 160x128
  indexed mode, portrait via `GB_VIEW_ROT90`. Only change: a stand-in `sin`
  for the LED effects (which the layer ignores). The layer gained
  `Display::drawRoundRect`, `snprintf`, `float_t`, `Lights::_buffer`.
- `gbpong/`: PongMETA, Rodot, LGPL-3.0, https://github.com/Rodot/PongMETA
  (1e0d4bf, 2018). Sketch unchanged. The right paddle (A/B in the original)
  stays with the computer. The layer gained `collideRectRect`,
  `pickRandomSeed`, `map`, `Display::setColor(c, bg)` (bg ignored) and
  `Lights::fillRect/drawPixel(x, y)` no-ops.
- `sweetvalentine/`: Sweet Valentine (inverted breakout), Stephane Calderoni,
  MIT, https://github.com/m1cr0lab-gamebuino/gb-sweet-valentine (dcd758a).
  HD slices, portrait via `GB_VIEW_ROT90`; the 160x128 background is read
  from `sweetvalentine.dat` (`Background.cpp`, `tools/mkpics16.py`). Three UB
  fixes that worked by luck on the SAMD21: an unspecified-order
  `a[n] = f(n++)` in `SplashScreen.cpp`, a null ball written to before it
  exists and null paddle/ball read after game over in `GameEngine.cpp`. The
  layer gained `setFrameRate`, `atan2`, `__cxa_pure_virtual`, and `min`/`max`
  now return by value (they returned a reference to a parameter, so were
  not constant expressions); a game's own `snprintf` was dropped for the
  layer's.
- `apollo/`: Apollo (lunar lander), Steph (Stephane Calderoni), MIT,
  https://github.com/m1cr0lab-gamebuino/apollo (79c71d0). 160x128 indexed,
  scaled to fit. SELECT+LEFT/RIGHT = thrust + side thruster; the pot controls
  are stubbed out (`yp3_arduino.h`), the UP/DOWN toggle to them removed; the
  game's `Sound` class renamed `ApolloSound`. Layer gained sin/cos/sqrt/PI,
  `print(x, y, ...)`, float `print`, `getTimePerFrame`.
- `gruniozerca/`: Gruniozerca Meta, Dark Archon (code), Neko (art), Unlicense,
  https://github.com/arhneu/gruniozerca-meta (17b3538). The six big pictures
  are in `gruniozerca.dat` (`tools/mkpics16.py`) and loaded at start; title,
  game over and credits share one buffer, reloaded on change. Music/SFX
  (`sfx/*.wav`, music by Ozzed, CC BY-SA 3.0, halved to 22.05 kHz) stream
  from the card. The layer gained `Sound::isPlaying`, `changeGame` (quit),
  `setCursorY`. WAV clips use the mixer's beep channel and mix with Rockbox
  playback (only `gb_big_buffer` users stop it), which Help now says.
- `uforace/`, `metarace/`: UFO Race (Rodot, https://github.com/Rodot/UFO-Race-META
  716de16) and MetaRace (Sorunome, https://github.com/Sorunome/MetaRace
  9f38843), both LGPL-3.0, sketches unchanged except `0b` for Arduino `B0000`
  binary literals in `world.ino` and the generated prototypes. Gas is SELECT
  held (A) while LEFT/RIGHT steer, SELECT+DOWN brakes (B). The layer gained
  `Gui::menu`/`keyboard` (list menu, name entry), the Classic-style
  `gb.menu/keyboard/popup/setFont`, `fontWidth/Height`, public `cursorX/Y`
  and `textWrap` on `Display` (renamed from cx/cy/wrap), `NAMELENGTH`,
  `NUM_HIGHSCORE`.
- `supercrate/`: Super Crate Box, Aurelien Rodot, LGPL-3.0,
  https://github.com/Rodot/SuperCrateMETA (1a38ca4). Classic-API sketch:
  `Gamebuino-EEPROM.h` is a shim over the save file; `__FlashStringHelper`
  prototypes replaced by `const char *`; `B0000` literals -> `0b`; one
  `return 0` added to `Enemy::update` (fell off the end of an int function,
  which crashed). UP is jump (B), SELECT fires (A). The layer gained flipped
  `drawBitmap`, `getBitmapPixel`, `NOROT/FLIPH`, `F()`, Tft/Lights no-ops.
- `waternet/`: Waternet pipe puzzle, Willems Davy (joyrider3774), MIT,
  https://github.com/joyrider3774/waternet_gamebuino_meta (a279023). Indexed
  160x128, held sideways. Changes: Windows `src\\x.h` includes fixed,
  `getRandomSeed` from `millis()` (no `micros`/`analogRead`), `sprintf` ->
  `snprintf`, `randomSeedGame` declared `uint32_t` like its definition.
  SELECT is A, SELECT+DOWN is B (back/pause). The layer gained
  `Sound::play(const uint16_t *track, loop)`: Gamebuino note tracks played as
  square tones, one step per frame (sfx tones interrupt a note), and `PGM_P`.
- `gbjezzball/`: JezzBall, Thomas Pietrzak, GPL-3.0,
  https://github.com/ArduboyCollection/gamebuino-jezzball (d51e949). Gamebuino Classic (84x48) sketch over `lib/gbclassic.h`. Changes:
  logo title screen replaced by a text one (`jb_title`), `INVERT` -> `BLACK`,
  battery and font lines dropped, `floor(log10())` and `gb.collidePointRect`
  replaced by integer helpers in `jezzball.cpp`. SELECT is A, SELECT+DOWN is
  B (switch wall direction).
- `gbbangbang/`: Bang! Bang!, Thomas Pietrzak, GPL-3.0,
  https://github.com/ArduboyCollection/gamebuino-bangbang (cfef988). Classic
  sketch over `lib/gbclassic.h`; title screen calls dropped, colours inverted
  for the black LCD. Arrows aim, SELECT (A) fires.
- `gbsolitaire/`: Solitaire (Klondike), Andy O'Neill, MIT,
  https://github.com/ArduboyCollection/gamebuino-solitaire (80f3f76). Classic
  sketch over `lib/gbclassic.h`; piles use fixed storage instead of
  allocating in static constructors; tick/OK sounds stand in for the beeps.
  SELECT is A, SELECT+DOWN is B (game menu).
