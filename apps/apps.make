#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

INCLUDES += -I$(APPSDIR) $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
SRC += $(call preprocess, $(APPSDIR)/SOURCES)

# apps/features.txt is a file that (is preprocessed and) lists named features
# based on defines in the config-*.h files. The named features will be passed
# to genlang and thus (translated) phrases can be used based on those names.
# button.h is included for the HAS_BUTTON_HOLD define.
#
# Kludge: depends on config.o which only depends on config-*.h to have config.h
# changes trigger a genlang re-run
#

ifneq (,$(USE_LTO))
$(BUILDDIR)/apps/features: PPCFLAGS += -DUSE_LTO
endif

$(BUILDDIR)/apps/features: $(APPSDIR)/features.txt  $(BUILDDIR)/firmware/common/config.o
	$(SILENT)mkdir -p $(BUILDDIR)/apps
	$(SILENT)mkdir -p $(BUILDDIR)/lang
	$(call PRINTS,PP $(<F))
	$(SILENT)$(CC) $(PPCFLAGS) \
                 -E -P -imacros "config.h" -imacros "button.h" -x c $< | \
		grep -v "^#" | grep -v "^ *$$" > $(BUILDDIR)/apps/features; \

$(BUILDDIR)/apps/genlang-features:  $(BUILDDIR)/apps/features
	$(call PRINTS,GEN $(subst $(BUILDDIR)/,,$@))tr \\n : < $< > $@

ifeq ($(MODELNAME),yp3)
# Modules reach core structs (mp3entry, user_settings, ...) by pointer, so a
# layout change need not change the API size. With HAVE_EXACT_MODULE_API the
# core and every module link in a hash of the core's view of the module API
# (plugin.h, which includes codecs.h); the loaders reject other values. Only
# rewritten when the hash changes, so modules relink only then.
MODULE_BUILD_ID_LDS := $(BUILDDIR)/module_build_id.lds
$(MODULE_BUILD_ID_LDS): $(BUILDDIR)/apps/plugin.o
	$(call preprocess2file,$(APPSDIR)/plugin.h,$@.i,)
	$(SILENT)echo "module_build_id = 0x$$(cksum < $@.i | \
		awk '{printf "%08x", $$1}');" > $@_; rm $@.i
	$(SILENT)if cmp -s $@_ $@; then rm $@_; \
		else echo "GEN $(@F)"; mv $@_ $@; fi
$(BUILDDIR)/ram.link $(BUILDDIR)/apps/plugins/plugin.link \
$(BUILDDIR)/apps/plugins/overlay_ref.link \
$(BUILDDIR)/lib/rbcodec/codecs/codec.link: $(MODULE_BUILD_ID_LDS)
CLEANOBJS += $(MODULE_BUILD_ID_LDS)
endif

ASMDEFS_SRC += $(APPSDIR)/core_asmdefs.c
