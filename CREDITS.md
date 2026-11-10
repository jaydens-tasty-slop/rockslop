# Credits

Rockslop stands on other people's work. Files that adapt upstream code say
so in their header comment as well.

<!-- The flasher's credits roll reads this file (src/credits.rs): each "## "
heading, and each bullet of the form

- **Title** by Author [licence] <https://link>

where "by Author", "[licence]" and "<link>" are optional; lines indented
under a bullet are notes, shown here only. Headings must be keys in
assets/lang/strings.txt, so that they translate. -->

## Made by

- **Jayden Milne**
  The idea and the design, the YP3 and TS17 Rockbox ports and their
  bring-up tools, and the reference photos.
- **Claude** by Anthropic <https://www.anthropic.com>
  Wrote this flasher with Jayden, and machine-translated its interface
  into Rockbox's languages (corrections from native speakers welcome).
- **Clippy**
  The helper with the tips, after Clippit, the Office Assistant of
  Microsoft Office 97 to 2003. Drawn from scratch as a wireframe; no
  Microsoft artwork is used, and rockslop has nothing to do with Microsoft.

## Device protocols and payloads

- **smartlink_flash** by Ilya Kurdyukov <https://github.com/ilyakurdyukov/smartlink_flash>
  `smtlink_dump.c`: the SmartLink SL6801 boot ROM protocol (USB
  301a:2800, mass-storage BOT with vendor CDBs `0xC0 | cmd`, RUNRAM/exec)
  in `src/device/yp3.rs` and `src/device/bot.rs` follows this dumper. Its
  own flash commands are not used: they hang in the watchdog-return ROM
  state.
- **actions_flash** by Ilya Kurdyukov <https://github.com/ilyakurdyukov/actions_flash>
  The Actions ADFU command set and the known chip IDs, which the TS17's
  boot ROM protocol (`ts17dev/adfu.py`, ported in `src/device/ts17.rs`)
  was worked out from. Source read, not run; no code copied.
- **atj2127decrypt** by Nicholas FitzRoy-Dale [MIT] <https://github.com/nfd/atj2127decrypt>
  An independent ADFU implementation with the same endpoints: the
  cross-check.
- **adfuload** by Marcin Bukat and Amaury Pouly [GPL-2.0-or-later] <https://www.rockbox.org>
  Rockbox's earlier ADFU loader for the ATJ213x (`utils/atj2137/adfuload`),
  with a different command format.
- **SanDisk Clip Sport Recovery Mini-HOWTO** by Roman Yepishev <https://gist.github.com/roman-yepishev/737dfda3a0a853fe730286d3ce49fccd>
  The ATJ2127 boot chain and flashing process.
- **ATJ212x/ATJ215x write-up** <https://lotide.fbxl.net/collection_targets/1524/items/9811>
  Background on the older Actions chip families.
- **U-Boot's Cubieboard7 documentation** by the U-Boot contributors <https://docs.u-boot.org/en/latest/board/actions/cubieboard7.html>
  The same `10d6:10d6` download mode on another Actions chip.
- **rockbox-yp3 yp3dev** by Jayden Milne
  The YP3 service payload (`payloads/yp3/svc.c`, `entry.S`, `svc.ld`;
  adapted, adds a read-only JEDEC op), the host side of it
  (`yp3dev/yp3.py`) that `src/device/yp3.rs` ports, and the flash placer
  `yp3dev/package.py` that `packaging/mkpkg.py` runs.
- **rockbox-yp3 ts17dev** by Jayden Milne
  The TS17 ADFU hook and register-script gadget (`payloads/ts17/hook.S`,
  `ops.c`, `gadget.ld`; copied unchanged), the ADFU, NOR and stock-kick
  sequences (`adfu.py`, `norprobe.py`, `stock.py`) that
  `src/device/ts17.rs` ports, and the NOR data randomizer model
  (`rdm.py`) that `src/device/rdm.rs` ports.
- **Rockbox** by the Rockbox contributors [GPL-2.0-or-later] <https://www.rockbox.org>
  The firmware this tool installs, and the SL6801 / Actions target ports
  it comes from. Its credits list (`docs/CREDITS`) is copied unchanged to
  `assets/rockbox/CREDITS` for the second pane of the credits roll, and
  its language list is the one the interface is translated into.

## Music

Tracker modules from The Mod Archive, unmodified (stored xz-compressed in
the binary) and played live by xmrsplayer, shuffled.

- **Kicking It Powerful** by null1024 [CC BY 4.0] <https://modarchive.org/index.php?request=view_by_moduleid&query=170242>
- **Keygen Wraith** by Drozerix [Public domain] <https://modarchive.org/index.php?request=view_by_moduleid&query=207854>
  Public domain as set by the registered artist on The Mod Archive (as
  are the other Drozerix and Katie Cadet modules).
- **m4's chipgabber** by M4kachu [CC BY 4.0] <https://modarchive.org/index.php?request=view_by_moduleid&query=212725>
- **Glitch Abuse** by usrfriendly [CC0 1.0] <https://modarchive.org/index.php?request=view_by_moduleid&query=163775>
- **Kick the beat** by Andreas Viklund (TSEC) [CC BY 4.0] <https://modarchive.org/index.php?request=view_by_moduleid&query=34320>
  A 90s scene module; its samples are of unknown origin.
- **Gate 303** by Andreas Viklund [CC BY 4.0] <https://modarchive.org/index.php?request=view_by_moduleid&query=34313>
  Its own text says the samples are "from just about everywhere".
- **Neon Techno** by Drozerix [Public domain] <https://modarchive.org/index.php?request=view_by_moduleid&query=178172>
- **Bassline Techno** by Katie Cadet [Public domain] <https://modarchive.org/index.php?request=view_by_moduleid&query=175335>

## Voice lines

From Freesound. Each is cut to one "access granted" take, high-passed at
110 Hz, bitcrushed (ffmpeg `acrusher` bits=8 mode=log samples=3),
normalised, and made mono 22.05 kHz.

- **Access Denied/Granted (male)** by Sonoticai [CC0 1.0] <https://freesound.org/people/Sonoticai/sounds/524626/>
  The fourth take, 7.66-9.36 s.
- **Access Denied/Granted (female)** by Sonoticai [CC0 1.0] <https://freesound.org/people/Sonoticai/sounds/524628/>
  The fourth take, 7.67-9.36 s.
- **Access Granted** by JohnnyRaySounds [CC0 1.0] <https://freesound.org/people/JohnnyRaySounds/sounds/502221/>
  0.08-1.62 s.

## Sound effects

The interface's sounds before the music starts, from Freesound's
high-quality previews. Each has its leading silence removed, is cut short
with a fade-out, peak-normalised to -1 dB and made mono 44.1 kHz
(`assets/audio/sfx/`).

- **TR08_Clave01** by Karman_Lyne [CC0 1.0] <https://freesound.org/people/Karman_Lyne/sounds/519304/>
  Pointer over a control (`hover.wav`), the first 80 ms.
- **PC Game UI Select** by plasterbrain [CC0 1.0] <https://freesound.org/people/plasterbrain/sounds/464905/>
  A click (`click.wav`).
- **Rave Stab** by a former Freesound user [CC0 1.0] <https://freesound.org/people/deleted_user_8563950/sounds/443931/>
  The main button of each screen (`confirm.wav`).
- **Filter Sweep Down** by SecureSubset [CC0 1.0] <https://freesound.org/people/SecureSubset/sounds/786132/>
  Between screens (`transition.wav`), the first 0.4 s.
- **Loading_Energy_Basic_Retro Sci-Fi_Dry** by cabled_mess [CC0 1.0] <https://freesound.org/people/cabled_mess/sounds/364935/>
  The title (`boot.wav`), the first 1.4 s.
- **UIMisc_Digital, Interface, Message, Selection, Confirmation, Alert_33** by JW_Audio [CC0 1.0] <https://freesound.org/people/JW_Audio/sounds/828726/>
  A player answers (`link.wav`).
- **Digital Error** by dotY21 [CC0 1.0] <https://freesound.org/people/dotY21/sounds/349899/>
  A failure or a rejected package (`error.wav`).
- **Select, Granted 01** by LilMati [CC0 1.0] <https://freesound.org/people/LilMati/sounds/404151/>
  A package checks out (`drop.wav`).

## Fonts

Licence texts in `assets/fonts/` (`OFL-*.txt`). The interface's own fonts cover Latin;
the others fill in, glyph by glyph, for every other script a translation
uses (subsets made by `packaging/fonts.py`).

- **Orbitron** by Matt McInerney and The Orbitron Project Authors [OFL-1.1] <https://github.com/theleagueof/orbitron>
  Black and SemiBold.
- **Share Tech Mono** by Carrois Type Design, Ralph du Carrois [OFL-1.1] <https://fonts.google.com/specimen/Share+Tech+Mono>
- **Noto Sans** by the Noto Project Authors [OFL-1.1] <https://github.com/notofonts>
  Noto Sans, Sans Arabic, Sans Hebrew, Sans Devanagari and Sans Thai
  (Regular, Bold), subset to their scripts.
- **Noto Sans CJK** by Adobe and the Noto Project Authors [OFL-1.1] <https://github.com/notofonts/noto-cjk>
  The SC, TC, JP and KR subset fonts, cut down to the characters the
  translations use.
- **GNU Unifont** by Roman Czyborra, Paul Hardy and contributors [OFL-1.1 / GPL-2.0-or-later with font embedding exception] <https://unifoundry.com/unifont/>
  The last resort for any other character. Version 18.0.01, xz-compressed.

## Rust crates

Statically linked into the binaries. Full dependency list in `Cargo.lock`.

- **macroquad** [MIT OR Apache-2.0] <https://crates.io/crates/macroquad>
  Window, GL, input.
- **miniquad** [MIT OR Apache-2.0] <https://crates.io/crates/miniquad>
  Vendored in `vendor/miniquad/` with fixes for X11 drag and drop,
  the high-DPI window size and multisample resolves (`PATCHES.md` there).
- **rusb** [MIT] <https://crates.io/crates/rusb>
  USB.
- **libusb** by the libusb project [LGPL-2.1-or-later] <https://libusb.info>
  Vendored by libusb1-sys and linked statically, so the full source of
  this program is the relinking offer.
- **rfd** [MIT] <https://crates.io/crates/rfd>
  Native file picker.
- **cpal** [Apache-2.0] <https://crates.io/crates/cpal>
  Audio output.
- **xmrs** [MIT] <https://crates.io/crates/xmrs>
  Tracker modules.
- **xmrsplayer** [MIT] <https://crates.io/crates/xmrsplayer>
  Tracker module playback.
- **rustybuzz** [MIT] <https://crates.io/crates/rustybuzz>
  Text shaping (Arabic, Devanagari, Thai and the rest).
- **ttf-parser** [MIT OR Apache-2.0] <https://crates.io/crates/ttf-parser>
  Font outlines.
- **ab_glyph_rasterizer** [Apache-2.0] <https://crates.io/crates/ab_glyph_rasterizer>
  Glyph rasterizing.
- **unicode-bidi** [MIT OR Apache-2.0] <https://crates.io/crates/unicode-bidi>
  Right-to-left text.
- **sys-locale** [MIT OR Apache-2.0] <https://crates.io/crates/sys-locale>
  The system language.
- **lzma-rust2** [Apache-2.0] <https://crates.io/crates/lzma-rust2>
  .tar.xz packages, fonts and music.
- **tar** [MIT OR Apache-2.0] <https://crates.io/crates/tar>
  .tar.xz packages.
- **sha2** [MIT OR Apache-2.0] <https://crates.io/crates/sha2>
  Checksums.
- **crc32fast** [MIT OR Apache-2.0] <https://crates.io/crates/crc32fast>
  Checksums.
- **libc** [MIT OR Apache-2.0] <https://crates.io/crates/libc>
  OS calls.
- **windows-sys** [MIT OR Apache-2.0] <https://crates.io/crates/windows-sys>
  OS calls.

## Reference photos

- **YP3 and TS17 photos** by Jayden Milne
  The player models in `src/gfx/model.rs` are measured from these photos of
  both players.
