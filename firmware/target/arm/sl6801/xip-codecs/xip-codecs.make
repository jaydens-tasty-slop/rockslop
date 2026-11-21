#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# XIP codecs (HAVE_XIP_CODECS): decoders linked to run from flash, built
# into $(CODECDIR)/xip/: <codec>.flash and codecs.flash, the image of the
# codec flash area with every shipped codec in its slot and a directory of
# them at its end (yp3.py writes it with the core; the loader needs no file
# on the card). Each codec's decoder library is rebuilt with CODEC_XIP,
# which turns on ICODE_ATTR & co. so upstream's IRAM selections land in the
# arena; assembly objects are the normal ones.

XIPCODEC_DIR := $(FIRMDIR)/target/arm/sl6801/xip-codecs
XIPCODECDIR := $(CODECDIR)/xip

# codec: flash slot (offset from XIP_CODEC_AREA_START, size), the decoder
# library rebuilt for XIP (its make variable prefix), other libraries
XIP_aac_SLOT := 0x00000 0x24000
XIP_aac_LIB := FAADLIB
XIP_aac_LIBS := $(M4ALIB)
XIP_vorbis_SLOT := 0x24000 0x16000
XIP_vorbis_LIB := TREMORLIB
XIP_vorbis_LIBS := $(TLSFLIB) $(SETJMPLIB)
XIP_opus_SLOT := 0x3a000 0x14000
XIP_opus_LIB := OPUSLIB
XIP_opus_LIBS := $(TLSFLIB)
XIP_mpa_SLOT := 0x4e000 0x11000
XIP_mpa_LIB := MADLIB
XIP_mpa_LIBS := $(ASFLIB)
XIP_wma_SLOT := 0x5f000 0x10000
XIP_wma_LIB := WMALIB
XIP_wma_LIBS := $(ASFLIB)
# mpa from flash leaves room for the PCM ring in the arena (62 KB instead
# of 105); vorbis and opus get playback memory lent while they play
# (HAVE_CODEC_EXTRA_BUFFER), and so
# does wma (its buffers from the heap, WMA_HEAP_BUFFERS in libwma). The
# area is full (0x6f000 of 0x70000): mpc lost its slot to wma. make
# xipcodecs-all builds them all.
XIP_CODEC_NAMES := aac mpa vorbis opus wma
XIP_CODEC_ALL := $(XIP_CODEC_NAMES)
xipcodecs-all: $(XIP_CODEC_ALL:%=$(XIPCODECDIR)/%.flash)
XIPCODECS := $(XIP_CODEC_NAMES:%=$(XIPCODECDIR)/%.flash)
XIPCODEC_FLASH := $(XIPCODECDIR)/codecs.flash

xipcodecs: $(XIPCODEC_FLASH)

$(XIPCODECDIR)/%.o: $(RBCODECLIB_DIR)/codecs/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC xip/$(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(CODECFLAGS) -DCODEC_XIP -MMD -MP -c $< -o $@

# lib<name>.a for XIP: C sources rebuilt, assembly from the normal build
define XIP_LIB_template
XIP_$(1)_A := $$(XIPCODECDIR)/$$(notdir $$($(2)))
XIP_$(1)_OBJ := $$(patsubst $$(RBCODECLIB_DIR)/codecs/%.c,$$(XIPCODECDIR)/%.o,$$(filter %.c,$$($(2)_SRC)))
XIP_OBJ += $$(XIP_$(1)_OBJ) $$(XIPCODECDIR)/$(1).o
$$(XIP_$(1)_A): $$(XIP_$(1)_OBJ) $$(call c2obj,$$(filter-out %.c,$$($(2)_SRC)))
	$$(SILENT)$$(shell rm -f $$@)
	$$(call PRINTS,AR xip/$$(@F))$$(AR) rcs $$@ $$^ >/dev/null
$$(XIPCODECDIR)/$(1).elf: $$(XIP_$(1)_A) $$(XIP_$(1)_LIBS)
$$(XIPCODECDIR)/$(1).link: XIP_SLOT_FLAGS := \
	-DXIP_SLOT_START="(XIP_CODEC_AREA_START+$$(word 1,$$(XIP_$(1)_SLOT)))" \
	-DXIP_SLOT_SIZE=$$(word 2,$$(XIP_$(1)_SLOT))
endef
$(foreach c,$(XIP_CODEC_ALL),$(eval $(call XIP_LIB_template,$(c),$(XIP_$(c)_LIB))))
# their header dependencies (-MMD): the normal build's covers only its own
# objects
-include $(XIP_OBJ:.o=.d)

# the normal build's per-library flags
$(XIPCODECDIR)/libfaad.a: CODECFLAGS += -O2
$(XIPCODECDIR)/libwma.a: CODECFLAGS += -Wno-shift-negative-value
$(XIPCODECDIR)/libmad.a: CODECFLAGS += -I$(RBCODECLIB_DIR)/codecs/libmad \
	-UDEBUG -DNDEBUG -DHAVE_LIMITS_H -DHAVE_ASSERT_H
$(XIPCODECDIR)/libopus.a: CODECFLAGS := -DHAVE_CONFIG_H \
	-I$(RBCODECLIB_DIR)/codecs/libopus \
	-I$(RBCODECLIB_DIR)/codecs/libopus/celt \
	-I$(RBCODECLIB_DIR)/codecs/libopus/silk \
	$(CODECFLAGS)

$(XIPCODECDIR)/%.link: $(XIPCODEC_DIR)/%.lds $(XIPCODEC_DIR)/codec-xip.lds $(CONFIGFILE) \
		$(XIPCODEC_DIR)/xip-codecs.make \
		$(MODULE_BUILD_ID_LDS)
	$(call PRINTS,PP xip/$(@F))
	$(SILENT)mkdir -p $(dir $@)
	$(call preprocess2file,$<,$@,-DCODEC -DCODEC_XIP $(XIP_SLOT_FLAGS))

# libraries in link order: codec's own, then the common ones
$(XIPCODECDIR)/%.elf: $(XIPCODECDIR)/%.o $(CODEC_CRT0) $(XIPCODECDIR)/%.link $(CODEC_LIBS)
	$(call PRINTS,LD xip/$(@F))$(CC) $(CODECFLAGS) -o $@ \
		$(filter %.o, $^) \
		$(filter-out $(CODEC_LIBS),$(filter %.a, $^)) $(CODEC_LIBS) \
		-lgcc -T$(XIPCODECDIR)/$*.link -Wl,--gc-sections \
		-Wl,$(LDMAP_OPT),$(XIPCODECDIR)/$*.map $(GLOBAL_LDOPTS)

$(XIPCODECDIR)/%.flash: $(XIPCODECDIR)/%.elf $(XIPCODEC_DIR)/xip-codec.py
	$(call PRINTS,XIP $(@F))python3 $(XIPCODEC_DIR)/xip-codec.py split \
		$(OC) $< $@

$(XIPCODEC_FLASH): $(XIPCODECS) $(XIPCODEC_DIR)/xip-codec.py
	$(call PRINTS,XIP $(@F))python3 $(XIPCODEC_DIR)/xip-codec.py pack \
		$@ $(XIPCODECS)
