#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

ROCKTRACKSRCDIR := $(APPSDIR)/plugins/rocktrack
ROCKTRACKBUILDDIR := $(BUILDDIR)/apps/plugins/rocktrack

ROCKS += $(ROCKTRACKBUILDDIR)/rocktrack.rock

ROCKTRACK_SRC := $(call preprocess, $(ROCKTRACKSRCDIR)/SOURCES)
ROCKTRACK_OBJ := $(call c2obj, $(ROCKTRACK_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(ROCKTRACK_SRC)

ROCKTRACKFLAGS = $(PLUGINFLAGS)

# the mixer runs in the audio interrupt: speed over size
$(ROCKTRACKBUILDDIR)/engine.o: ROCKTRACKFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(ROCKTRACKBUILDDIR)/rocktrack.rock: $(ROCKTRACK_OBJ)

$(ROCKTRACKBUILDDIR)/%.o: $(ROCKTRACKSRCDIR)/%.c $(ROCKTRACKSRCDIR)/rt.h $(ROCKTRACKSRCDIR)/rocktrack.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(ROCKTRACKFLAGS) -c $< -o $@
