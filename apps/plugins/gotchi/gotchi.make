GOTCHI_SRCDIR := $(APPSDIR)/plugins/gotchi
GOTCHI_BUILDDIR := $(BUILDDIR)/apps/plugins/gotchi
GOTCHI_SRC := $(call preprocess, $(GOTCHI_SRCDIR)/SOURCES)
GOTCHI_OBJ := $(call c2obj, $(GOTCHI_SRC))
OTHER_SRC += $(GOTCHI_SRC)
ROCKS += $(GOTCHI_BUILDDIR)/gotchi.rock
$(GOTCHI_BUILDDIR)/gotchi.rock: $(GOTCHI_OBJ)

# TamaLIB is written with unused callback arguments.
$(call c2obj, $(GOTCHI_SRCDIR)/tama/cpu.c $(GOTCHI_SRCDIR)/tama/hw.c): PLUGINFLAGS += -Wno-unused-parameter
