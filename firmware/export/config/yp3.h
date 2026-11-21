/* SPDX-License-Identifier: GPL-2.0-or-later */
#define MODEL_NAME "YP3 SL6801 (experimental)"
/* Bluetooth device name: the controller keeps only 32 bytes of it */
#define BT_LOCAL_NAME "Rockbox YP3"
#define MODEL_NUMBER 255
#ifndef SIMULATOR
/* Filesystem handles and disk cache. Every open file and directory keeps
 * one disk-cache sector buffer while open; the FAT code needs ~10 more at
 * once. With too few left the LRU evicts sectors still in use and directory
 * writes go astray (seen as "Dir entry ... is not free" / duplicate entries
 * with 16 entries for 8 files + 6 dirs, and with 8 for 4 + 2). */
#define MAX_OPEN_FILES      8   /* database: scan/commit 3, playback 2-3 */
#define MAX_OPEN_DIRS       6   /* database scan holds one per folder level */
/* plugins/lib/mul_id3.c (stats, properties): the folder walk closes a
 * folder while it walks a subfolder, so depth is not capped at 6 (torture:
 * stats failed on /.rockbox/rocks/games/gamebuino/reuben/assets) */
#define PLUGIN_DIRWALK_REOPEN
#define DC_NUM_ENTRIES      24
#define DC_MAP_NUM_ENTRIES  96
#if DC_NUM_ENTRIES < MAX_OPEN_FILES + MAX_OPEN_DIRS + 10
#error "disk cache too small for the open file/dir limits"
#endif
/* Keep FAT sector aggregation within the YP3 adapter's 128-sector cap. */
#define FAT_MAX_TRANSFER_SIZE 128
#endif
#define CONFIG_CPU SL6801
#define CPU_FREQ 192000000
/* PLL2 384 MHz / 6 or / 2 (system-sl6801.c); 192 MHz is the highest verified. */
#define HAVE_ADJUSTABLE_CPU_FREQ
/* libc parts whose licence combines with GPLv3 plugins */
#define HAVE_GPL3_COMPATIBLE_LIBC
/* UI work after a key runs boosted (WPS entry starved audio) */
#define HAVE_GUI_BOOST
#define CPUFREQ_MAX     192000000
#define CPUFREQ_NORMAL   64000000
#define CPUFREQ_DEFAULT CPUFREQ_NORMAL
#ifndef SIMULATOR
#define TIMER_FREQ CPU_FREQ
#else
/* Explicit host heap, independent of the historical MiB feature bucket. */
#define TARGET_SIMULATOR_HEAP_SIZE (200 * 1024)
#endif
#define CONFIG_KEYPAD YP3_PAD
/* The centre key's long press powers off (button-yp3.c). Plugins get it as
 * a plain key (button_set_power_hold()), and the main menu offers Shut
 * down instead. */
#define HAVE_SELECT_POWER_KEY
#define CONFIG_LCD LCD_YP3
#define LCD_WIDTH 128
#define LCD_HEIGHT 160
#define LCD_DPI 100
/* Three default WPS metadata lines, plus one spare scrolling line. */
#define LCD_SCROLLABLE_LINES 4
/* Yes/no prompts wrap at spaces instead of scrolling (gui/yesno.c) */
#define HAVE_YESNO_WORDWRAP
/* On-screen keyboard: every character on one page of 16-cell rows, and a
 * hint line naming the hold actions that finish an edit (five keys, no
 * touch buttons; recorder/keyboard.c) */
#define HAVE_KBD_COMPACT_LAYOUT
/* Boot logo: this note under it, in red (apps/main.c) */
#define BOOT_LOGO_NOTE "Unofficial Build"
#define HAVE_LCD_COLOR
#define LCD_DEPTH 16                /* RGB565, the GC9106's native format */
#define LCD_PIXELFORMAT RGB565SWAPPED /* byte order the LCD FIFO sends (DMA) */
#define HAVE_LCD_INVERT
#define HAVE_LCD_ENABLE
/* The panel sleeps a while after the backlight goes off (setting "Sleep
 * (after backlight off)"); waking it costs ~0.2 s, turning the light back
 * on within that time is instant. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#define HAVE_LCD_BITMAP
#define IRAM_LCDFRAMEBUFFER __attribute__((section(".framebuffer")))
#ifndef SIMULATOR
/* An LCD update sleeps (DMA and its mutex): the scroll thread pushes its
 * line only after restoring the UI thread's viewport (lcd-scroll.c) */
#define HAVE_LCD_UPDATE_SLEEPS
#endif
#define HAVE_YP3_CODEC
#define HAVE_SW_VOLUME_CONTROL
/* Software-volume chunks match the 256-sample mixer frame (saves 6 KiB). */
#define PCM_PLAY_DBL_BUF_SAMPLES 256
/* Amplifier on GPIO1.30; no headphone detect. */
#define HAVE_SPEAKER
/* On: the headphone jack cuts the speaker in hardware (user test
 * 2026-10-20) */
#define DEFAULT_SPEAKER_MODE    1
#define HW_SAMPR_CAPS SAMPR_CAP_44
#ifndef SIMULATOR
/* USB: MUSB-style controller at 0x40040000 (usb-drv-yp3.c), full speed,
 * EP1 bulk IN/OUT, EP2 interrupt IN. */
#define HAVE_USBSTACK
#define USB_HAS_BULK
#define USB_NUM_ENDPOINTS 3
#define USB_DEVBSS_ATTR __attribute__((aligned(4)))
#define USB_VENDOR_ID  0x1209   /* pid.codes test ID; not the ROM's 301a */
#define USB_PRODUCT_ID 0x0001
#define HAVE_USB_POWER          /* "USB mode": mass storage or charge only */
/* External power is the only cable signal (PC and wall charger look the
 * same): connect at once and enter USB mode on the first host request. */
#define USB_DETECT_BY_REQUEST
#ifdef YP3_DEV
#define HAVE_DEVLOG             /* devlog.h; the ring: dev-yp3.c */
/* CPU time per thread and in WFI for the dev shell's "load"
 * (kernel-sl6801.c; core_sleep() in system-target.h) */
#define HAVE_THREAD_CPU_ACCOUNTING
#define DEV_CARD_DIR "/yp3dev"  /* the dev shell's files on the card */
/* CDC-ACM dev shell (apps/dev_shell.c); shares EP1 with mass storage, so it
 * runs in charge-only mode while Rockbox keeps running. */
#define USB_ENABLE_SERIAL
#define USB_SERIAL_RX_HOOK dev_serial_rx
/* usb-drv-yp3.c: one request, one packet */
#define USB_SERIAL_RX_WHOLE_PACKET
#endif
/* Mass storage transfer buffers (default 2 x 64 KiB does not fit the
 * arena). */
#define USB_READ_BUFFER_SIZE  (16*1024)
#define USB_WRITE_BUFFER_SIZE (16*1024)
/* They are allocated when the host configures us, before playback is told
 * to stop for the disk; while playing, the audio buffer refuses to shrink
 * below its reserve and the allocation panicked with OOM (usb_storage.c) */
#define USB_STORAGE_ALLOC_HARD
#endif
#define CONFIG_RTC RTC_SL6801
/* RDA5807 (0x5808) on I2C (fmradio-i2c-yp3.c) */
#define CONFIG_TUNER RDA5802
/* 12 MHz reference (CLK_MODE 001): fmradio-i2c-yp3.c */
#define RDA5802_POWERCFG_INIT   0xC411
/* Antenna on LNAP (LNA_PORT_SEL 10, LNA_ICSEL 10), keeping Rockbox's seek threshold and full volume. */
#define RDA5802_SYSCONFIG2_INIT 0x86AF
/* RDS/RBDS: rda5802.c polls the tuner from its own
 * thread; rds.c adds programme type, call letters, RT+ artist/title and a
 * confirmed clock time (HAVE_RDS_EXTENDED). */
#define HAVE_RDS_CAP
#define CONFIG_RDS (RDS_CFG_POLL | RDS_CFG_PROCESS)
#define CONFIG_RDS_POLL_TICKS 2 /* RDSR is high ~42 ms of each 88 ms group */
#define RDA5802_RDS
#define HAVE_RDS_EXTENDED
#ifndef SIMULATOR
#define TARGET_EXTRA_THREADS 1  /* RDS thread */
#endif
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)
#define HAVE_RECORDING          /* FM via the ADC capture (capture-yp3.c) */
#define REC_SAMPR_CAPS SAMPR_CAP_44
/* Pre-recording keeps encoded data in a ~60 KB buffer: ~3.7 s of MP3 at
 * 128 kbps, ~0.3 s of 44.1 kHz stereo WAV. Offering 30 s would promise
 * what the RAM cannot hold. */
#define REC_PRERECORD_MAX_SECONDS 5
#define REC_FREQ_DEFAULT REC_FREQ_44
#define REC_SAMPR_DEFAULT SAMPR_44
/* Recording in a heap of ~120 KiB at best: small PCM and stream buffers,
 * and recording refused (audio_recording_ready()) when a theme left too
 * little (recorder/pcm_record.c) */
#define PCMREC_LOWMEM
/* Bluetooth: the generic host stack (firmware/bluetooth/) on the on-chip
 * controller, target/arm/sl6801/yp3/bt-*-yp3.c. */
#define HAVE_BLUETOOTH
/* Long-lived pinned blocks (Bluetooth's) go to the heap's top, so they do
 * not split it when a short-lived block below is freed (core_alloc_top.h) */
#define HAVE_CORE_ALLOC_TOP
#ifndef SIMULATOR
/* The codec's audio PLL runs only while the built-in output is the sink
 * (or capture runs): pcm_sink_power() in pcm-yp3.c */
#define HAVE_PCM_SINK_POWER
#endif
/* A game controller or keyboard as buttons (firmware/bluetooth/hid.c), with
 * its own buttons (BUTTON_PAD_*) in the keymaps and plugins: undefine to
 * build without any of it. */
#define HAVE_BLUETOOTH_HID
#ifndef SIMULATOR
/* The RF calibration is reused from RAM or the card (bt_drv_cal_*) */
#define HAVE_BLUETOOTH_DRV_CAL
#endif
/* SBC: the portable encoder in IRAM (17% of a core at 192 MHz); the boot
 * ROM's float encoder (HAVE_BLUETOOTH_SBC_ENCODER, bt-sbc-yp3.c) needs no
 * SRAM but 53%, too much next to a codec. */
#define CONFIG_STORAGE STORAGE_SD
#define HAVE_FLASH_STORAGE
#define HAVE_FAT16SUPPORT
#define HAVE_SEMAPHORE_OBJECTS
#define INCLUDE_TIMEOUT_API
#ifdef YP3_DEV
#define YP3_DEV_LOG_BYTES 4096 /* dev-yp3.c log ring */
#else
#define YP3_DEV_LOG_BYTES 0
#endif
#if defined(YP3_RECOVERY_WATCHDOG) && \
    (!defined(YP3_DEV) || defined(YP3_DEV_WATCHDOG_POWEROFF))
/* Release: a recovery watchdog reset (hang, fault, panic) ends powered
 * off instead of in ROM USB (recovery-watchdog-yp3.c). Dev builds keep the
 * ROM USB return unless YP3_DEV_WATCHDOG_POWEROFF (to test this path with
 * the dev config's return=/crash=). */
#define YP3_WATCHDOG_POWEROFF
#define YP3_RUN_MARKER_BYTES 8 /* recovery-watchdog-yp3.c run marker */
#else
#define YP3_RUN_MARKER_BYTES 0
#endif
/* Top of SRAM, kept across watchdog resets: fault record 128 + dev log +
 * run marker. */
#define YP3_DIAG_RETAINED_BYTES \
    (128 + YP3_DEV_LOG_BYTES + YP3_RUN_MARKER_BYTES)
#define CODEC_SIZE (128 * 1024 - YP3_DIAG_RETAINED_BYTES)
#define PLUGIN_BUFFER_SIZE CODEC_SIZE
#define HAVE_SHARED_CODEC_PLUGIN_BUFFER
/* Codecs too big for the arena run from flash (XIP): code and rodata in the
 * codec flash area, data, bss and hot code/tables (ICODE_ATTR...) in the
 * arena, the rest of the arena is their heap. The core
 * links below the area; the dev sdpush bundle starts at its end. */
#ifndef SIMULATOR   /* the simulator loads them whole */
#define HAVE_XIP_CODECS
#define XIP_CODEC_AREA_START 0x00ce0000 /* flash offset 0x0e0000 */
#define XIP_CODEC_AREA_SIZE  0x00070000 /* to flash offset 0x150000 */
#endif
/* HE-AAC: the SBR decoder (config.h enables it only with a 512 KiB codec
 * area) with its state trimmed to fit the arena plus the playback memory
 * lent while it plays (libfaad FAAD_SBR_LOWMEM). No PS:
 * HE-AAC v2 plays as mono. */
#define CODEC_AAC_SBR_DEC
#define FAAD_SBR_LOWMEM
/* More codec memory trimmed for the arena, each explained where it is used:
 * Tremor's packed codebooks and small stream pages; Opus's byte-wise CRC
 * and one-page buffers; WMA's buffers on the heap and its integer LSP
 * curve; ffmpeg VLC setup with the caller's scratch buffer. */
#define TREMOR_LOWMEM
#define OPUS_LOWMEM
#define WMA_HEAP_BUFFERS
#define WMA_NO_SOFT_FLOAT
#define FFMPEG_VLC_LOWMEM
/* MAD's largest decode frame is over 7 KiB, before its callers. */
#define TARGET_CODEC_STACK_SIZE (12 * 1024)
/* Measured peaks (dev `mem`, MP3/FM/recording): buffering 1.1 KiB (metadata
 * parsers reach ~3 KiB), audio 1.8 KiB, scroll ~0.1 KiB. */
#define TARGET_BUFFERING_STACK_SIZE (4 * 1024)
#define TARGET_AUDIO_STACK_SIZE (3 * 1024)
#define TARGET_SCROLL_STACK_SIZE (2 * 1024)
#define HAVE_XIP_CODEPAGE_TABLES
/* English UI for the first build; metadata keeps all codepage conversions. */
#define HAVE_BUILTIN_LANGUAGE_ONLY
/* Disk-only database with reduced buffers. */
#define HAVE_TAGCACHE
#define TAGCACHE_LOWMEM
#define HAVE_NO_VOICE /* no voice files: compile out talk/voice (frees RAM) */
#define HAVE_NO_SOFTLOCK /* keymap has no key-lock action: hide its settings */
/* Cuesheet scratch is 73 KiB; exclude this option from the initial build. */
#define HAVE_MINIMAL_AUDIO_PROFILE
/* No pitch or speed control: the time-stretch stage and its buffers are
 * left out of the low-memory profile */
#define HAVE_NO_PITCHCONTROL
/* Backdrop images are not loaded (skins and the backdrop setting): each is
 * a whole 40 KiB frame of heap, and cabbiev2's two left playback without
 * its buffer (OOM panic). Skins keep their colours, fonts and icons. */
#define HAVE_NO_BACKDROP_FILES
/* No surround: its 31 KB delay buffer comes out of the playback buffer,
 * which then stayed shrunk (stutter until a reset) */
#define HAVE_NO_SURROUND
/* File lists in the small heap: names average 32 bytes here; 128 entries
 * were below an ordinary card's root ("Dir buffer is full" on every browse,
 * user 2026-10-21). */
#define AVERAGE_FILENAME_LENGTH     32
#define MAX_FILES_IN_DIR_DEFAULT    256
#define MAX_FILES_IN_DIR_MAX        512
#define MAX_FILES_IN_DIR_STEP       32
#define MAX_FILES_IN_PLAYLIST_MIN   100
#define MAX_FILES_IN_PLAYLIST_MAX   400
#define MAX_FILES_IN_PLAYLIST_STEP  100
/* Plugins and codecs must have exactly the core's API struct size, not
 * just no larger: a config change can add or drop #ifdef'd members in the
 * middle and shift every later entry, without an API version bump. Core
 * and modules ship together, so an older module is always stale. Modules
 * also carry a hash of the module API headers (apps/apps.make), which
 * catches layout changes of structs reached by pointer (mp3entry...). */
#define HAVE_EXACT_MODULE_API
/* playback.c: a seek on a track that is not loaded yet ends seek mode;
 * upstream leaves ff_rw_mode set, so PCM stays paused for good (torture
 * shuffle: a hold-seek then a skip, playing at 3801 ms with ff_rw 1) */
#define PLAYBACK_SEEK_UNLOADED_ENDS_FFRW
/* speex.c: seek by bisection; upstream's failed whenever its first guess
 * overshot, and elapsed then ran past the length (torture shuffle stuck) */
#define SPEEX_SEEK_BISECT
/* metadata/ogg.c: find the last page backwards from the end; upstream reads
 * the last 64 kB a sector at a time, ~600 ms per Ogg track start */
#define OGG_METADATA_SCAN_BACK
/* libtremor/sharedbook.c: codebook setup in the arena (1.5 KB of the Vorbis
 * heap): from XIP flash it took ~1.7 s of every Vorbis start */
#define VORBIS_BOOK_INIT_ICODE
/* pcm_mixer.c: a channel that starts as the 3 s of silence after the last
 * one runs out restarts the PCM; upstream's stops under it (a playback
 * stall at 0 ms that never ends, seen in torture runs) */
#define MIXER_IDLE_RESTART
#define MAX_FONT_SIZE 4000
/* Decoded PCM ring in 4 KiB chunks: with 8 KiB chunks a 32 KiB ring
 * dropped whole mixer frames (the codec may only write when a chunk plus its
 * request is free). Measured stalls it must ride out:
 * settings writes up to ~540 ms, track starts ~185 ms, skin renders. */
/* Two PCM rings: while the codec leaves room
 * (MP3 from flash, FLAC, WAV, TTA, ALAC, WavPack...: up to ~65 KB) the
 * ring is in the codec arena's top (0.33 s) and costs no heap; codecs that
 * need the whole arena (AAC, Vorbis, Opus) get the smaller one (0.19 s) in
 * the playback block. */
#define TARGET_PCM_BUFFER_SIZE (32 * 1024)
#define TARGET_ARENA_PCM_SIZE (56 * 1024)
/* Vorbis, Opus, HE-AAC and WMA run from flash with their hot code in the
 * arena and need more heap than its rest: while they play, the playback
 * block lends them this much (apps/playback.c) */
#define HAVE_CODEC_EXTRA_BUFFER
#define TARGET_CODEC_EXTRA_SIZE (32 * 1024)
#define TARGET_PCMBUF_CHUNK_SIZE 4096u
/* Decode boosted whenever the ring is not full (the CPU idles in WFI once
 * it is): boosting at half a ring (~90 ms) came too late, TTA underran
 * every frame at 64 MHz. */
#define TARGET_PCMBUF_WATERMARK TARGET_ARENA_PCM_SIZE
/* A codec that finds the ring full re-checks every tick, not every 50 ms
 * (a quarter of the ring): UI work that doesn't yield, e.g. drawing Now
 * Playing (~100-120 ms), can start right after. Entering Now Playing
 * underran 7 times in 8 with 50 ms, 2 in 8 with one tick (2026-10-18). */
#define CODEC_PCMBUF_FULL_WAIT 1
/* The ring outlasts ~280 ms of CPU-bound work in another thread (threads
 * are not preempted): directory scans yield per entry. */
#define HAVE_FS_SCAN_YIELD
/* Rockbox Info: OK (Select or Right) only redraws with the free count that
 * FSInfo gives at mount and allocations keep current. The forced recount
 * reads the whole FAT under the disk writer lock (~35 s on a 32 GB card),
 * which freezes the UI and starves playback buffering. */
#define HAVE_NO_FREE_RECOUNT
/* Buffering allocation including its guard: 40 KiB of file data (~2.5 s at
 * 128 kbps); SD DMA refills it in milliseconds. The guard (default 32 KiB)
 * also caps codec requests: a 24-bit stereo FLAC/ALAC frame of 4096 samples
 * fits; larger requests used to wait forever. */
#define TARGET_AUDIO_BUFFER_CAP (64 * 1024)
#define TARGET_GUARD_BUFSIZE (24 * 1024)
/* Smallest buffering size the playback buffer may shrink to when another
 * allocation needs heap (upstream 256 KiB, more than our whole buffer, so
 * it never shrank: USB storage's 32 KiB transfer buffer panicked with OOM
 * while playing with a theme loaded). Shrinking stops and resumes playback. */
#define TARGET_AUDIO_BUFFER_RESERVE (TARGET_GUARD_BUFSIZE + 8 * 1024)
/* Glyph cache per skin font (%Fl without a count; upstream uses the UI
 * font's 250, ~12 KB for a 16 px font). Track titles rarely use more
 * distinct glyphs. Lavenia (four skin fonts) gained 4.5 KB of heap. */
#define TARGET_SKIN_FONT_GLYPHS 48
/* With glyph caches this small a glyph read from the card (which sleeps) is
 * common: text output from the UI and scroll threads takes a lock (font.h) */
#define HAVE_FONT_TEXT_LOCK
#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE
/* The PMU charges by itself; power-yp3.c reports input/charging state. */
#define CONFIG_CHARGING CHARGING_MONITOR
/* Settings > System > Battery > Charge Limit (power-yp3.c) */
#define HAVE_CHARGE_LIMIT
/* On USB with charging off the battery rests: percent_to_volt_rest */
#define HAVE_BATTERY_REST_CURVE
/* PWM3 backlight (backlight-yp3.c): levels 1..10 = 10..100 % duty */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING 1
#define MAX_BRIGHTNESS_SETTING 10
#define DEFAULT_BRIGHTNESS_SETTING 3
#define BATTERY_CAPACITY_DEFAULT 300
#define BATTERY_CAPACITY_MIN 300
#define BATTERY_CAPACITY_MAX 300
#define BATTERY_CAPACITY_INC 0
#define BOOTFILE_EXT "yp3"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"
