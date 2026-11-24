#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

VPSRCDIR := $(APPSDIR)/plugins/videoplayer
VPBUILDDIR := $(BUILDDIR)/apps/plugins/videoplayer

ROCKS += $(VPBUILDDIR)/videoplayer.rock

VP_SRC := $(call preprocess, $(VPSRCDIR)/SOURCES)
VP_OBJ := $(call c2obj, $(VP_SRC))

# mpegplayer's libmpeg2, the C version, with the quantizer tables of 4:2:0
# only (setup_frames() takes no other) and a chunk buffer for one slice:
# encoders write a slice per macroblock row (ffmpeg's are under 3 KB at
# 160x120 and q=1); a longer one is skipped
VP_MPEG2DIR := $(APPSDIR)/plugins/mpegplayer/libmpeg2
VP_MPEG2_SRC := $(addprefix $(VP_MPEG2DIR)/, decode.c header.c idct.c \
                motion_comp.c motion_comp_c.c slice.c)
VP_MPEG2_OBJ := $(patsubst $(VP_MPEG2DIR)/%.c, $(VPBUILDDIR)/libmpeg2/%.o, \
                $(VP_MPEG2_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(VP_SRC)

$(VPBUILDDIR)/videoplayer.rock: $(VP_OBJ) $(VP_MPEG2_OBJ)

# small, but the inner loops of the decoders fast
VPFLAGS = $(filter-out -O%,$(PLUGINFLAGS))
VPOPT = -Os
$(addprefix $(VPBUILDDIR)/, mjpeg.o mpeg.o mpa.o video.o sound.o \
    libmpeg2/idct.o libmpeg2/motion_comp_c.o): VPOPT = -O2

$(VPBUILDDIR)/%.o: $(VPSRCDIR)/%.c $(VPSRCDIR)/videoplayer.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) \
		$(VPFLAGS) $(VPOPT) -c $< -o $@

# (not in OTHER_SRC, so its headers are named here)
$(VPBUILDDIR)/libmpeg2/%.o: $(VP_MPEG2DIR)/%.c $(VPSRCDIR)/videoplayer.make \
                            $(wildcard $(VP_MPEG2DIR)/*.h) $(APPSDIR)/plugin.h \
                            $(BUILDDIR)/sysfont.h $(BUILDDIR)/lang_enum.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(VPFLAGS) $(VPOPT) \
		-fgnu89-inline -DMPEG2_CHUNK_SIZE=8192 -DMPEG2_420_ONLY -c $< -o $@
