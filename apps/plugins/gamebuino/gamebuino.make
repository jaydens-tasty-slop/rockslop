#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Gamebuino META compatibility layer (lib/) plus one .rock per game. Each game
# directory holds a single translation unit <game>/<game>.cpp that includes the
# sketch's files; the layer is linked into every rock. C++ is compiled
# freestanding (no exceptions, RTTI or libstdc++), as in celeste.make.

GAMEBUINO_SRCDIR := $(APPSDIR)/plugins/gamebuino
GAMEBUINO_BUILDDIR := $(BUILDDIR)/apps/plugins/gamebuino

GAMEBUINO_LIB_C := $(GAMEBUINO_SRCDIR)/lib/gb_glue.c
GAMEBUINO_LIB_CPP := $(GAMEBUINO_SRCDIR)/lib/gb.cpp $(GAMEBUINO_SRCDIR)/lib/gb_fmath.cpp
GAMEBUINO_LIB_OBJ := $(call c2obj, $(GAMEBUINO_LIB_C) $(GAMEBUINO_LIB_CPP))

# Games: name -> C++ sources (relative to gamebuino/<name>/)
GAMEBUINO_metawarfare_SRC := metawarfare.cpp
GAMEBUINO_glitch_SRC := glitch.cpp Backpack.cpp Character.cpp Cinematic.cpp \
	GameController.cpp Object.cpp Sfx.cpp Space.cpp StackContainer.cpp \
	View.cpp soundAssets.cpp

GAMEBUINO_parachute_SRC := parachute.cpp

GAMEBUINO_octopus_SRC := octopus.cpp

GAMEBUINO_gbsokoban_SRC := gbsokoban.cpp

GAMEBUINO_gbminesweeper_SRC := gbminesweeper.cpp

GAMEBUINO_gbpong_SRC := gbpong.cpp

GAMEBUINO_sweetvalentine_SRC := sweetvalentine.cpp Application.cpp Background.cpp \
	Ball.cpp GameEngine.cpp HeartBrick.cpp Paddle.cpp Renderable.cpp Renderer.cpp \
	SplashScreen.cpp SplashText.cpp Sprite.cpp TinyHeart.cpp UserController.cpp

GAMEBUINO_apollo_SRC := apollo.cpp src/engine/Apollo.cpp src/engine/Camera.cpp \
	src/engine/Game.cpp src/engine/Map.cpp src/engine/Scrap.cpp src/hardware/Pot.cpp \
	src/sound/Sound.cpp

GAMEBUINO_gruniozerca_SRC := gruniozerca.cpp

GAMEBUINO_uforace_SRC := uforace.cpp

GAMEBUINO_metarace_SRC := metarace.cpp

GAMEBUINO_supercrate_SRC := supercrate.cpp

GAMEBUINO_waternet_SRC := waternet.cpp $(addprefix src/,commonvars.cpp cursor.cpp game.cpp \
	helperfuncs.cpp helpscreens.cpp intro.cpp level.cpp levelscleared.cpp levelselect.cpp \
	palettedefines.cpp palettes.cpp printfuncs.cpp savestate.cpp sound.cpp \
	titlescreen.cpp)

GAMEBUINO_gbjezzball_SRC := jezzball.cpp ball.cpp board.cpp cursor.cpp line.cpp

GAMEBUINO_gbbangbang_SRC := gbbangbang.cpp bullet.cpp cannon.cpp terrain.cpp

GAMEBUINO_gbsolitaire_SRC := gbsolitaire.cpp card.cpp pile.cpp undo.cpp

GAMEBUINO_GAMES := metawarfare glitch parachute octopus gbsokoban gbminesweeper gbpong sweetvalentine apollo gruniozerca uforace metarace supercrate waternet gbjezzball gbbangbang gbsolitaire

GAMEBUINO_HEADERS := $(wildcard $(GAMEBUINO_SRCDIR)/lib/*.h \
	$(GAMEBUINO_SRCDIR)/*/*.h $(GAMEBUINO_SRCDIR)/*/*/*.h \
	$(GAMEBUINO_SRCDIR)/*/*.ino)

OTHER_SRC += $(GAMEBUINO_LIB_C)
ROCKS += $(foreach g,$(GAMEBUINO_GAMES),$(GAMEBUINO_BUILDDIR)/$(g).rock)

ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
$(call c2obj, $(GAMEBUINO_SRCDIR)/lib/gb_fmath.cpp): \
	PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU
# gb_glue.c stays soft-float (mixer callback) but switches FPU access on.
$(call c2obj, $(GAMEBUINO_SRCDIR)/lib/gb_glue.c): PLUGINFLAGS += -DPLUGIN_FPU
endif

define GAMEBUINO_GAME_RULE
ifneq (,$$(and $$(findstring YP3_SL6801,$$(TARGET)),$$(findstring -mcpu=cortex-m4,$$(GCCOPTS))))
# YP3: the game runs on the FPU (lib/plugin_fpu.h); the layer (lib/), which
# holds the mixer callback and the synth it calls, stays soft-float.
$$(call c2obj, $$(addprefix $$(GAMEBUINO_SRCDIR)/$(1)/,$$(GAMEBUINO_$(1)_SRC))): \
	PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU
endif
$$(GAMEBUINO_BUILDDIR)/$(1).rock: $$(call c2obj, $$(addprefix $$(GAMEBUINO_SRCDIR)/$(1)/,$$(GAMEBUINO_$(1)_SRC))) $$(GAMEBUINO_LIB_OBJ)
	$$(call PRINTS,LD $$(@F))$$(CC) $$(PLUGINFLAGS) -o $$(GAMEBUINO_BUILDDIR)/$(1).elf \
		$$(filter %.o, $$^) $$(filter %.a, $$+) \
		-lgcc $$(filter-out -Wl%.map, $$(PLUGINLDFLAGS)) -Wl,$$(LDMAP_OPT),$$(GAMEBUINO_BUILDDIR)/$(1).map
	$$(SILENT)$$(call objcopy_plugin,$$(GAMEBUINO_BUILDDIR)/$(1).elf,$$@)
endef
$(foreach g,$(GAMEBUINO_GAMES),$(eval $(call GAMEBUINO_GAME_RULE,$(g))))

# Glitch's float maths is written with double literals; keep it single.
$(call c2obj, $(addprefix $(GAMEBUINO_SRCDIR)/glitch/,$(GAMEBUINO_glitch_SRC))): PLUGINFLAGS += -fsingle-precision-constant

GAMEBUINO_CXXFLAGS = $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign \
	-Wmissing-prototypes -Wno-override-init -Wold-style-declaration,$(PLUGINFLAGS)) \
	-std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	-fno-use-cxa-atexit -fpermissive -Wno-narrowing -fwrapv -w \
	-I$(GAMEBUINO_SRCDIR)/lib

$(GAMEBUINO_BUILDDIR)/%.o: $(GAMEBUINO_SRCDIR)/%.cpp $(GAMEBUINO_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(GAMEBUINO_SRCDIR)/,,$<))$(CC) -x c++ $(GAMEBUINO_CXXFLAGS) \
	 -I$(dir $<) -c $< -o $@

# Rockbox's C rule compiles gb_glue.c; its header is shared with the C++ code.
$(GAMEBUINO_BUILDDIR)/lib/gb_glue.o: $(GAMEBUINO_HEADERS)
