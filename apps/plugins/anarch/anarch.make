ANARCH_SRCDIR := $(APPSDIR)/plugins/anarch
ANARCH_BUILDDIR := $(BUILDDIR)/apps/plugins/anarch
ANARCH_SRC := $(call preprocess, $(ANARCH_SRCDIR)/SOURCES)
ANARCH_OBJ := $(call c2obj, $(ANARCH_SRC))
OTHER_SRC += $(ANARCH_SRC)
ROCKS += $(ANARCH_BUILDDIR)/anarch.rock
$(ANARCH_BUILDDIR)/anarch.rock: $(ANARCH_OBJ)
