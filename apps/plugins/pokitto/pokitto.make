#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# PokittoLib compatibility layer (lib/) plus one pokitto_<game>.rock per
# game. The layer is linked into every rock. C++ is compiled freestanding
# (no exceptions, RTTI or libstdc++), as in celeste.make and arduboy.make.
# Large sound data lives on the card: tools/<game>_data.py extracts it from
# the upstream repository (POKITTO_UPSTREAM) into the build dir, and
# CARDFILES installs it.

POKITTO_SRCDIR := $(APPSDIR)/plugins/pokitto
POKITTO_BUILDDIR := $(BUILDDIR)/apps/plugins/pokitto
POKITTO_UPSTREAM ?= $(ROOTDIR)/../build/np/src

POKITTO_LIB_C := $(POKITTO_SRCDIR)/lib/pk_glue.c
POKITTO_LIB_CPP := $(addprefix $(POKITTO_SRCDIR)/lib/, pk_core.cpp \
	pk_data.cpp pk_libc.cpp)
POKITTO_LIB_OBJ := $(call c2obj, $(POKITTO_LIB_C))

# Games: name -> C++ sources (relative to pokitto/<name>/)
POKITTO_obs_SRC := main.cpp obs_rb.cpp src/OBS.cpp src/OBS_Boss.cpp \
	src/OBS_Enemy.cpp src/OBS_Explode.cpp src/OBS_Game.cpp \
	src/OBS_SplashScreen.cpp src/OBS_Title.cpp src/OBS_Utils.cpp
POKITTO_obs_UPSTREAM := OBS_Pokitto

POKITTO_fireworks_SRC := main.cpp fw_rb.cpp $(addprefix src/, FireWorks.cpp \
	FireWorks_Explode.cpp FireWorks_Game.cpp FireWorks_Instructions.cpp \
	FireWorks_Render.cpp FireWorks_ShowOver.cpp FireWorks_ShowTarget.cpp \
	FireWorks_SplashScreen.cpp FireWorks_TargetAchieved.cpp \
	FireWorks_Title.cpp FireWorks_Utils.cpp)
POKITTO_fireworks_UPSTREAM := FireWorks_Pokitto
POKITTO_fireworks_LIBS := pk_font_koubit.cpp

POKITTO_darkritual_SRC := main.cpp dr_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/darkritual/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/darkritual/src/*.cpp \
	$(POKITTO_SRCDIR)/darkritual/src/entities/*.cpp))
POKITTO_darkritual_UPSTREAM := DarkRitual_Pokitto
POKITTO_darkritual_LIBS := pk_run.cpp pk_font_koubit.cpp pk_font_3x5.cpp

POKITTO_spacecab_SRC := main.cpp sc_rb.cpp $(patsubst $(POKITTO_SRCDIR)/spacecab/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/spacecab/src/*.cpp))
POKITTO_spacecab_UPSTREAM := SpaceCab_Pokitto
POKITTO_spacecab_LIBS := pk_font_3x5.cpp

POKITTO_nvader_SRC := main.cpp nv_rb.cpp $(patsubst $(POKITTO_SRCDIR)/nvader/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/nvader/src/*.cpp))
POKITTO_nvader_UPSTREAM := 1nvader_Pokitto

POKITTO_minirogue_SRC := main.cpp mr_rb.cpp $(patsubst $(POKITTO_SRCDIR)/minirogue/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/minirogue/src/*.cpp))
POKITTO_minirogue_UPSTREAM := MiniRogue_Pokitto
POKITTO_minirogue_LIBS := pk_font_3x5.cpp

POKITTO_astarok_SRC := main.cpp as_rb.cpp $(patsubst $(POKITTO_SRCDIR)/astarok/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/astarok/src/*.cpp $(POKITTO_SRCDIR)/astarok/src/entities/*.cpp))
POKITTO_astarok_UPSTREAM := TrialsOfAstarok_Pokitto
POKITTO_astarok_LIBS := pk_font_3x5.cpp

POKITTO_minesweeper_SRC := main.cpp ms_rb.cpp $(patsubst $(POKITTO_SRCDIR)/minesweeper/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/minesweeper/src/*.cpp $(POKITTO_SRCDIR)/minesweeper/src/*/*.cpp))
POKITTO_minesweeper_UPSTREAM := MineSweeper_Pokitto

POKITTO_lightsout_SRC := main.cpp lo_rb.cpp $(patsubst $(POKITTO_SRCDIR)/lightsout/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/lightsout/src/*.cpp $(POKITTO_SRCDIR)/lightsout/src/*/*.cpp))
POKITTO_lightsout_UPSTREAM := LightsOut_Pokitto
POKITTO_lightsout_LIBS := pk_font_5x7.cpp

POKITTO_buttonstrail_SRC := main.cpp bt_rb.cpp $(patsubst $(POKITTO_SRCDIR)/buttonstrail/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/buttonstrail/src/*.cpp))
POKITTO_buttonstrail_UPSTREAM := ButtonsTrail_Pokitto
POKITTO_buttonstrail_LIBS := pk_font_5x7.cpp

POKITTO_ballbust_SRC := main.cpp bb_rb.cpp
POKITTO_ballbust_UPSTREAM :=

POKITTO_galaxyfighter_SRC := main.cpp gf_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/galaxyfighter/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/galaxyfighter/src/*.cpp $(POKITTO_SRCDIR)/galaxyfighter/src/*/*.cpp))
POKITTO_galaxyfighter_UPSTREAM := GalaxyFighter_Pokitto
POKITTO_galaxyfighter_LIBS := pk_font_c64.cpp

POKITTO_linez_SRC := lz_rb.cpp $(patsubst $(POKITTO_SRCDIR)/linez/%,%,$(wildcard $(POKITTO_SRCDIR)/linez/*.cpp))
POKITTO_linez_UPSTREAM :=
POKITTO_linez_LIBS := pk_font_3x5.cpp pk_font_koubit.cpp

POKITTO_leword_SRC := main.cpp lw_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/leword/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/leword/src/*.cpp))
POKITTO_leword_UPSTREAM := LeWord_Pokitto

POKITTO_scrambler_SRC := main.cpp sc2_rb.cpp $(patsubst $(POKITTO_SRCDIR)/scrambler/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/scrambler/src/*.cpp))
POKITTO_scrambler_UPSTREAM := Scrambler_Pokitto
POKITTO_scrambler_LIBS := pk_font_5x7.cpp

POKITTO_roadtrip_SRC := main.cpp rt_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/roadtrip/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/roadtrip/src/*.cpp))
POKITTO_roadtrip_UPSTREAM := RoadTrip_Pokitto
POKITTO_roadtrip_LIBS := pk_font_tight.cpp

POKITTO_pzero_SRC := $(patsubst $(POKITTO_SRCDIR)/pzero/%,%,$(wildcard $(POKITTO_SRCDIR)/pzero/*.cpp))
POKITTO_pzero_CSRC := fixmath/fix16.c fixmath/fix16_sqrt.c fixmath/fix16_trig.c
POKITTO_pzero_UPSTREAM :=
POKITTO_pzero_LIBS := pk_font_3x5.cpp

POKITTO_abbaye_SRC := $(patsubst $(POKITTO_SRCDIR)/abbaye/%,%,$(wildcard $(POKITTO_SRCDIR)/abbaye/*.cpp))
POKITTO_abbaye_UPSTREAM := Abbaye_Pokitto
POKITTO_abbaye_LIBS := pk_font_adventurer.cpp

POKITTO_scopa_SRC := main.cpp sc_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/scopa/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/scopa/src/*.cpp))
POKITTO_scopa_UPSTREAM := Scopa_V1_Pokitto
POKITTO_scopa_LIBS := pk_font_c64.cpp

POKITTO_rogue_SRC := main.cpp rg_rb.cpp $(patsubst $(POKITTO_SRCDIR)/rogue/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/rogue/src/*.cpp))
POKITTO_rogue_UPSTREAM := Rogue_Pokitto
POKITTO_rogue_LIBS := pk_font_5x7.cpp

POKITTO_stackshift_SRC := main.cpp ss_rb.cpp src/images/images_rt.cpp $(patsubst $(POKITTO_SRCDIR)/stackshift/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/stackshift/src/*.cpp))
POKITTO_stackshift_UPSTREAM := StackShift_Pokitto
POKITTO_stackshift_LIBS := pk_font_c64.cpp

POKITTO_noughts_SRC := main.cpp nc_rb.cpp
POKITTO_noughts_UPSTREAM := SimpleNoughtsAndCrosses
POKITTO_noughts_LIBS := pk_font_5x7.cpp

# No float maths, and its image is within 1.5 KiB of the arena: stays soft-float.
POKITTO_SOFT_GAMES := darkritual

POKITTO_picross_SRC := main.cpp pc_rb.cpp $(patsubst $(POKITTO_SRCDIR)/picross/%,%,\
	$(wildcard $(POKITTO_SRCDIR)/picross/src/*.cpp $(POKITTO_SRCDIR)/picross/src/*/*.cpp))
POKITTO_picross_UPSTREAM := PiCross_Pokitto
POKITTO_picross_LIBS := pk_font_5x7.cpp

POKITTO_GAMES := obs fireworks darkritual spacecab pzero abbaye nvader minirogue astarok minesweeper lightsout buttonstrail ballbust galaxyfighter linez leword scrambler roadtrip picross scopa rogue stackshift noughts

POKITTO_HEADERS := $(wildcard $(POKITTO_SRCDIR)/lib/*.h $(POKITTO_SRCDIR)/lib/LibAudio $(POKITTO_SRCDIR)/lib/LibSchedule \
	$(POKITTO_SRCDIR)/*/*.h $(POKITTO_SRCDIR)/*/src/*.h \
	$(POKITTO_SRCDIR)/*/src/*/*.h $(POKITTO_SRCDIR)/*/src/*/*/*.h)

OTHER_SRC += $(POKITTO_LIB_C)
POKITTO_ROCKS := $(foreach g,$(POKITTO_GAMES),$(POKITTO_BUILDDIR)/pokitto_$(g).rock)
ROCKS += $(POKITTO_ROCKS)

# YP3: pk_glue.c stays soft-float (mixer callback, interrupt context) but
# switches FPU access on for the game (lib/plugin_fpu.h).
ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
$(call c2obj, $(POKITTO_LIB_C)): PLUGINFLAGS += -DPLUGIN_FPU
endif

define POKITTO_GAME_RULE
# The layer is built per game: the screen mode comes from the game's My_settings.h.
POKITTO_$(1)_LIBOBJ := $$(addprefix $$(POKITTO_BUILDDIR)/$(1)/lib/, $$(notdir $$(POKITTO_LIB_CPP:.cpp=.o)) $$(POKITTO_$(1)_LIBS:.cpp=.o))
$$(POKITTO_BUILDDIR)/$(1)/lib/%.o: $$(POKITTO_SRCDIR)/lib/%.cpp $$(POKITTO_HEADERS)
	$$(SILENT)mkdir -p $$(dir $$@)
	$$(call PRINTS,CXX $(1)/lib/$$(<F))$$(CC) -x c++ $$(POKITTO_CXXFLAGS) \
	 -I$$(POKITTO_SRCDIR)/$(1) -c $$< -o $$@
POKITTO_$(1)_OBJ := $$(call c2obj, $$(addprefix $$(POKITTO_SRCDIR)/$(1)/,$$(POKITTO_$(1)_SRC)))
POKITTO_$(1)_COBJ := $$(call c2obj, $$(addprefix $$(POKITTO_SRCDIR)/$(1)/,$$(POKITTO_$(1)_CSRC)))
OTHER_SRC += $$(addprefix $$(POKITTO_SRCDIR)/$(1)/,$$(POKITTO_$(1)_CSRC))
ifneq (,$$(and $$(findstring YP3_SL6801,$$(TARGET)),$$(findstring -mcpu=cortex-m4,$$(GCCOPTS)),$$(filter-out $$(POKITTO_SOFT_GAMES),$(1))))
# YP3: the game and layer run on the FPU (lib/plugin_fpu.h); only pk_glue.c,
# which holds the mixer callback (interrupt context), stays soft-float.
$$(POKITTO_$(1)_OBJ) $$(POKITTO_$(1)_COBJ) $$(POKITTO_$(1)_LIBOBJ): \
	PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU
endif
$$(POKITTO_BUILDDIR)/pokitto_$(1).rock: $$(POKITTO_$(1)_OBJ) $$(POKITTO_$(1)_COBJ) $$(POKITTO_LIB_OBJ) $$(POKITTO_$(1)_LIBOBJ) \
		$$(POKITTO_BUILDDIR)/$(1)-data.stamp
	$$(call PRINTS,LD $$(@F))$$(CC) $$(PLUGINFLAGS) -o $$(POKITTO_BUILDDIR)/pokitto_$(1).elf \
		$$(filter %.o, $$^) $$(filter %.a, $$+) \
		-lgcc $$(filter-out -Wl%.map, $$(PLUGINLDFLAGS)) -Wl,$$(LDMAP_OPT),$$(POKITTO_BUILDDIR)/pokitto_$(1).map
	$$(SILENT)$$(call objcopy_plugin,$$(POKITTO_BUILDDIR)/pokitto_$(1).elf,$$@)

# Card data; a missing upstream checkout only skips the extraction.
$$(POKITTO_BUILDDIR)/$(1)-data.stamp: $$(wildcard $$(POKITTO_SRCDIR)/tools/$(1)_data.py)
	$$(SILENT)mkdir -p $$(POKITTO_BUILDDIR)/$(1)-data
	$$(SILENT)if [ ! -f $$(POKITTO_SRCDIR)/tools/$(1)_data.py ]; then :; \
	elif [ -d $$(POKITTO_UPSTREAM)/$$(POKITTO_$(1)_UPSTREAM) ]; then \
	  python3 -I $$(POKITTO_SRCDIR)/tools/$(1)_data.py $$(POKITTO_UPSTREAM)/$$(POKITTO_$(1)_UPSTREAM) $$(POKITTO_BUILDDIR)/$(1)-data/; \
	else echo "pokitto: no upstream $$(POKITTO_$(1)_UPSTREAM), $(1) data not extracted (tools/fetch_upstream.sh)"; fi
	$$(SILENT)touch $$@
endef
$(foreach g,$(POKITTO_GAMES),$(eval $(call POKITTO_GAME_RULE,$(g))))
# Abbaye's float maths is written with double literals; keep it single.
$(POKITTO_abbaye_OBJ): PLUGINFLAGS += -fsingle-precision-constant

POKITTO_CXXFLAGS = $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign \
	-Wmissing-prototypes -Wno-override-init -Wold-style-declaration,$(PLUGINFLAGS)) \
	-std=gnu++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	-fno-use-cxa-atexit -w -Wno-narrowing -I$(POKITTO_SRCDIR)/lib -I$(POKITTO_SRCDIR)/pzero/fixmath

$(POKITTO_BUILDDIR)/%.o: $(POKITTO_SRCDIR)/%.cpp $(POKITTO_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(subst $(POKITTO_SRCDIR)/,,$<))$(CC) -x c++ $(POKITTO_CXXFLAGS) \
	 -I$(POKITTO_SRCDIR)/$(firstword $(subst /, ,$(subst $(POKITTO_SRCDIR)/,,$<))) -c $< -o $@

# The C glue is compiled by Rockbox's own rule; the header is shared.
$(POKITTO_BUILDDIR)/lib/pk_glue.o: $(POKITTO_HEADERS)
