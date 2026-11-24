#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# SAF (Small Abstract Fish) games: one SAF frontend, one .rock per game.

SAF_SRCDIR := $(APPSDIR)/plugins/saf
SAF_BUILDDIR := $(BUILDDIR)/apps/plugins/saf
SAF_SRC := $(call preprocess, $(SAF_SRCDIR)/SOURCES)
SAF_OBJ := $(call c2obj, $(SAF_SRC))
OTHER_SRC += $(SAF_SRC)
ROCKS += $(SAF_SRC:$(SAF_SRCDIR)/%.c=$(SAF_BUILDDIR)/%.rock)

$(foreach c,$(notdir $(SAF_SRC:.c=)),$(eval $(SAF_BUILDDIR)/$(c).rock: $(SAF_BUILDDIR)/$(c).o))
