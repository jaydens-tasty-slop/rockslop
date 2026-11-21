#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

ROCKGPTSRCDIR := $(APPSDIR)/plugins/rockgpt
ROCKGPTBUILDDIR := $(BUILDDIR)/apps/plugins/rockgpt

ROCKS += $(ROCKGPTBUILDDIR)/rockgpt.rock

ROCKGPT_SRC := $(call preprocess, $(ROCKGPTSRCDIR)/SOURCES)
ROCKGPT_OBJ := $(call c2obj, $(ROCKGPT_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(ROCKGPT_SRC)

# the inference loops want speed over size
ROCKGPTFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

# The engine runs on the FPU (lib/plugin_fpu.h): only llm.o is built for it,
# through the soft-float-compatible softfp ABI.
ifneq (,$(and $(findstring YP3_SL6801,$(TARGET)),$(findstring -mcpu=cortex-m4,$(GCCOPTS))))
ROCKGPTFLAGS += -DPLUGIN_FPU
$(ROCKGPTBUILDDIR)/llm.o: ROCKGPTFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16
endif

$(ROCKGPTBUILDDIR)/rockgpt.rock: $(ROCKGPT_OBJ)

$(ROCKGPTBUILDDIR)/%.o: $(ROCKGPTSRCDIR)/%.c $(ROCKGPTSRCDIR)/rockgpt.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(ROCKGPTFLAGS) -c $< -o $@
