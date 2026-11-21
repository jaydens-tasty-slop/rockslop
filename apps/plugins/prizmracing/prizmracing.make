#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# PRIZM Racing.

PRIZMRACING_SRCDIR := $(APPSDIR)/plugins/prizmracing
PRIZMRACING_BUILDDIR := $(BUILDDIR)/apps/plugins/prizmracing
PRIZMRACING_SRC := $(call preprocess, $(PRIZMRACING_SRCDIR)/SOURCES)
PRIZMRACING_OBJ := $(call c2obj, $(PRIZMRACING_SRC))
OTHER_SRC += $(PRIZMRACING_SRC)
ROCKS += $(PRIZMRACING_SRC:$(PRIZMRACING_SRCDIR)/%.c=$(PRIZMRACING_BUILDDIR)/%.rock)

$(foreach c,$(notdir $(PRIZMRACING_SRC:.c=)),$(eval $(PRIZMRACING_BUILDDIR)/$(c).rock: $(PRIZMRACING_BUILDDIR)/$(c).o))
