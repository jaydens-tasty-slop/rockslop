#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# Terrario (Terraria rewrite, KBD2) with a Rockbox platform layer.

TERRARIO_SRCDIR := $(APPSDIR)/plugins/terrario
TERRARIO_BUILDDIR := $(BUILDDIR)/apps/plugins/terrario
TERRARIO_SRC := $(call preprocess, $(TERRARIO_SRCDIR)/SOURCES)
TERRARIO_OBJ := $(call c2obj, $(TERRARIO_SRC))
OTHER_SRC += $(TERRARIO_SRC)
OTHER_INC += -I$(TERRARIO_SRCDIR)/include -I$(TERRARIO_SRCDIR)/rb -I$(TERRARIO_SRCDIR)
ROCKS += $(TERRARIO_BUILDDIR)/terrario.rock
$(TERRARIO_BUILDDIR)/terrario.rock: $(TERRARIO_OBJ)

# Upstream code: quiet the warnings the Rockbox flags add.
$(TERRARIO_OBJ): PLUGINFLAGS += -I$(TERRARIO_SRCDIR)/include -I$(TERRARIO_SRCDIR)/rb \
	-I$(TERRARIO_SRCDIR) -w -fno-strict-aliasing

# YP3: the whole game runs on the FPU (lib/plugin_fpu.h). It has no mixer or
# timer callbacks and no extra threads. Upstream writes float maths with
# double literals; keep it single.
ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
$(TERRARIO_OBJ): PLUGINFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -DPLUGIN_FPU \
	-fsingle-precision-constant
endif
