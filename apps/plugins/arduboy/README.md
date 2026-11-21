# Arduboy layer for Rockbox

Runs Arduboy (Arduino C++) games as ordinary Rockbox plugins: one compatibility
library (`lib/`) linked into one `.rock` per game (`arduboy.make`, one
directory per game, like `puzzles`). Rocks land in `rocks/games/`. Games keep
their upstream sources as far as possible; each directory has the upstream
LICENSE and a README with URL and commit. `CARDFILES` lists the licence texts
and the FX data image that go on the SD card (`yp3dev/rocks.sh`).

## Library (`lib/`)
- `ArdVoice.h`: stand-in for ArdVoice (8 kHz PCM through the AVR speaker timer); `playVoice` and friends are no-ops, there is no PCM path.
- `ArdBitmap.h`: ArdBitmap 2.0.3 (full version incl. drawCompressed), Ignacio Vina (github.com/igvina/ArdBitmap), Apache-2.0 (licence in the header). Unmodified, header only; the game defines `ARDBITMAP_SBUF` (the screen buffer) before including it.
- `Arduboy2.*`, `Sprites*`, `Arduboy2Audio.*`, `Arduboy2Data.cpp`: the real
  Arduboy2 library (MLXXXp/Arduboy2, BSD-3-Clause). Changes: AVR asm replaced
  by C, no boot logo, `nextFrame()` paced against an accumulated deadline (the
  Rockbox tick is 10 ms, so a per-frame wait would drift).
- `Arduboy2Core.*`: the OLED/AVR hardware layer, replaced by calls into the glue.
- `ArduboyFX.*`: the Arduboy FX library (Mr.Blinky, CC0) over a data file: the
  flash chip's SPI data register is emulated (`ab_fx_read`, 32 x 512 byte page
  cache taken from the plugin buffer), so the library's C++ code (bitmaps,
  frames, text, indexed reads) runs unchanged. FX data is big endian.
  `uint24_t` is 32 bits here (3 on AVR): `sizeof` on FX structs must be patched
  in the game (see the FX games). Game state is saved to `arduboy/<game>.sav`.
- `ArduboyTones.*`: same API (MLXXXp/ArduboyTones, MIT), a square-wave
  synthesiser on the mixer BEEP channel, so it mixes with music. A second
  source (`ab_tunes_fill`) is used by Playtune scores (Ardulem) and FX tone
  streams. The start-menu Sound setting is the master switch.
- `ArduboyV1.h` (`Arduboy.h`, `Arglib.h`): compatibility shim for games written against the old Arduboy v1 library or Team a.r.g.'s Arglib fork: an `Arduboy` class on Arduboy2 (`start`, `clearDisplay`, `drawSprite`, `tunes.tone/playScore`...), `SimpleButtons`, and `Sprites(arduboy)`. The game's own copy of the old library is not compiled.
- `ab_new.cpp`: opt-in operator new/delete (small static pool, per-size free lists) for the few sketches that allocate objects (ab_cxx.cpp's no-op deletes are weak); `Tinyfont.*` (Botond Kis, BSD-3-Clause), `Print.*`, `Arduino.h`,
  `avr/pgmspace.h` (PROGMEM is plain memory), `EEPROM.h`, `ab_libc.c`,
  `ab_cxx.cpp` (the few libc/C++ runtime symbols a plugin lacks).
- `ab_glue.c`: the only file that includes `plugin.h`: plugin entry (`setup()`
  once, `loop()` forever; quitting is `exit()`), display, keys, menus, EEPROM
  file (`arduboy/<game>.eep`, 1 KiB, flushed 3 s after a change, at the menu
  and on exit), FX file, audio.

## Display
The 128x64 buffer is converted page by page to RGB565 and blitted 1:1,
centred vertically in 128x160 (no extra framebuffer). Colour scheme (white,
green, amber, ice blue, pocket LCD) is a start-menu setting. The area below
shows a one-line key hint. A game that fakes grey by alternating dither
patterns (`ab_game.blend`, Ard-Drivin) gets real grey: pixels are drawn from
the last two frames.

## Keys and menus
A game supplies `struct ab_game` (`<game>_rb.cpp`): name, title, help and
credits text, a key rule table, the menu gesture. A rule maps physical keys to
Arduboy buttons; first match wins and consumes its keys:
`{AB_P_SELECT | AB_P_DOWN, AB_B_B}`. Besides the five keys there are
`AB_P_LONG` (SELECT held `long_ms`, default 400) and `AB_P_TAP` (SELECT
released before `long_ms`; lasts 60 ms). Default mapping: SELECT = A,
SELECT+DOWN = B, directions as is; a lone SELECT waits 30 ms when SELECT chords
exist so a chord is not seen as A first.
Hardware facts that shape the tables: SELECT is also the power key (about 1 s
powers off), Up/Down share a ladder and Left/Right share a pin, so those pairs
cannot be pressed together; any other pair, and SELECT with anything, can.
The only menu gesture is SELECT+UP held 0.5 s (1 s where that chord is also a
gameplay combo); while it is held a 500 ms chord presses nothing in the game.
Every game opens on a Rockbox start menu (Play/Resume, Settings, Help, About,
Playback Control, Quit); the gesture returns to it. Time spent there does not
count for `millis()`. About shows the game's and the layer's licences.

## Adding a game
Copy the sketch into `arduboy/<game>/` (a `.ino` becomes `.cpp` with
`#include <Arduino.h>` and the prototypes the IDE would generate), write
`<game>_rb.cpp`, add the sources to `arduboy.make`, the rock to `CATEGORIES`,
licence/data to `CARDFILES`.

## Games
- `catacombs/` Catacombs of the Damned, James Howard, MIT.
- `ardugolf/` ArduGolf, Tiberius Brown, MPL-2.0 (via apfxtech/FlipperGolf).
- `ardulem/` Ardulem, Alban Nanty, MIT (its bundled Arduboy v1 library is kept
  except the hardware/audio files, which are Rockbox replacements).
- `ardrivin/` Ard-Drivin, Remi Veilleux and Louis-Philippe Guilbert, MIT.
- `shroom/` Shroom Knight, Onebit Productions & Filmote, GPL-3.0.
- `kong2/` Kong II, Press Play On Tape et al. (github.com/Press-Play-On-Tape/Kong-II @ 8ba0b35), BSD-3-Clause. Own ArduboyNewTones replaced by the layer's ArduboyTones; image-table reads use pgm_read_ptr.
- `kong/` Kong (Game & Watch Donkey Kong remake), Press Play On Tape et al. (github.com/Press-Play-On-Tape/Kong @ ec95141), BSD-3-Clause. ArduboyTonesExt is a wrapper over ArduboyTones; drawScenery made void (missing return).
- `hellocmd/` Hello, Commander, Felipe Manga (github.com/felipemanga/HelloCommander @ f540d0a), MIT. AVR ADC seeding removed.
- `darkunder/` Dark & Under, Press Play On Tape/Filmote (github.com/Press-Play-On-Tape/Dark-And-Under @ 7dd8f6a), BSD-3-Clause. The .ino files are concatenated into DarkUnder.cpp.
- `castleboy/` CastleBoy, dir3kt (github.com/dir3kt/CastleBoy @ 1d758fd), MIT. freeRam() stubbed.
- `choplifter/` Choplifter, Filmote/Press Play On Tape (github.com/Press-Play-On-Tape/Choplifter @ 2fcd339), BSD-3-Clause. Own mirrored-bitmap reader renamed (ODR clash with lib).
- `mysticballoon/` Mystic Balloon, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-34-Mystic-Balloon @ 58743b6), MIT, code and art (c) TEAM a.r.g. Function-pointer table read with pgm_read_ptr; `B01010101` constants rewritten as `0b...`. SELECT = B, SELECT+DOWN = A. Sound: ArduboyTones effects.
- `samegame/` SameGame and the other OBONO games (github.com/obono/ArduboyWorks @ d4b1f04), MIT. MyArduboy runs on Arduboy2 + lib/ArduboyPlaytune (new). Keys: Select = B (confirm), Select+Down = A (menu).
- `knightmove/` Knight Move, OBONO (ArduboyWorks @ d4b1f04), MIT. Same MyArduboy/Playtune base as samegame; sin() replaced by an integer table. Keys: Select = B, Select+Down = A.
- `quarto/` Quarto!, OBONO (ArduboyWorks @ d4b1f04), MIT. Needs `strlen_PF`/`uint_farptr_t` (lib/avr/pgmspace.h). Keys as samegame.
- `psicolo/` Psi Cola (PSICOLO), OBONO (ArduboyWorks @ d4b1f04), MIT. Fixed a non-void function with no return (crashed on ARM and x86). Keys as samegame.
- `lasers/` Lasers, OBONO (ArduboyWorks @ d4b1f04), MIT. AVR-asm screen fill replaced by a C loop; game starts with a flicker warning. Keys as samegame.
- `hollow/` Hollow Seeker, OBONO (ArduboyWorks @ d4b1f04), MIT. Float trig via lib/ab_math.h. Keys as samegame.
- `hopper/` Hopper, OBONO (ArduboyWorks @ d4b1f04), MIT. Float trig/sqrt via lib/ab_math.h. Keys as samegame.
- `chiemagari/` Chiemagari, OBONO (ArduboyWorks @ d4b1f04), MIT. Keys as samegame.
- `obreversi/` Reversi (OBONO) (ArduboyWorks @ d4b1f04), MIT. Dir named obreversi: `reversi` is a core Rockbox plugin. Keys as samegame.
- `chribocchi/` Chribocchi, OBONO (ArduboyWorks @ d4b1f04), MIT. A = pause (Select+Down). Keys as samegame.
- `n43/` Nineteen43, Simon Holmes/Filmote (github.com/filmote/Nineteen43), MIT; uses lib/FixedPoints (Pharap, Apache-2.0). Tap Select = A (fire), hold Select 0.4 s = B (roll). Tones wired (ArduboyTones).
- `n44/` Nineteen44: Return to Midway, Simon Holmes/Filmote (github.com/filmote/Nineteen44), MIT; uses lib/FixedPoints. Sketch (.ino x5) concatenated with prototypes; tap/hold keys as n43. Tones wired.
- `evasion/` Evasion, OBONO (ArduboyWorks @ d4b1f04), MIT. The Arduboy2 variant of MyArduboy (`MyArduboy2`): its AVR-timer audio engine is replaced by `lib/MyArduboy2Audio.inc` over lib/ArduboyPlaytune (score with pitch offset + one tone voice; priorities kept, duty cycle ignored). Keys as samegame.
- `ardubullets/` ARDUBULLETs, `gosencho/` I want 5 quadrillion yen!, `toyokumono/` Toyokumono, `rysk/` RYSK: OBONO (ArduboyWorks @ d4b1f04), MIT, MyArduboy2 variant like evasion. Keys as samegame; gosencho's AVR-layout cheat code left as is.
- `bananonsense/` Bananonsense, OBONO (ArduboyWorks @ d4b1f04), MIT. The .incbin data becomes assembler data in generated bindata.cpp; the 8-bit PWM banana sample plays through ArduboyPlaytune::playWave. Keys as samegame.
- `morse/` Morse Code Trainer, OBONO (ArduboyWorks @ d4b1f04), MIT. Select = B (straight key), Left/Right = paddle, Select+Down = A. USB HID keyboard output dropped (FakeKeyboard is a no-op); tone via MyArduboy2Audio.inc (stopTone).
- `undertower/` Under the Tower, wuuff (github.com/wuuff/under-the-tower-arduboy @ b510e66), GPL-3.0 (art: part of the same repository, same licence). Sketch concatenated with prototypes; Arduino `Bxxxxxxxx` constants converted. No sound upstream. Select = A, Select+Down = B (menu outside battles).
- `shatteredlands/` Shattered Lands: Towers of Perdition and `shatteredlands2/` Sea of Despair (the latter reuses shatteredlands/SLEngine with its own data.h), tuxinator2009 (github.com/tuxinator2009/ShatteredLands @ 2b67b47), GPL-3.0-or-later (code and art). SLTones replaced by ArduboyTones (music wired); exitToBootloader/UPLOAD removed (QUIT GAME leaves the plugin, game over/story end go back to the title); NULL-chunk read at map edges guarded; part two starts a new game as HERO when no save exists (SL_FRESH_START). Keys: Select = B, Select+Down = A.
- `trollyfish/` Trolly Fish, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-36-Trolly-Fish @ ddaf606), MIT, code and art (c) TEAM a.r.g. Designated Rect initialisers turned into constructor calls. Sound: ArduboyTones effects.
- `blobattack/` Blob Attack, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-28-Blob-Attack @ 71d33c8), MIT, code and art (c) TEAM a.r.g. Sound: ArduboyTones effects.
- `shadowrunner/` Shadow Runner, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-15-Shadow-Runner @ 25f2775), MIT, code and art (c) TEAM a.r.g. Type-less functions made void (falling off the end crashed); Rect initialisers as constructors. Sound: ArduboyTones effects.
- `sirene/` Sirene, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-42-Sirene @ 2b96c1d), MIT, code and art (c) TEAM a.r.g. Type-less/return-less `checkEndWave` made void (fell off the end). Sound: ArduboyTones effects.
- `rmh/` Reverse Mermaid Hockey, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-31-RMH @ 377a314), MIT, code and art (c) TEAM a.r.g.  Sound: ArduboyTones effects.
- `dicefate/` DICE of FATE, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-37-DICE-of-FATE @ d2d7cf4), MIT, code and art (c) TEAM a.r.g.  Sound: ArduboyTones effects.
- `begemmed/` Begemmed, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-30-BEGEMMED @ 7dcb1be), MIT, code and art (c) TEAM a.r.g.  Sound: ArduboyTones effects.
- `virus/` Virus LQP-79, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-40-VIRUS-LQP-79 @ 9f664f0), MIT, code and art (c) TEAM a.r.g. Sound: ArduboyTones effects. SELECT = B, SELECT+DOWN = A.
- `bubblepop/` Bubble PoP, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-14-Bubble-PoP @ 17bc3fe), MIT, code and art (c) TEAM a.r.g. Unused ATMlib object dropped; sin/cos shim (no libm). Sound: ArduboyTones effects.
- `fantasyrampage/` Fantasy Rampage, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-48-Fantasy-Rampage @ 29d30d3), MIT, code and art (c) TEAM a.r.g. ATMlib music (ATMlib.cpp/.h are shared, `../atmlib/`).
- `sunfire/` Sunfire, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-50-Sunfire @ e99fff7), MIT, code and art (c) TEAM a.r.g. ATMlib music (shared, `../atmlib/`). SELECT = B, SELECT+DOWN = A.
- `outpostdefense/` Outpost Defense, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-49-Outpost-Defense @ 9297095), MIT, code and art (c) TEAM a.r.g. Unused ATMlib object dropped. No sound upstream.
- `helmetshordes/` Helmets & Hordes, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-47-Helmets-Hordes @ 87d1b7e), MIT, code and art (c) TEAM a.r.g. Type-less `goldBarSetInLine` made void, `Rect` initialisers as constructors; no sound upstream.
- `pinball/` Pinball, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-44-Pinball @ 62ec62d), MIT, code and art (c) TEAM a.r.g. libm calls go to `lib/ab_math.h` (floatmath.h adds pow/round). No sound upstream.
- `ohmummy/` Oh Mummy, Matthew Vicaradge (github.com/Mattvic79/OhMummy @ a8f155a), BSD-3-Clause. Single sketch with ArduboyTones music and effects. Tap SELECT = B, hold SELECT = A.
- `rooftop/` Rooftop Rescue, Bert van't Veer (github.com/BertVeer/Rooftop @ cb8e920), MIT. Single sketch with ArduboyTones effects; libm calls go to `lib/ab_math.h` (floatmath.h adds fabs/round). SELECT = A, SELECT+DOWN = B.
- `towerofpain/` Tower Of Pain, Fran J. Alvarez (github.com/franalvarez21/TowerOfPain @ 3b34f9e), BSD-3-Clause. String-table reads use pgm_read_ptr. Sound (ArduboyTones effects) starts off as upstream; toggle in the pause menu. SELECT = A, SELECT+DOWN = B.
- `nightraid/` Night Raid, Evan Barger (github.com/epbarger/night-raid @ 36e7fa0), MIT. Prototypes added for the .ino, `String(x)` print dropped. No sound upstream. Tap SELECT = A, hold SELECT = B.
- `starflight/` Star Flight, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-26-Star-Flight @ e6a4486), MIT, code and art (c) TEAM a.r.g. Uses the Arglib shim (ArduboyV1.h); falling-off-the-end `checkEndWave` made void. No sound upstream.
- `teenytank/` TeenyTank, Jez Swanson (github.com/Jezzamonn/TeenyTank @ 9e83e83), MIT. Uses the Arduboy v1 shim (ArduboyV1.h); sin/cos via lib/ab_math.h, round local, prototypes added, Serial dropped. Tones for the startup tune and shots. Tap SELECT = B, hold SELECT = A.
- `arduman/` Ardu-man, Seth A. Robinson (github.com/SethRobinson/arduman_arduboylib11 @ 3c7fb4c), MIT. Arduboy lib 1.1 game on the v1 shim; its Playtune adapter class renamed (the layer has ArduboyPlaytune), no memset from a global constructor (the loader crashed). Playtune music and tones.
- `galaxion/` Galaxion, tako2 (github.com/tako2/galaxion @ 8cfbafd), MIT. Arduboy v1 shim, links lib/ab_new.cpp (the game allocates), Playtune start jingle and tone effects.
- `chickenx/` Chicken Cross, David Martinez, Arduboy2/ArduboyTones port by Scott Allen (github.com/dragula96/ChickenX @ b60c7dc), LGPL 2.1+ per source header (GPL 2 file in repo). Sketch prototypes added, title bitmap tab as titlebmp.cpp. Tone effects.
- `crateconfusion/` Crate Confusion, phoboslab (github.com/phoboslab/arduboy-games @ de5e858), MIT (LICENSE names OBONO). The layer's BeepPin1 is silent, so play_sound uses ArduboyTones. Only A is used.

- `evade/` Evade, Modus Create (github.com/ModusCreateOrg/evade-arduboy-game @ 0933a8f), MIT, v1 Arduboy.h via the shim, Playtune music
- `ppblackjack/` Blackjack, Press Play On Tape (github.com/Press-Play-On-Tape/Blackjack @ 72a6b1b), Apache-2.0. Dir named ppblackjack: `blackjack` is a core Rockbox plugin. AVR timer speech/bytebeat disabled; the splash beep uses ArduboyTones. Default keys.
- `arduwars/` Arduwars, Botond Kis (github.com/yinkou/Arduwars @ 9cd6ce8), BSD-3-Clause (SimpleArduinoUtilities: Apache-2.0); hot-seat multiplayer only (single player is an upstream stub); uses lib/ab_new.cpp
- `dungeons2/` Dungeons 2, TEAM a.r.g. ID-21 (github.com/TEAMarg/Dungeons2 @ d79ab43), MIT; unfinished upstream prototype, no sound; uses lib/ab_new.cpp
- `poopanic/` Poop Panic!, inajob (github.com/ArduboyCollection/poop-panic @ c0c44a8), MIT
- `flappyball/` FlappyBall, Chris Martinez / Scott Allen (github.com/ArduboyCollection/FlappyBall @ 22253a6), BSD-3-Clause (license text from the source header)
- `farkle/` Farkle, Press Play On Tape (github.com/Press-Play-On-Tape/Farkle), Apache-2.0. AVR timer splash beep via ArduboyTones; tone: splash only. Default keys.
- `cribbage/` Cribbage, Press Play On Tape (github.com/Press-Play-On-Tape/Cribbage), BSD-3-Clause. Uses the layer's new fixed-buffer String (lib/WString.h). Default keys.
- `germanwhist/` German Whist, Press Play On Tape (github.com/Press-Play-On-Tape/GermanWhist), BSD-3-Clause. Default keys.
- `pppicross/` PiCross, Press Play On Tape (github.com/Press-Play-On-Tape/PiCross), BSD-3-Clause. Dir named pppicross: `picross` is a core Rockbox plugin. Default keys.
- `minirogue/` Mini Rogue, Press Play On Tape (github.com/Press-Play-On-Tape/MiniRogue), BSD-3-Clause. Default keys.
- `turtlebridge/` Turtle Bridge, Press Play On Tape (github.com/Press-Play-On-Tape/TurtleBridge), BSD-3-Clause. ArduboyTonesExt via lib/ArduboyTonesExt.h (tones wired). Default keys.
- `firepanic/` Fire Panic, Press Play On Tape (github.com/Press-Play-On-Tape/FirePanic), BSD-3-Clause. ArduboyTonesExt via lib/ArduboyTonesExt.h (tones wired). Default keys.
- `oilpanic/` Oil Panic, Press Play On Tape (github.com/Press-Play-On-Tape/OilPanic), BSD-3-Clause. Default keys.
- `logix/` Logix, Filmote / Press Play On Tape (github.com/Press-Play-On-Tape/Logix), BSD-3-Clause. Default keys.
- `ardubuggy/` Ardu Buggy, a Moon Patrol style game (github.com/ArduboyCollection/ArduBuggy @ 65ddafa), Unlicense
- `pongab/` Pong, Taylor Hansen (github.com/ArduboyCollection/Pong-Arduboy @ 840b42a), MIT
- `liteout/` Lite Out (github.com/ArduboyCollection/ArduboyLiteOut @ 75a4e63), Unlicense
- `tres/` Tres, Casey Gold (github.com/ArduboyCollection/tres @ 8befbc9), MIT
- `trenchrun/` Trench Run, Lucas Salton Cardinali (github.com/lscardinali/TrenchRun-Arduboy @ 2012f98), MIT
- `hangman/` Hangman, serisman (github.com/serisman/arduboy-hangman @ da1825f), MIT; word list from github.com/Xethron/Hangman
- `dominoes/` Dominoes All Fives, Press Play On Tape (github.com/Press-Play-On-Tape/Dominoes), BSD-3-Clause. Default keys.
- `junofirst/` Juno First, Press Play On Tape (github.com/Press-Play-On-Tape/JunoFirst), BSD-3-Clause. Default keys.
- `spacecab/` Space Cab, Press Play On Tape (github.com/Press-Play-On-Tape/SpaceCab), BSD-3-Clause. Hold Select = thrust (A), Select+Down = landing gear (B).
- `boris/` Boris Goes Skiing, clone of Horace goes skiing (github.com/ArduboyCollection/boris-goes-skiing @ 75d7a11), MIT (TEAM a.r.g. 2017)
- `lion/` Lion Tamer, Press Play On Tape (github.com/Press-Play-On-Tape/Lion), BSD-3-Clause. NewTones roar sweep approximated by a growl of low tones. Default keys.
- `buttonstrail/` Buttons Trail, Press Play On Tape (github.com/Press-Play-On-Tape/ButtonsTrail), BSD-3-Clause. Default keys.
- `roadtrip/` Road Trip, Press Play On Tape (github.com/Press-Play-On-Tape/RoadTrip), BSD-3-Clause. Default keys.
- `stellarimpact/` Stellar Impact, Athene Allen (github.com/ArduboyCollection/Stellar_Impact @ e56419c), MIT, v1 Arduboy.h via the shim
- `arduroad/` ARDUROAD, Filmote / Press Play On Tape (github.com/Press-Play-On-Tape/ARDUROAD), BSD-3-Clause. Radio music (Left/Right on title) wired; SPI screen paint replaced by paintScreen. Default keys.
- `loverush/` LoveRush, Stephane Cote (github.com/vampirics/LoveRush @ 993a2db), code MIT, graphics CC BY-NC-SA 4.0
- `helii/` Helii, Stephen Eisenhauer (github.com/ArduboyCollection/Helii-Arduboy @ f6f9a8f), MIT, v1 Arduboy.h via the shim
- `ardulo/` ArduLO, Jon Thysell (github.com/jonthysell/ArduLO @ 34d3b87), MIT
- `layingpipe/` Laying Pipe, Press Play On Tape (github.com/Press-Play-On-Tape/LayingPipe), BSD-3-Clause. Sound: ArduboyTones beeps. Default keys.
- `glove/` Glove, fuopy (github.com/ArduboyCollection/glove), MIT, v1 Arduboy.h via the shim
- `omegachase/` Omega Chase, Karl P. Williams (github.com/Karl-Williams/OmegaChase @ 64bc7d3), MIT
- `humanityrevenge/` Humanity Revenge DC, giangregorio (github.com/ArduboyCollection/Humanity_Revenge_DC @ 9730bf5), MIT
- `cosmicpods/` CosmicPods, cubic9com (github.com/ArduboyCollection/CosmicPods @ 0923919), BSD-3-Clause
- `cyberhack/` Cyberhack, Press Play On Tape (github.com/Press-Play-On-Tape/Cyberhack), BSD-3-Clause. ByteBeat music/sfx (timer-4 PWM on AVR) rendered by a game-local ab_tunes_fill at 7812.5 Hz; title bootloader hint replaced. Default keys.
- `randocity/` Randocity, Paul Wasson (github.com/pmwasson/Randocity @ 42e9b7e), MIT
- `leword/` LeWord (Arduboy FX), Press Play On Tape (github.com/Press-Play-On-Tape/LeWord), BSD-3-Clause. Needs fxdata.bin on the card (English/French word lists). No sound upstream. Default keys.
- `beamemup/` Beam 'Em Up, Ben Combee (github.com/unwiredben/arduboy-beamemup @ 95a7fb6), Apache-2.0
- `ravine/` Ravine Despoiler, Ben Combee (github.com/unwiredben/arduboy-ravine-despoiler @ c48915c), Apache-2.0
- `ponghauki/` PongHauKi, databhor (github.com/databhor/PongHauKi @ bcbc762), MIT
- `trialsofastarok/` The Trials of Astarok, Press Play On Tape (github.com/Press-Play-On-Tape/TrialsOfAstarok), BSD-3-Clause. Playtune music wired (ArduboyPlaytune from lib). Default keys.
- `ardecipher/` ARDecipher, databhor (github.com/databhor/ARDecipher @ 2b2255a), MIT
- `crates3d/` Crates 3D, Brian Smith / fuopy (github.com/ArduboyCollection/Crates3D @ 7573547), MIT, v1 Arduboy.h via the shim
- `floodfill/` Flood Fill, Gary Franz (github.com/garyfranz/arduboy_floodfill @ 471503d), MIT
- `snakecdr/` Snake, CDR_Xavier (github.com/CDRXavier/SNAKE @ 501d6a4), GPL-2.0-or-later, v1 Arduboy.h via the shim (named snakecdr: Rockbox has its own snake plugin)
- `backtojungle/` Back to the Jungle, Innovation Garage (github.com/ArduboyCollection/ArduboyBackToTheJungle), BSD-3-Clause. Playtune music and tones wired. Dialogs advance with B (Select+Down). Uses lib/PGMWrap.h.
- `ringpuzzle/` Ring Puzzle, Skuratovich Sergey (github.com/ArduboyCollection/ring_puzzle_game), MIT. No sound upstream. A = move, B = show (Select+Down).
- `sfcave/` SFCave, Stephanie Maks / Rodney Norton (github.com/ArduboyCollection/SFCave @ 3cbb554), GPL-3.0
- `waternetab/` Waternet (Arduboy port), Willems Davy / joyrider3774 (github.com/joyrider3774/waternet_arduboy), MIT. Dir named waternetab because a Gamebuino Waternet exists. Music and sfx via ArduboyTones (audible). A = confirm/rotate, B (Select+Down) = back.
- `apara/` APara, Leonhard Vogt (github.com/ArduboyCollection/APara @ b963159), GPL-3.0, v1 Arduboy.h via the shim
- `diamonds/` Diamonds (breakout), Redbug / kyuranGames (github.com/ArduboyCollection/diamonds), MIT. Removed: the title tune (a chiptune of the "Diamonds Are Forever" film theme, no grant), so the title is silent; bounce tones kept. Default keys.
- `asteroidsx/` Asteroids, CDR_Xavier (github.com/CDRXavier/ASTEROID @ 68dedff), GPL-2.0-or-later; ADC battery readout stubbed, debug overlay compiled out
- `ardusweeper/` Ardusweeper, jbellue (github.com/ArduboyCollection/minesweeper @ c228957), GPL-3.0, v1 Arduboy.h via the shim
- `blockspush/` Blocks, William Woody (github.com/ArduboyCollection/blocks @ 26b650e), GPL-3.0-or-later, v1 Arduboy.h via the shim
- `blockstacker/` Block Stacker, David Martinez / DMG (github.com/ArduboyCollection/BlockStacker), LGPL-2.1-or-later per source header (repo LICENSE is the BSD text of the Arduboy v1 library it bundled, not shipped). Via lib/ArduboyV1.h; tone blips. Loop paced with delay(4). A (Select) = drop.
- `quadrastic/` Quadrastic, David Martinez / DarKaoz (github.com/ArduboyCollection/Quadrastic @ 9d36cc5), LGPL 2.1+ per header (GPL-3 text in repo)
- `keykatit/` KeyKat IT, Robert Lowe (github.com/ArduboyCollection/keykat-it @ 2ddac6f), GPL-3.0-or-later
- `microtank/` MicroTank, Thomas Hartmann (github.com/hartmann1301/MicroTank @ 1baad6c), Apache-2.0
- `starhonor/` Star Honor, Wenceslao Villanueva Jr (github.com/wvillanueva/StarHonor), MIT. Uses lib/ArduboyV1.h instead of its renamed copy of the Arduboy library. No sound. A = Select, B = Select+Down.
- `joustish/` Joustish, wuuff (github.com/wuuff/joustish @ 18d3988), GPL-3.0
- `spacewads/` Space Wads, Cuddles04 (github.com/Cuddles04/Space-Wads @ d4b12a2), Apache-2.0
- `littlebeluga/` Little Beluga, Lincoln Hach Yellick (github.com/lyellick/LittleBeluga @ 5bbaa08), MIT
- `spacefighter/` Space Fighter, Maicon Hieronymus (github.com/ArduboyCollection/SpaceFighter), GPL-3.0-or-later. Select = fire (B), the game's own A pause is dropped (use the Rockbox pause menu). No sound.
- `jumpingjack/` Jumping Jack, marggines (github.com/marggines/JumpingJack @ d9ba382), MIT
- `streetracer/` Street Racer, marggines (github.com/marggines/Street-Racer @ 84e86e6), MIT
- `newblocks/` New Blocks on the Kid, bateske (github.com/bateske/NewBlocksOnTheKid @ 133fb6a), GPL-3.0
- `tunnelfighter/` Tunnel Fighter, Matthew Bryan (github.com/TropicAnthropic/tunnelFighter @ eec0991), GPL-3.0
- `tilesfv/` Tiles, Frank van de Ven (github.com/FrankVanDeVen/Tiles @ 7dfa3c4), MIT
- `gohome/` Go Home, Frank van de Ven (github.com/FrankVanDeVen/GoHome @ 4fda4c8), MIT
- `montana/` Montana, Frank van de Ven (github.com/FrankVanDeVen/Montana @ 35810a1), MIT
- `calculate/` Calculate, Frank van de Ven (github.com/FrankVanDeVen/Calculate @ cda5dc7), MIT
- `slidexl/` Slide-XL, Frank van de Ven (github.com/FrankVanDeVen/Slide-XL @ 0b0b160), MIT
- `tinydash/` Tiny Dash, Frank van de Ven (github.com/FrankVanDeVen/Tiny-Dash @ 30131ce), MIT
- `arduskiing/` ArduSkiing, Frank van de Ven (github.com/FrankVanDeVen/ArduSkiing @ 1ee2239), MIT
- `lasermaze/` Laser Maze, Frank van de Ven (github.com/FrankVanDeVen/LaserMaze @ 46615d2), MIT
- `arduel/` Arduel, curlyc (github.com/curlyc/arduel @ 1f770d4), MIT
- `mouseescape/` Mouse Escape, Zachariah Falgout (github.com/RetrobitCoder/MouseEscape @ 058e19c), MIT. Removed: the title banner and lose screen by LENZ (no licence); plain text replaces them.
- `militaryaviator/` Military Aviator, curlyc (github.com/curlyc/militaryaviator @ 303b417), MIT
- `candlelight/` Candlelight, Matthew Bryan (github.com/TropicAnthropic/Candlelight @ 9a48b56), GPL-3.0
- `bladerunner/` Blade Runner, Popov Ilia (github.com/NoobGeek-Ilia/BladeRunner @ cf6db4e), Apache-2.0
- `hunt/` HUNT!, Volodymyr Kadzhaia (github.com/ladroid/HUNT @ 03b27f8), BSD-3-Clause
- `minotaur/` Minotaur, Maxime Hanicotte (github.com/ideometre/Minotaur @ 28a5715), Unlicense
- `batgame/` BAT-GAME, Quickdrawkid (github.com/Quickdrawkid/BAT-GAME @ 19789af), MIT
- `calldungeon/` Call of Dungeon, tako2 (github.com/tako2/dungeon @ 473dce3), MIT
- `breakouttv/` Breakout, Vlad Tomoiaga (github.com/tvlad1234/breakout @ a5c1e49), GPL-3.0
- `breakouthiro/` Breakout, Hiroshi Tamura (github.com/HiroshiTamura/arduboy_breakout_game @ 5c1ebe8), MIT
- `tunnelterror/` Tunnel Terror, Tom and Jamie Millichamp (github.com/Tomsk666/arduboy-terror-tunnels @ 01d5bb9), GPL-3.0
- `tiny2048/` Tiny 2048, akkera102 (github.com/akkera102/03_tiny_2048 @ 4ea9c6b), GPL-2.0
- `tinyasteroids/` Tiny Asteroids, akkera102 (github.com/akkera102/06_tiny_asteroids @ b8baea4), GPL-2.0
- `tinynstower/` Tiny NS-Tower, akkera102 (github.com/akkera102/04_tiny_ns_tower @ 0ee5827), GPL-2.0
- `tinyinvader/` Tiny Digital Invader, akkera102 (github.com/akkera102/10_tiny_invader @ 1abefdb), GPL-2.0
- `tinydepth/` Tiny Depth, akkera102 (github.com/akkera102/02_tiny_depth @ 7cc8c8e), GPL-2.0; its unlicensed MIDI background tune is left out
- `easterpanic/` Easter Panic, Pharap (github.com/Pharap/EasterPanic @ 4a7b724), Apache-2.0
- `ardubloxx/` ArduBloxx, Popov Ilia (github.com/NoobGeek-Ilia/ARDUBLOXX @ eec69d9), Apache-2.0
- `magic8ball/` Magic 8 Ball, HugoQuadrado (github.com/HugoQuadrado82/Arduboy8ball @ 0ac1459), MIT
ArduSnake, Alec Tutin (github.com/chootin/ardusnake @ cb74b97), GPL-3
Unicorn Dash, Kirill Korolkov (github.com/KirillKorolkov/unicorn-dash @ c134b4c), MIT
Hippodrome, Miguel Riaza Valverde (github.com/MiguelRiazaValverde/Hippodrome @ 10db095), GPL-3
Ardu N-Back, Ruben Berenguel Montoro (github.com/rberenguel/ardu-n-back @ 029f518), MIT
Tronduboy, Simon Prickett (github.com/simonprickett/tronduboy @ 66312d8), MIT
Shooter, Sandor Kovacs (github.com/shantheone/shooter @ cb40c26), MIT
Ardutan, yamasakihiromi (github.com/yamasakihiromi/ardutan @ e475f36), MIT
Quadfall, stonedev-app (github.com/stonedev-app/Quadfall @ 1f81cb7), MIT
LATE, Vladimir Borisov (github.com/core1024/LATE @ 643440b), MIT
TechnoMonkey, Zachariah Falgout (github.com/RetrobitCoder/TechnoMonkey @ 266686b), MIT
Flappy Halloween, Andrew Low (github.com/andrewlow/flappyhalloween @ 94f926c), EPL-1.0
Battle for Zuthsrah, Mre64 (github.com/Mre64/BattleForZuthsrah @ 4433a4e), MIT
Row Farm, Glitsch3n (github.com/Glitsch3n/arduboy-game-collection @ b1ffab7), MIT
Snack, Shane Powell (github.com/shane-powell/ArduboyGames @ 23ad3fe), MIT
Space Shooter, Shane Powell (github.com/shane-powell/ArduboyGames @ 23ad3fe, space), MIT
Tamaguino, Alojz Jakob, Arduboy port by Keyboard_Camper (github.com/KeyboardCamper/Tamaguino-AB @ bd4b513), GPL-3
JumpBoy, takaiwa / t-iwasaki (github.com/t-iwasaki/arduboy-JumpBoy @ 1b2dcc9), MIT
JumpBoy 2, takaiwa / t-iwasaki (github.com/t-iwasaki/arduboy-JumpBoy2 @ 09a10fc), MIT
Bone Shakers, James Howard (github.com/jhhoward/BoneShakers @ ab61ef8), GPL-3
Epic Crates of Mass Destruction, TEAM a.r.g. (github.com/Team-ARG-Museum/ID-33-ECOMD @ c2dac94), MIT, code and art (c) TEAM a.r.g.
Pico Racers, TEAM a.r.g.: emuty, PixelArtM, JO3RI (github.com/Team-ARG-Museum/ID-43-Pico-Racers @ ff088cd), MIT, code and art (c) TEAM a.r.g.
Coin Rush, ArdFlamingo (github.com/Pharap/ArdFlamingo-Arduboy-Games @ 5de05a7), MIT
City Life, ArdFlamingo (github.com/filmote/Arduboy-City-Life @ 5620fef), MIT; Font4x6 by Filmote, BSD-3
Cooties Attack, The Programming Raccoon (github.com/Pharap/Cooties-Attack @ 2978d2f), MIT; Font3x5 by Filmote, BSD-3
MicroFlight, James Howard (github.com/jhhoward/MicroFlight @ 7061957), MIT
DDRBOY, Tyler Nijmeh (github.com/tytydraco/DDRBOY @ 1b0b98e), BSD-2-Clause
1010 for Arduboy, joshua stein (github.com/jcs/arduboy-1010 @ 2519bab), ISC
Arena Fight, FreezingSnail (github.com/FreezingSnail/fight @ 9f3a5a2), MIT
A-Maze, Alojz Jakob, Arduboy port by Keyboard_Camper (github.com/alojzjakob/A-Maze @ 37e2305), GPL-3
Any Kind of Car and Any Kind of Truck, Tommy Marshall (github.com/rocktronica/any-kind-of-car-and-any-kind-of-truck @ 911c662), MIT
Ultra Hop, rolohaun (github.com/rolohaun/ultra-hop @ 19c4801), MIT
Athletic Boy, Frank van de Ven (github.com/FrankVanDeVen/AthleticBoy @ 7025f04), MIT
Egg Master Pro 2020, Bert van't Veer (github.com/BertVeer/EggMasterPro2020 @ 4e6dccc), MIT
The Social Distance Game, Marcus Sanatan (github.com/msanatan/TheSocialDistanceGame @ 025e866), MIT
The Bounce, Joshimuz (github.com/ArduboyCollection/TheBounceArduboy @ 2fcc65f), MIT (copyright holder left blank in the licence text)
Nielk, Greyson Potter (github.com/gkpotter/Nielk @ d577698), MIT
Ardu Dash, Sergei Petrov (github.com/SuperSolik/arduboy-games @ 523b23a), MIT
Guess Dots, Sergei Petrov (github.com/SuperSolik/arduboy-games @ 523b23a), MIT
Triangle Wave, SergioFLS (github.com/SergioFLS/TriangleWave @ 11f4ac1), MIT
ArduGo, Modus Create (Don Anderson) (github.com/ModusCreateOrg/ardugo @ 2343b90), MIT
Particles' Segregation, MariEss, 64 hour game jam entry (github.com/MariEss/GJM_Particles-_Segregation @ 8c9e0b8), MIT (no holder named in the licence)
You Won't Survive, Andre Hellwig (github.com/ArduboyCollection/YouWontSurvive @ c6f489e), MIT (source header)
POITTO, @ina_ani (github.com/ArduboyCollection/poitto @ 8e0f8b9), MIT
MicroTD, Miloslav Ciz (drummyfish) (gitlab.com/drummyfish/Arduboy_TD @ 0c8958f), CC0
i4arduboy, Planet Marimo (github.com/ArduboyCollection/i4arduboy @ 60e156f), MIT
Space Quarth, Schuemi (github.com/ArduboyCollection/SpaceQuarth @ e76e561), MIT
Armageddon, wuuff (github.com/ArduboyCollection/armageddon @ c9c471d), GPL-3
Bang! Bang!, Thomas Pietrzak (github.com/ArduboyCollection/gamebuino-bangbang @ cfef988), GPL-3
JezzBall, Thomas Pietrzak (github.com/ArduboyCollection/gamebuino-jezzball @ d51e949), GPL-3
Solitaire (Gamebuino), Andy O'Neill (github.com/ArduboyCollection/gamebuino-solitaire @ 80f3f76), MIT
Super Crate Buino, Aurélien Rodot (Arduboy2 port by Dan O'Shea) (github.com/uXeBoy/Super-Crate-Buino @ 6441161), LGPL-3+
The Quest for Truth, Guillaume Elias (github.com/GuillaumeElias/TheQuestForTruth @ 961a215), LGPL-3
Ardu-Whack, zeduckmaster (framagit.org/zeduckmaster/ardu-whack @ d01b9c9), GPL-3
MicroCity, James Howard (github.com/jhhoward/MicroCity @ ae265f1), GPL-3
Tiny Dungeon, Sven B (Lorandil) (github.com/Lorandil/TinyDungeon @ 1f0f60e), MIT
Cursed Mansion 1300, ImMrShrike (github.com/ImMrShrike/Cursed-Mansion-1300 @ 9089962), AGPL-3
Pocket Othello, Wang Renxin (github.com/ArduboyCollection/pocket_othello @ 5a03e05), CC BY-SA 4.0 as stated in its README
RUND.INO (offline dinosaur runner), Flaki (github.com/ArduboyCollection/arduboy-rund-ino @ ff8615f), ISC as declared in its package.json
RUND.INO Halloween edition, Flaki (github.com/ArduboyCollection/arduboy-rund-ino @ ff8615f, halloweend), ISC as declared in its package.json
Defenduino, Alex McChessers (bitbucket.org/alexmcchessers/defenduino, v1.2 sources), BSD-3-Clause. Uses lib/ArdBitmap.h; Sel = fire, Sel+Down = smart bomb, hold Sel = hyperspace.
Pyoro, Thesola10 (github.com/Thesola10/Pyoro), GPL-3.0. Uses lib/ArdBitmap.h; Sel = A (tongue). The upstream Up+B hi-score reset is not reachable.
Harambe's Revenge, Molly Carpenter and Danielle Lapis (github.com/ArduboyCollection/Harambe-s-Revenge), zlib. Select = B (menus, dash). The optional ArdVoice dash sample is compiled out (the game falls back to tones); local _crc_ibutton_update.
Kung Fu Escape, MrSazazel (github.com/MrSazazel/Kung_Fu_Escape), MIT. Uses lib/ArdBitmap.h (drawCompressed). Tap Select = B (punch), hold Select 0.4 s = A (kick), with Down held the crouching variants. The title jingle is below ArduboyTones' 250 Hz limit, so it is silent as on a real Arduboy.
Roshambo, CDRXavier (github.com/CDRXavier/Roshambo), GPL-2.0. Uses lib/WString.h. Up/Down = choose, Select = A. Answering N to "Play again?" clears the console and starts over instead of exiting.
Santa's Happy Little Packer, Erwin Ried (github.com/ArduboyCollection/ArduboyXmasGame), BSD-3-Clause. Uses lib/ArdVoice.h (new stand-in: the "ho ho ho" voice sample is dropped), lib/ArduboyPlaytune. Select = A, Select+Down = B, hold Select+Down 1 s = A+B (restart level).
Creapawn, Revlis (github.com/Revlis64/Creapawn @ bdc5bd6), Apache-2.0. Uses lib/ArdBitmap.h; header file names case-fixed for Linux; Sel = B, Sel+Down = A.
Cannon Father, jaychugstein (github.com/jaychugstein/Cannon-Father @ b7ea9d3), MIT. Arduboy FX; data file cannonfather/fxdata.bin. Sel = B (go), Sel+Down = A (glide).
Escape From Earth, jaychugstein (github.com/jaychugstein/Escape-From-Earth @ 62af70c), MIT. Arduboy FX; data file escapefromearth/fxdata.bin. Hold Sel = propel (B), Sel+Down = spin (A).
Pocket Rocket, jaychugstein (github.com/jaychugstein/Pocket-Rocket @ 4c1a62d), MIT. Arduboy FX; data file pocketrocket/fxdata.bin (1.5 MB). Hold Sel = thrust (A), Sel+Down = parachute (B).
2D Mini Golf, David (Bennett) Ballentine (github.com/BennettB123/Arduboy-2DMiniGolf @ 2c0a731), MIT. Arduboy FX; fxdata-data.bin built from src/FX/fxdata.txt with fxdata-build.py. Sel = B, Sel+Down = A. Adds String::indexOf/substring to lib/WString.h.
Billy Graham's Bible Blaster, jaychugstein (github.com/jaychugstein/Billy-Grahams-Bible-Blaster @ 5afa3c5), MIT. Music via lib/ArduboyPlaytune. Sel = B (throw), Sel+Down = A (reload).
Tire Tester, jaychugstein (github.com/jaychugstein/Tire-Tester @ fbffb8f), MIT. Arduboy FX; data file tiretester/fxdata.bin. Sel = B (next/rev), Sel+Down = A (back).
The Fall, Onebit Productions (github.com/TropicAnthropic/The-Fall), GPL-3.0. Arduboy FX; data file thefall/fxdata.bin; ArduboyG (greyscale) replaced by lib/ArduboyG.h dither shim, SpritesU by a C++ rewrite. Sel = A.
Rhombus, Onebit Productions (github.com/TropicAnthropic/RhombusArduboyGame), GPL-3.0. Arduboy FX; data file rhombus/fxdata.bin; uses lib/ArduboyG.h shim. Directions hop, Sel = A.
KUBE, Onebit Productions (github.com/TropicAnthropic/KUBE), GPL-3.0. No FX data; uses lib/ArduboyG.h shim. Directions move, Sel = jump.
Mastermind, Frank Van De Ven (github.com/FrankVanDeVen/Mastermind @7d23b74), MIT. No sound in original; ino converted to cpp (prototypes, start-screen waits delay). Up/Down pawn, Left/Right number, Sel+Down = guess, Sel = new game.
Peg Solitaire, Frank Van De Ven (github.com/FrankVanDeVen/PegSolitaire @ad2b257), MIT. No sound in original; ino converted to cpp (prototypes, busy waits delay). Directions move, Sel+Down = pick/jump, Sel = undo.
Frogger, Simon Holmes (github.com/filmote/Frogger), BSD-3-Clause. No sound in original; ino renamed to cpp, includes flattened. Directions hop.
Bangi, Ignacio Vina (github.com/igvina/Bangi), Apache-2.0. Sound kept; ino concatenated, prototype added.
Firebox, Jonathan Howard (github.com/jhhoward/Firebox @53c4de2), MIT. ino converted to cpp, paint8Pixels buffered. Sel = drop, Sel+Down = A (fire), Up/Down choose, Left/Right move. No sound in original.
Fidget Spinner (github.com/ArduboyCollection/FidgetSpinner-Arduboy @903d133), MIT. Sound kept; ino converted to cpp. Sel = spin, Sel+Down = spin other way, Left/Right = design.
