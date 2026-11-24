CELESTE_SRCDIR := $(APPSDIR)/plugins/celeste
CELESTE_BUILDDIR := $(BUILDDIR)/apps/plugins/celeste
CELESTE_SRC := $(call preprocess, $(CELESTE_SRCDIR)/SOURCES)
CELESTE_CPP := $(CELESTE_SRCDIR)/rockbox/engine.cpp $(CELESTE_SRCDIR)/rockbox/graphics.cpp
CELESTE_OBJ := $(call c2obj, $(CELESTE_SRC) $(CELESTE_CPP))
OTHER_SRC += $(CELESTE_SRC)
ROCKS += $(CELESTE_BUILDDIR)/celeste.rock
$(CELESTE_BUILDDIR)/celeste.rock: $(CELESTE_OBJ)

# Rockbox's C link rule is retained; these objects require only libgcc.
# The configured compiler must have its C++ frontend (cc1plus) installed.
CELESTE_HEADERS := $(wildcard $(CELESTE_SRCDIR)/rockbox/*.h) \
 $(wildcard $(CELESTE_SRCDIR)/rockbox/*.hpp) $(wildcard $(CELESTE_SRCDIR)/src/*.h)
$(CELESTE_BUILDDIR)/rockbox/%.o: $(CELESTE_SRCDIR)/rockbox/%.cpp $(CELESTE_HEADERS) $(CELESTE_SRCDIR)/src/classic.cpp
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(@F))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init,$(PLUGINFLAGS)) \
	 -std=gnu++11 -DCE_ROCKBOX -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -ffreestanding -fno-builtin -c $< -o $@
