# Error numbers

Every error starts with a number (`E270: battery too low ...`). The number does not change between languages or versions.
1xx package, 2xx USB link, download mode, identity checks, battery, 3xx flash write and checksum, 4xx backup and restore, 5xx SD card, 8xx command-line prompts, 9xx bugs. "CLI" marks errors only `rockslop-cli` gives.

To report one, press **COPY REPORT** on the error screen: it puts the error and the end of the log on the clipboard. The log is `rockslop.log` in the data directory, beside `backups/` (see the README); `rockslop-cli` prints its path when a command fails.

| Code | Meaning | What to do |
|------|---------|------------|
| E101 | The package file could not be opened. | Check the path. |
| E102 | Not a `.tar.xz` release. | Use the `.tar.xz` from the release page. |
| E103 | The archive could not be read. It is corrupt or cut short. | Download it again. |
| E104 | A file name in the archive is not UTF-8. | Not one of ours. Use an official release. |
| E105 | A file in the archive is far too large. | Not one of ours. Use an official release. |
| E110 | No MANIFEST at the start. Not a release for this flasher. | Use a Rockbox release made for rockslop. |
| E111 | MANIFEST is not UTF-8. | The package is broken. Build it again. |
| E112 | A MANIFEST line makes no sense (line number follows). | The package is broken. Build it again. |
| E113 | The package is in a newer format than this rockslop reads. | Update rockslop. |
| E114 | MANIFEST has no format. | The package is broken. Build it again. |
| E115 | MANIFEST names no player. | The package is broken. Build it again. |
| E116 | The package is in an older format than this rockslop reads. | Get a newer release of the package. |
| E120 | No SHA256SUMS after MANIFEST. | The package is broken. Build it again. |
| E121 | SHA256SUMS is not UTF-8. | The package is broken. Build it again. |
| E122 | A SHA256SUMS line makes no sense. | The package is broken. Build it again. |
| E123 | A SHA256SUMS hash is malformed. | The package is broken. Build it again. |
| E130 | A path in the archive points outside the card. Refused. | Do not use this package. |
| E131 | The archive contains a link or special file. Refused. | Do not use this package. |
| E132 | A file in the archive is not listed in SHA256SUMS. | The package is broken. Build it again. |
| E133 | A file fails its checksum. The download is corrupt. | Download it again. |
| E134 | The archive holds a file outside `flash/` and `card/`. | The package is broken. Build it again. |
| E135 | Files listed in SHA256SUMS are missing. | Download it again. |
| E136 | The package has no flash image. | The package is broken. Build it again. |
| E140 | A file passed its checksum, then failed it on a second read. The file changed, or the disk misread. | Leave the package alone while flashing. Copy it elsewhere and retry. |
| E150 | The package is for the other player. | Use the package for your player. |
| E151 | CLI: the package names a player rockslop does not know. | Use a yp3 or ts17 package. |
| E152 | No firmware image at the front of the package (it must come before the card files). | Build it again with the release script. |
| E153 | The package would write over the bootloader or the player's own settings. Refused; nothing was written. | Do not use this package. |
| E201 | CLI: the player did not appear in download mode in time. | Follow the steps again, or raise `--wait`. |
| E202 | CLI: the player is there, but this user may not open it. | Linux: install the udev rule. macOS: use sudo. Windows: install WinUSB with Zadig. |
| E205 | Another tool has the player open. | Close the other tool. |
| E210 | The player is not on the bus. | Reconnect it in download mode. |
| E211 | The player is not in the expected USB mode. | Reconnect it in download mode. |
| E212 | No permission to open the player. | Linux: install the udev rule. macOS: use sudo. Windows: install WinUSB with Zadig. |
| E213 | Another program is using the player. | Close it. Retry. |
| E214 | The player dropped off USB. | Check the cable. Reconnect in download mode. Retry. |
| E215 | The player stopped answering. | Unplug, enter download mode again, retry. |
| E216 | Some other USB error (message follows). | Reconnect and retry. Report it if it repeats. |
| E217 | A USB write was cut short. | Check the cable. Retry. |
| E218 | The player sent an empty reply. | Reconnect in download mode. Retry. |
| E220 | The boot ROM sent a garbled status. | Reconnect in download mode. Retry. |
| E221 | The boot ROM rejected a command. | Reconnect in download mode. Retry. |
| E230 | YP3: the flasher's helper did not load into the player's RAM. | Reconnect and retry. |
| E231 | YP3: the helper sent a garbled reply. | Reconnect and retry. |
| E232 | YP3: the helper reported an error (operation and status follow). | Reconnect and retry. Report it if it repeats. |
| E240 | TS17: the device is not an Actions download-mode ROM. | Reconnect it in download mode. |
| E241 | TS17: the USB hook did not load into RAM. | Reconnect and retry. |
| E242 | TS17: the flasher's helper did not load into RAM. | Reconnect and retry. |
| E243 | TS17: the helper's crash guard is not active. Refused. | Reconnect and retry. |
| E244 | TS17: the helper did not finish. | Unplug, enter download mode again, retry. |
| E245 | TS17: the helper crashed. | Unplug, enter download mode again, retry. Report it if it repeats. |
| E250 | TS17: the stock firmware gave an unexpected identity. Left alone. | Enter download mode by hand: hold POWER about 4 s while connecting. |
| E251 | The player did not come back in download mode. | Enter download mode by hand. |
| E252 | The YP3 cannot be switched to download mode by software. | Unplug, power off, hold Down while connecting USB. |
| E260 | YP3: the USB device is not a SmartLink boot ROM. Nothing was written. | Check it is a YP3 in download mode. |
| E261 | TS17: wrong chip ID. Nothing was written. | Check it is a TS17. |
| E262 | Wrong CPU ID. Nothing was written. | Check it is the player you picked. |
| E263 | YP3: unknown flash chip. Nothing was written. | Unsupported hardware. Report it. |
| E264 | TS17: no partition table in flash. Nothing was written. | Unexpected flash layout. Report it. |
| E265 | TS17: the partition table does not put the firmware partition (`fw0_sys`) where the flasher writes. Nothing was written. | Unexpected flash layout. Report it. |
| E270 | Battery too low to flash. | Charge it, then retry. |
| E301 | The player's bootloader is not the one the package was built for. Nothing was written. | Use a package for this player. Report it if it should match. |
| E310 | YP3: a write to a protected sector was refused. | Should not happen. Report it. |
| E311 | TS17: a write to a protected sector was refused. | Should not happen. Report it. |
| E315 | YP3: writing a sector failed (status follows). | Retry. Download mode still works and the backup is on disk. |
| E316 | TS17: erasing a sector failed. | Retry. Download mode still works and the backup is on disk. |
| E317 | TS17: programming a sector failed. | Retry. Download mode still works and the backup is on disk. |
| E318 | TS17: a written sector read back wrong. | Retry. Download mode still works and the backup is on disk. |
| E320 | YP3: the player could not checksum its flash. | Reconnect and retry. |
| E321 | A sector's checksum is wrong after writing. | Retry. If it repeats, return to stock and report it. |
| E330 | YP3: the player refused to reset. | Unplug and press RESET. |
| E331 | The new firmware did not start; the player is back in download mode. | Retry, or return to stock. |
| E401 | YP3: a sector read differently twice during the backup. Nothing was written. | Reconnect and retry. |
| E402 | TS17: two reads of the flash disagree during the backup. Nothing was written. | Reconnect and retry. |
| E410 | The backup folder could not be created. | Check disk space and permissions. |
| E411 | The backup could not be written. | Check disk space and permissions. |
| E412 | The backup read back wrong from disk. Nothing was written to the player. | Check the disk. |
| E413 | The backup could not be moved into place. | Check disk space and permissions. |
| E414 | The backup's checksum file could not be written. | Check disk space and permissions. |
| E420 | CLI: no stock backup on file for this player. | Give the image path. |
| E421 | The image file could not be read. | Check the path. |
| E422 | The image is not the size of a full flash image. | Use a full backup of this player. |
| E423 | The image does not match its `.sha256` file. It is damaged. | Use another backup. |
| E501 | Waiting for the card was cancelled. | Flash again. |
| E502 | The card could not be mounted (five tries). | Mount it yourself, then flash again. Unchanged sectors are skipped. |
| E503 | The card never appeared. | Put a microSD card in the player. Flash again. |
| E510 | Not enough free space on the card. | Free some space. Flash again. |
| E520 | A folder could not be created on the card. | Check the card is not full or locked. Flash again. |
| E521 | A file could not be created on the card. | Check the card is not full or locked. Flash again. |
| E522 | Writing to the card failed. | Flash again. Try another card if it repeats. |
| E523 | Flushing a file to the card failed. | Flash again. Try another card if it repeats. |
| E530 | macOS: remounting the card failed (diskutil's message follows). | Close anything using the card. Flash again. |
| E531 | macOS: the card vanished after remounting. | Flash again. |
| E532 | macOS: the card did not remount. | Flash again. |
| E533 | Windows: the card's volume could not be opened. | Close anything using the card. Flash again. |
| E534 | Windows: another program has the card open. | Close it. Flash again. |
| E540 | A file could not be read back from the card. | Try another card. |
| E541 | A file read back from the card is not what was written. | The card is failing or fake. Use another. |
| E801 | CLI: you said no. | Nothing to do. |
| E802 | CLI: not on a terminal, so nobody could confirm. | Pass `--yes`. |
| E803 | CLI: the answer could not be read. | Pass `--yes`. |
| E901 | The flasher crashed. A bug. | Report it with the message. |
| E902 | TS17: an internal command list is too long. A bug. | Report it. |
