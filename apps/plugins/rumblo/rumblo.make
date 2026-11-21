RUMBLO_SRCDIR := $(APPSDIR)/plugins/rumblo
RUMBLO_BUILDDIR := $(BUILDDIR)/apps/plugins/rumblo
RUMBLO_SRC := $(call preprocess, $(RUMBLO_SRCDIR)/SOURCES)
RUMBLO_OBJ := $(call c2obj, $(RUMBLO_SRC))
OTHER_SRC += $(RUMBLO_SRC)
ROCKS += $(RUMBLO_BUILDDIR)/rumblo.rock
$(RUMBLO_BUILDDIR)/rumblo.rock: $(RUMBLO_OBJ)

# The generated artwork tables are flat initialisers.
$(call c2obj, $(RUMBLO_SRCDIR)/rockbox/assets.c): PLUGINFLAGS += -Wno-missing-braces
