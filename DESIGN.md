xX ROCKSLOP FLASHER Xx

A completely over the top, unhinged flasher for the YP3 and TS17, featuring
full 3D UI, fluid 60fps animations, and a slightly cringe hacker aesthetic.

"floating in the cyberspace" is the theme

This isn't a winforms app - it's something trying to be too edgy and cool

very restrained on screen strings. edgy font. stark, harsh dark color theme
with contrasting, vivid colors. rotating 3D wireframes of MP3 players. 

## Requirements

- MacOS, Windows, Linux (via flatpak or something)
  - it would be nice if it were buildable for all targets from linux in podman containers
- makefile to build
- Self contained - no installer
- Minimal binary size
- native code - dealers choice (rust, c++, etc)
- Full 3D ui/gratuitous animations - dealers choice. see if there is a modern framework to use here
- as few steps as possible. select player -> how to get into download mode -> select zip -> one button to flash/backup -> no manual SD card copy step

- the 3D model of the player never goes off screen - you pick it on the first 
screen, and it smoothly transitions from one screen to the next with flourish

## YP3/TS17 Models
They are almost identical in size. 3 1/2" tall, 1 1/2 + 1/16" wide, 3/8" thick

### TS17
- vol buttons on right
- down on d pad is "back"
- reset on back
- usbc on left, headphone on right
- power button on bottom in between
- often (but not always?) has two tone aesthetic

### yp3

- reset on right
- usbc on right, headphones on left
- speaker on back towards top

## Flow

"select your player" screen -> smoothly transitions to instructions on how to get
it to download mode with an animation of eg a usbc cable being plugged in and buttons
being pressed / highlighted

"select image" triggers a native file picker popup -> FLASH triggers a cool animation
of rockbox being flashed on the screen. status text tells what is going on.

should load SD card in one step without user having to do anything

## Verification

Since these are random chinese devices, in boot rom mode we should try and
do some device probes to make sure the CPU flash etc is what we are expecting.

remind user to insert microsdcard with animation

after flashing / loading files, do checksum to double check

## release zips

design a simple highly compressed format that contains flash images + sd card tree to copy over
+ some metadata to show in UI (build date, etc) and checksums for verification

## Backup

Before flashing, always back up the stock firmware, and then afterwards offer
a "return to stock" option.

