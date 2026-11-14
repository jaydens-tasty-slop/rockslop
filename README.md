# rockslop

A completely normal [Rockbox](https://www.rockbox.org) flasher. It installs
Rockbox on two cheap MP3 players, and puts them back to stock:

- **YP3**: SmartLink SL6801, 2 MiB SPI NOR, 128x160 LCD, firmware `yp3_*`.
- **TS17**: Actions "Lark" (chip 0x2351), 4 MiB SPI NOR.

`rockslop` is the graphical flasher and `rockslop-cli` is the command-line
one. Both are single native binaries for Linux and Windows (x86-64 and
arm64) and macOS (universal).

Builds are on the [releases page](../../releases). Every push to `master`
publishes one, named after its commit's time (UTC) and hash, e.g.
`20261028_141519-1a2b3c4`; the app shows it under the logo and
`rockslop-cli --version` prints it. Linux binaries need `chmod +x` after
downloading.

## Before the first flash

The first time **FLASH** is pressed on a computer, rockslop asks you to
sign a waiver before it writes anything. It starts by saying how this
software was made: rockslop and the Rockbox ports are 100% vibe-coded, no
human has ever looked at the code, and testing on real players is all the
checking it has had. Then come three points:

- **This can brick your player.** Rockbox on these players is unofficial
  and experimental, and a flash that goes wrong can leave a player dead
  for good.
- **Your warranty is void.** Treat the player as yours to fix from then on.
- **Don't take it back.** Never return a flashed player, claim on it, or
  complain to the seller or the maker. If flashed players come back, the
  next batch gets locked down, and that's the end of Rockbox on them for
  everyone.

You tick each point, then hold **HOLD TO SIGN**. There are no backsies.
The signature is kept in rockslop's data folder (`waiver`, beside
`backups/`). `rockslop-cli flash` asks too: you type `NO BACKSIES`, or
pass `--no-backsies`. `--yes` doesn't sign the waiver. The demo shows the
waiver on every flash and never saves the signature. Returning to stock
never asks.

## What a flash does

1. **Link.** Put the player in download mode (below). The flasher finds it,
   kicks a TS17 from stock into its boot ROM, and probes the CPU, the flash
   chip and the battery. If the battery is below 3.3 V it refuses.
2. **Backup.** The whole flash is read and saved before anything is
   written (see *Backups*). This step is never skipped.
3. **Verify.** The package's checksums are checked. Then the player's boot
   sectors are compared against the package's expectations, and a player
   that doesn't match is refused.
4. **Write.** Only sectors that differ are erased and programmed. Every
   sector is checksummed on the device afterwards.
5. **Reboot** into Rockbox. When its USB disk appears, **Sync** copies the
   card files onto the microSD card, reads each one back against its
   SHA-256, and ejects the disk. A file the card already holds as the
   package has it is only read and compared, not written again, and one
   that differs is written from its first difference on, so a reflash
   copies just what changed. If there is no card, the flasher waits
   and asks for one. A copy of the stock backup goes onto the card too, as
   `.rockbox/{player}-{unit}-{time}-stock.bin` (see *Backups*).

**Return to stock** writes a backup back the same way. It is always on
offer: it takes this computer's stock backup, else the one on the card of
a player plugged in running Rockbox, else asks for the file.

**TS17 rescue stub.** A new Rockbox install on a TS17 writes a 4 KiB
rescue stub at `0x14000` first and verifies it, then writes Rockbox behind
it at `0x15000`. Later flashes never touch it, even a stub that isn't
rockslop's (that is logged). Returning to stock removes it; the next
Rockbox install counts as new again. If the player won't start: hold the
D-pad's Down button (or the power button), plug in USB, and keep holding
about 3 seconds. The screen stays dark; rockslop will find it. Holding the
power button about 7 seconds restarts the player.

## Download mode

| player | how |
|---|---|
| YP3 | unplug it and power it off (hold the centre key; it won't power off on USB). Then hold **Down** (bottom of the wheel) while connecting USB, and let go once it is found. Or hold Down and tap **RESET**. |
| TS17 | connect it, switched on. Stock firmware is kicked into download mode automatically. If Rockbox is already running, or the player won't start, unplug it and switch it off, then hold **Down** on the D-pad (or **Power**) while connecting and keep holding about 3 s: the screen stays dark. Or, in Rockbox, pick System > Debug (Keep Out!) > *Reboot to USB download mode*. |

The GUI shows these steps on screen. The CLI prints them.

## Using it

```
rockslop                         # the GUI
rockslop --demo                  # a simulated player; nothing touches USB
rockslop --mute                  # no sound (M toggles it; muting in the app is remembered)
rockslop --lang deutsch          # interface language (below)
rockslop --render-scale 0.5      # render at half the screen's resolution

rockslop-cli status              # players on USB, and their mode
rockslop-cli check  rockbox-yp3-4.0.tar.xz
rockslop-cli probe  yp3|ts17
rockslop-cli flash  rockbox-yp3-4.0.tar.xz
rockslop-cli restore yp3|ts17 [IMAGE]   # default: a stock backup here, or on Rockbox's card
rockslop-cli backups [yp3|ts17]
rockslop-cli card                # where Rockbox's USB disk is mounted
    --yes  (don't ask)  --wait SECS  (default 300)  --demo
    --no-backsies  (sign the first flash's waiver without typing it)
```

**DEMO** at the bottom of the first screen switches the GUI to the
simulated player and back, as `--demo` does.

### Languages

The interface is translated into every language Rockbox ships. It starts
in the system's language, or English (UK) when Rockbox has no match. The
button with the globe at the top right (or **L**) picks another, and the
choice is remembered. `--lang` takes a Rockbox language name (`deutsch`,
`japanese`, `arabic`, ...), `auto`, or `pseudo` (stretched placeholder
text, for checking layout).

The translations are machine-made. Corrections are welcome in
`assets/lang/`, one file per language.

### Graphics

The GUI renders at the screen's own resolution (at most twice its point
size on high-DPI screens) and lowers it by itself if frames come slower
than about 48 per second, as on integrated graphics; it prints a line
when it does. `--render-scale` fixes it instead: 1 is full resolution,
0.5 is half. It draws at most about 75 frames a second, so a 120 Hz
screen doesn't double the work.

### Errors

Every error starts with a number, such as `E270: battery too low`. The
number is the same in every language. [ERRORS.md](ERRORS.md) lists them
all, with what to do about each. **COPY REPORT** on the error screen puts
the error and the end of the log on the clipboard, for a bug report.

Both binaries log what they do (commands, probe results, messages,
failures) to `rockslop.log` in the same data directory as the backups
(below). It moves to `rockslop.log.1` once it passes 1 MiB.

### Tips

Where people tend to get stuck (telling the players apart, download mode,
the package, the card, an error), Clippy turns up after a while with a
tip. **GOT IT** puts that tip away for the session.

### Backups

Backups go in `rockslop/backups/` under the platform's data directory:

| platform | location |
|---|---|
| Linux | `~/.local/share` |
| macOS | `~/Library/Application Support` |
| Windows | `%APPDATA%` |

Each file is named `{player}-{unit}-{UTC time}-{stock|other}.bin` and has a
`.sha256` beside it. `{unit}` is a CRC of the unit's own calibration
sectors, so a backup is matched to the player it came from. A backup with
no Rockbox in it is marked `stock`. Return to stock uses the newest stock
backup of that unit (or of another unit: the part it writes is the same).

A flash also copies the stock backup and its `.sha256` onto the player's
card, into `.rockbox/` (a hidden folder). On a computer without a stock
backup, rockslop takes it from there when the player is plugged in
running Rockbox, during the link or a flash. Or pick the file on the
payload panel: it must be a whole-flash image of the player with its
partition table, and its firmware is checked against the stock versions
rockslop knows (`KNOWN` in `src/device/backup.rs`). One it doesn't know is
flagged, and **FLASH ANYWAY** takes it all the same.

## Platform setup

- **Linux**: install the udev rule so you don't need root:
  `sudo cp packaging/udev/70-rockslop.rules /etc/udev/rules.d/ && sudo udevadm control --reload`,
  then reconnect the player. The SD card is mounted and ejected through
  udisks (`udisksctl`).
- **Windows**: both boot ROMs need the WinUSB driver. Use
  [Zadig](https://zadig.akeo.ie/) on `301a:2800` (YP3) and on `10d6:10d6`
  (TS17). Do this once per player, in download mode.
- **macOS** 11 or newer, Intel or Apple Silicon (`Rockslop.app` and
  `rockslop-cli-macos` are universal). The YP3's boot ROM presents as mass
  storage, so macOS claims it. The CLI may need `sudo` to detach it. Both
  are signed ad hoc, not notarized by Apple (that needs a paid developer
  account): the first time, open the app, then pick **Open Anyway** in
  System Settings > Privacy & Security. Or run
  `xattr -dr com.apple.quarantine Rockslop.app rockslop-cli-macos`.

The Windows and macOS paths compile but are **untested on hardware**.

The Windows `.exe` files aren't code-signed, so some antivirus products
(Windows Defender among them) may flag them on heuristics: an unsigned,
new file with a lot of compressed data inside. That is a false positive;
the source is all here.

The Linux arm64 binaries need glibc 2.34 or newer. On Windows on ARM,
`rockslop-arm64.exe` needs OpenGL: if its window doesn't open, install
Microsoft's OpenGL compatibility pack from the Microsoft Store.

## Packages

A package is a plain `.tar.xz`:

```
MANIFEST              format=2, target=yp3|ts17, version, built, title,
                      expect=OFFSET SIZE SHA256  (flash that must already match)
SHA256SUMS            sha256sum format, one line per file below
flash/0x0000e000.bin  an image, written at the offset in its name
card/.rockbox/...     copied to the root of the SD card
```

`format` is the layout version: rockslop refuses a package in a format it
doesn't read, and says whether rockslop or the package needs updating. A
package is only flashed onto the player its `target` names. YP3 packages
may still be format 1. A TS17 package must be format 2: one image,
`flash/0x00015000.bin`, which is Rockbox without the rescue stub. The stub
comes from rockslop itself, never from a package.

`packaging/mkpkg.py` builds them. It uses only the Python standard library
and gives reproducible output under `SOURCE_DATE_EPOCH`:

```
packaging/mkpkg.py yp3 --elf rockbox.elf \
    --codecs codecs.flash --card CARD --version 4.0-yp3 -o rockbox-yp3-4.0.tar.xz
packaging/mkpkg.py ts17 --bin rockbox.bin --card CARD --version 4.0-ts17 -o rockbox-ts17-4.0.tar.xz
packaging/mkpkg.py --help
```

For the TS17, `--bin` is the Rockbox build's `rockbox.bin`, stub included.
mkpkg.py refuses it unless its first 4 KiB is the pinned stub (sha256
`aa13ea64…44f0f`) and the vectors behind it pass the stub's own check. It
then drops the stub.

The card tree never holds the firmware itself (`.rockbox/rockbox.yp3`,
`.rockbox/rockbox.ts17`): the firmware is in flash, and a copy on the card
makes Rockbox offer to reboot into it, which fails. mkpkg.py leaves it
out, and the flasher deletes one it finds on the card.

The flasher won't write outside each player's writable window:

| player | writable window |
|---|---|
| YP3 | `0xE000..0x1FC000` (not the bootloader, partition table or calibration) |
| TS17 | `0x15000..0x1BB000`. The rescue stub at `0x14000..0x15000` is written only by rockslop, on a new install; a restore writes from `0x14000` |

## Building

```
make              # debug build (GUI + CLI)
make release
make demo         # run the GUI against the simulator
make test
make linux        # portable binaries in dist/, built in a podman container
make linux-arm64  # the same for arm64 (cross-compiled, container)
make windows      # .exe files in dist/ (MinGW, container)
make windows-arm64  # the same for arm64 (llvm-mingw, container)
make macos        # universal .app + CLI (osxcross, container; put a
                  # MacOSX*.sdk.tar.xz in packaging/containers/sdk/ first,
                  # made with osxcross's tools/gen_sdk_package_*.sh)
make flatpak      # dist/rockslop.flatpak (needs flatpak-builder)
make payloads     # rebuild the on-device payloads (prebuilt ones are committed)
```

On a Linux host the build needs a Rust toolchain plus the ALSA headers
(`libasound2-dev` or `alsa-lib`). libusb is vendored and linked in
statically.

CI (`.github/workflows/build.yml`) runs the tests and the same container
builds for Linux, Windows and the flatpak, and publishes the release. It
can't build for macOS (Apple's SDK can't go there): run `make macos` and
`gh release upload TAG dist/Rockslop-macos.zip dist/rockslop-cli-macos`.

## Layout

```
src/device/    USB, both boot-ROM protocols, backups, SD sync, simulator (lib)
src/pkg.rs     package reader and verifier (lib)
src/bin/       rockslop-cli
src/gfx/       drawing
src/app.rs     the GUI's screens; src/ui.rs widgets; src/audio.rs sound
src/clippy.rs  Clippy and his tips
src/i18n.rs    translations (assets/lang/); src/picker.rs language picker
src/credits.rs credits (from CREDITS.md and Rockbox's own list)
payloads/      code that runs on the players (see CREDITS.md)
packaging/     mkpkg.py, udev rule, containers, flatpak, macOS bundle
assets/        fonts, sound, translations, Rockbox's credits list
docs/          the making-of sketch
vendor/        miniquad 0.4.11 with drag-and-drop, high-DPI and MSAA fixes (PATCHES.md)
```

Upstream code and assets are listed in [CREDITS.md](CREDITS.md).

## Making of

![The first sketch, on paper: CHOOSE YOUR PLAYER, the YP3 and the TC17
with their buttons and ports labelled, and notes: "CYBERSPACE" FULL 3D
BACKGROUND, WIREFRAMES ROTATE SLOWLY, YOU CAN GRAB THEM & ORBIT, HINT BOXES
TRACE TO FEATURE ON PLAYER](docs/making-of.jpg)

The plan.
