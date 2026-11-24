#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# Demos and toys ported from small embedded projects: one .rock per demo.

FXDEMOS_SRCDIR := $(APPSDIR)/plugins/fxdemos
FXDEMOS_BUILDDIR := $(BUILDDIR)/apps/plugins/fxdemos
FXDEMOS_SRC := $(call preprocess, $(FXDEMOS_SRCDIR)/SOURCES)
FXDEMOS_OBJ := $(call c2obj, $(FXDEMOS_SRC))
OTHER_SRC += $(FXDEMOS_SRC)
ROCKS += $(FXDEMOS_SRC:$(FXDEMOS_SRCDIR)/%.c=$(FXDEMOS_BUILDDIR)/%.rock)

$(foreach c,$(notdir $(FXDEMOS_SRC:.c=)),$(eval $(FXDEMOS_BUILDDIR)/$(c).rock: $(FXDEMOS_BUILDDIR)/$(c).o))
