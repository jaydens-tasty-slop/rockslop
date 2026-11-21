TANKS_SRCDIR := $(APPSDIR)/plugins/tanks
TANKS_BUILDDIR := $(BUILDDIR)/apps/plugins/tanks
TANKS_SRC := $(call preprocess, $(TANKS_SRCDIR)/SOURCES)
TANKS_CPP := $(wildcard $(TANKS_SRCDIR)/engine/*.cpp)
TANKS_OBJ := $(call c2obj, $(TANKS_SRC) $(TANKS_CPP))
OTHER_SRC += $(TANKS_SRC)
ROCKS += $(TANKS_BUILDDIR)/tanks.rock
$(TANKS_BUILDDIR)/tanks.rock: $(TANKS_OBJ)

# C++ objects need only libgcc (no STL, exceptions or RTTI).
TANKS_HEADERS := $(wildcard $(TANKS_SRCDIR)/engine/*.h)
$(TANKS_BUILDDIR)/engine/%.o: $(TANKS_SRCDIR)/engine/%.cpp $(TANKS_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(@F))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init,$(PLUGINFLAGS)) \
	 -w -I$(TANKS_SRCDIR)/engine -std=gnu++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@
