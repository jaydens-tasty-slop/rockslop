#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Flipper Zero source-compatibility layer plus one .rock per ported game.

FLIPPER_SRCDIR := $(APPSDIR)/plugins/flipper
FLIPPER_BUILDDIR := $(BUILDDIR)/apps/plugins/flipper

FLIPPER_CORE_SRC := $(addprefix $(FLIPPER_SRCDIR)/,flipper_core.c flipper_gui.c \
    flipper_fonts.c flipper_storage.c flipper_string.c flipper_notify.c)
FLIPPER_CORE_OBJ := $(call c2obj, $(FLIPPER_CORE_SRC))

# flipper_game NAME, SOURCES (relative to the flipper directory)
define flipper_game
FLIPPER_$(1)_SRC := $$(addprefix $$(FLIPPER_SRCDIR)/,$(2))
FLIPPER_$(1)_OBJ := $$(call c2obj, $$(FLIPPER_$(1)_SRC))
OTHER_SRC += $$(FLIPPER_$(1)_SRC)
ROCKS += $$(FLIPPER_BUILDDIR)/$(1).rock
$$(FLIPPER_BUILDDIR)/$(1).rock: $$(FLIPPER_$(1)_OBJ) $$(FLIPPER_CORE_OBJ)
endef

OTHER_SRC += $(FLIPPER_CORE_SRC)
OTHER_INC += -I$(FLIPPER_SRCDIR) -I$(FLIPPER_SRCDIR)/include

# C-only warning switches: cc1plus warns about them, so the C++ rules drop them.
FLIPPER_C_ONLY_FLAGS := -Wno-missing-prototypes -Wno-strict-prototypes
FLIPPER_CFLAGS := -I$(FLIPPER_SRCDIR)/include -I$(FLIPPER_SRCDIR) \
    -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough \
    $(FLIPPER_C_ONLY_FLAGS)

$(FLIPPER_BUILDDIR)/%.o: PLUGINFLAGS += $(FLIPPER_CFLAGS)

# C++ games (no exceptions, RTTI or libstdc++); needs the toolchain's cc1plus.
$(FLIPPER_BUILDDIR)/%.o: $(FLIPPER_SRCDIR)/%.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(ROOTDIR)/,,$<))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init $(FLIPPER_C_ONLY_FLAGS),$(PLUGINFLAGS)) \
	 -std=gnu++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@

$(FLIPPER_BUILDDIR)/%.o: $(FLIPPER_SRCDIR)/%.cxx
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(ROOTDIR)/,,$<))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init $(FLIPPER_C_ONLY_FLAGS),$(PLUGINFLAGS)) \
	 -std=gnu++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@

$(eval $(call flipper_game,asteroids,asteroids/app.c asteroids/glue.c))
$(eval $(call flipper_game,mystic,mystic/main.cpp mystic/render.cpp mystic/mystic_balloon.cpp mystic/glue.c flipper_cxx.cpp))
OTHER_INC += -I$(FLIPPER_SRCDIR)/mystic
$(FLIPPER_BUILDDIR)/mystic/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/mystic


# YP3: games and the layer run on the FPU (lib/plugin_fpu.h). Nothing here
# runs in interrupt context; timers are serviced from the main thread's pump
# and no game starts a furi thread (a thread would have to be soft-float).
ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
$(FLIPPER_BUILDDIR)/%.o: PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU
# Their float maths is written with double literals; keep it single.
$(FLIPPER_BUILDDIR)/asteroids/%.o: PLUGINFLAGS += -fsingle-precision-constant
endif

$(eval $(call flipper_game,citybloxx,citybloxx/citybloxx.c citybloxx/blocks.c \
    citybloxx/manipulations.c citybloxx/glue.c))

$(eval $(call flipper_game,stackattack,stackattack/stack_attack.c stackattack/game.c \
    stackattack/render.c stackattack/glue.c))

$(eval $(call flipper_game,nwcrawl,nwcrawl/nw_crawl.c nwcrawl/glue.c))

$(eval $(call flipper_game,furiousbirds,furiousbirds/furious_birds.c \
    furiousbirds/furious_birds_icons.c furiousbirds/glue.c flipper_views.c))
$(FLIPPER_BUILDDIR)/furiousbirds/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/furiousbirds

$(eval $(call flipper_game,nupogodi,nupogodi/nupogodi.c nupogodi/nupogodi_icons.c \
    nupogodi/glue.c flipper_views.c))
$(FLIPPER_BUILDDIR)/nupogodi/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/nupogodi

$(eval $(call flipper_game,fliprogue,$(addprefix fliprogue/,$(filter-out glue.c,$(notdir $(wildcard $(FLIPPER_SRCDIR)/fliprogue/*.c)))) \
    fliprogue/glue.c))

$(eval $(call flipper_game,racegame,racegame/race_game.c racegame/glue.c))

$(eval $(call flipper_game,arkanoid,arkanoid/arkanoid_game.c arkanoid/glue.c))

$(eval $(call flipper_game,stackgame,stackgame/stack.c stackgame/glue.c))

$(eval $(call flipper_game,bomberduck,bomberduck/bomberduck.c bomberduck/bomberduck_icons.c \
    bomberduck/glue.c))
$(FLIPPER_BUILDDIR)/bomberduck/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/bomberduck

$(eval $(call flipper_game,simonsays,simonsays/simon_says.c simonsays/simon_says_icons.c \
    simonsays/glue.c))
$(FLIPPER_BUILDDIR)/simonsays/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/simonsays

$(eval $(call flipper_game,oregontrail,$(addprefix oregontrail/,$(filter-out glue.c,$(notdir $(wildcard $(FLIPPER_SRCDIR)/oregontrail/*.c)))) \
    oregontrail/glue.c))

$(eval $(call flipper_game,flipperhero,flipperhero/flipper_hero.c flipperhero/flipper_hero_icons.c \
    flipperhero/helpers/doom_music_player_worker.c flipperhero/helpers/storage.c \
    flipperhero/helpers/stratagem_data.c flipperhero/view/arrows.c flipperhero/view/draw.c \
    flipperhero/glue.c))
$(FLIPPER_BUILDDIR)/flipperhero/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/flipperhero \
    -I$(FLIPPER_SRCDIR)/flipperhero/helpers -I$(FLIPPER_SRCDIR)/flipperhero/view \
    -I$(FLIPPER_SRCDIR)/flipperhero/data

$(eval $(call flipper_game,flappybird,flappybird/flappy_bird.c flappybird/flappy_bird_icons.c \
    flappybird/glue.c))
$(FLIPPER_BUILDDIR)/flappybird/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/flappybird

$(eval $(call flipper_game,trexrunner,trexrunner/trexrunner.c trexrunner/t_rex_runner_icons.c \
    trexrunner/glue.c))

$(eval $(call flipper_game,scorchedtanks,scorchedtanks/scorched_tanks_game_app.c scorchedtanks/glue.c))

$(eval $(call flipper_game,flightassault,flightassault/flight_assault.c flightassault/glue.c))

$(eval $(call flipper_game,paperplane,paperplane/paperplane.c paperplane/game_state.c \
    paperplane/map.c paperplane/paper.c paperplane/paper_plane_icons.c paperplane/glue.c))

$(eval $(call flipper_game,wavegame,wavegame/main.c wavegame/font_rus.c wavegame/glue.c))

$(eval $(call flipper_game,applegrabber,applegrabber/apple_grabber.c \
    applegrabber/apple_grabber_icons.c applegrabber/glue.c))
$(FLIPPER_BUILDDIR)/applegrabber/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/applegrabber

$(eval $(call flipper_game,drifter,drifter/drifter.c drifter/glue.c))

$(eval $(call flipper_game,ringbounce,ringbounce/bounce.c ringbounce/glue.c))

$(eval $(call flipper_game,hanoi,hanoi/flipper_game_hanoi_towers.c hanoi/glue.c))
$(eval $(call flipper_game,wires,wires/flipper_game_connect_wires.c wires/glue.c))

$(eval $(call flipper_game,ironwyrm,ironwyrm/ironwyrm.c ironwyrm/renderer.c ironwyrm/core/combat.c \
    ironwyrm/core/data.c ironwyrm/core/game.c ironwyrm/core/generation.c ironwyrm/core/intro.c \
    ironwyrm/core/plate.c ironwyrm/core/random.c ironwyrm/core/wyrm.c ironwyrm/ironwyrm_icons.c ironwyrm/glue.c))
$(FLIPPER_BUILDDIR)/ironwyrm/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/ironwyrm

$(eval $(call flipper_game,rps,rps/pixel_rps.c rps/pixel_rps_icons.c flipper_views.c rps/glue.c))
$(FLIPPER_BUILDDIR)/rps/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/rps

$(eval $(call flipper_game,reaction,reaction/reaction.c reaction/glue.c))

$(eval $(call flipper_game,etch,etch/etch_a_sketch.c etch/etch_icons.c etch/glue.c))
$(FLIPPER_BUILDDIR)/etch/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/etch

$(eval $(call flipper_game,fourinrow,fourinrow/fourinrow.c fourinrow/glue.c))
$(eval $(call flipper_game,fighterjet,fighterjet/fighterjet.c fighterjet/graphics.c fighterjet/autopilot.c fighterjet/glue.c))
$(FLIPPER_BUILDDIR)/fighterjet/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/fighterjet
$(eval $(call flipper_game,fzsudoku,fzsudoku/fzsudoku.c fzsudoku/glue.c))
$(FLIPPER_BUILDDIR)/fzsudoku/%.o: PLUGINFLAGS += -I$(FLIPPER_SRCDIR)/fzsudoku
