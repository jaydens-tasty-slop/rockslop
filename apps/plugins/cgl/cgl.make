CGL_SRCDIR := $(APPSDIR)/plugins/cgl
CGL_BUILDDIR := $(BUILDDIR)/apps/plugins/cgl
CGL_SRC := $(call preprocess, $(CGL_SRCDIR)/SOURCES)
OTHER_SRC += $(CGL_SRC)

# One plugin per game (backend + library + that game), so each fits the
# plugin arena, plus a launcher that lists them.
CGL_COMMON := $(CGL_SRCDIR)/rockbox/cgl.c $(CGL_SRCDIR)/rockbox/cgl_audio.c \
 $(CGL_SRCDIR)/rockbox/cgl_math.c \
 $(CGL_SRCDIR)/lib/cglp.c $(CGL_SRCDIR)/lib/menu.c \
 $(CGL_SRCDIR)/lib/menuGameList.c $(CGL_SRCDIR)/lib/particle.c \
 $(CGL_SRCDIR)/lib/random.c $(CGL_SRCDIR)/lib/sound.c \
 $(CGL_SRCDIR)/lib/textPattern.c $(CGL_SRCDIR)/lib/vector.c
CGL_COMMON_OBJ := $(call c2obj, $(CGL_COMMON))
CGL_GAMES := BallTour:balltour CastN:castn ColorRoll:colorroll Fracave:fracave Froooog:froooog GrapplingH:grapplingh Hexmin:hexmin InTow:intow LadderDrop:ladderdrop PakuPaku:pakupaku PinClimb:pinclimb RWheel:rwheel Reflector:reflector Survivor:survivor Thunder:thunder TimberTest:timbertest

ROCKS += $(CGL_BUILDDIR)/cgl.rock
$(CGL_BUILDDIR)/cgl.rock: $(call c2obj, $(CGL_SRCDIR)/rockbox/launcher.c)

define CGL_GAME_RULE
ROCKS += $$(CGL_BUILDDIR)/cgl_$(2).rock
$$(CGL_BUILDDIR)/cgl_$(2).rock: $$(CGL_COMMON_OBJ) $$(call c2obj, $$(CGL_SRCDIR)/games/game$(1).c)
endef
$(foreach g,$(CGL_GAMES),$(eval $(call CGL_GAME_RULE,$(firstword $(subst :, ,$(g))),$(lastword $(subst :, ,$(g))))))

# The games are written with double literals; keep the soft-float math single.
$(call c2obj, $(CGL_SRC)): PLUGINFLAGS += -fsingle-precision-constant

# YP3: everything but the synth (mixer callback, interrupt context) and the
# launcher runs on the FPU (lib/plugin_fpu.h; yp3dev/fpucheck.py
# cgl_<game>.elf audio_more checks the callback stays soft-float).
ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
$(call c2obj, $(filter-out %/rockbox/cgl_audio.c %/rockbox/launcher.c,$(CGL_SRC))): \
    PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU
endif
