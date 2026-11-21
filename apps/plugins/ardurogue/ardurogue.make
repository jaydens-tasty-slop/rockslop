ARDUROGUE_SRCDIR := $(APPSDIR)/plugins/ardurogue
ARDUROGUE_BUILDDIR := $(BUILDDIR)/apps/plugins/ardurogue
ARDUROGUE_SRC := $(call preprocess, $(ARDUROGUE_SRCDIR)/SOURCES)
ARDUROGUE_CPP := $(wildcard $(ARDUROGUE_SRCDIR)/src/*.cpp) $(ARDUROGUE_SRCDIR)/rockbox/glue.cpp
ARDUROGUE_OBJ := $(call c2obj, $(ARDUROGUE_SRC) $(ARDUROGUE_CPP))
OTHER_SRC += $(ARDUROGUE_SRC)
ROCKS += $(ARDUROGUE_BUILDDIR)/ardurogue.rock
$(ARDUROGUE_BUILDDIR)/ardurogue.rock: $(ARDUROGUE_OBJ)

# C++ objects need only libgcc (no STL, exceptions or RTTI).
ARDUROGUE_HEADERS := $(wildcard $(ARDUROGUE_SRCDIR)/src/*.hpp) $(wildcard $(ARDUROGUE_SRCDIR)/rockbox/*.hpp)
$(ARDUROGUE_BUILDDIR)/src/%.o: $(ARDUROGUE_SRCDIR)/src/%.cpp $(ARDUROGUE_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(@F))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init,$(PLUGINFLAGS)) \
	 -w -I$(ARDUROGUE_SRCDIR)/rockbox -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@
$(ARDUROGUE_BUILDDIR)/rockbox/%.o: $(ARDUROGUE_SRCDIR)/rockbox/%.cpp $(ARDUROGUE_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(@F))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init,$(PLUGINFLAGS)) \
	 -w -I$(ARDUROGUE_SRCDIR)/rockbox -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@
