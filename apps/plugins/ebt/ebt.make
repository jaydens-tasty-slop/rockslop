EBT_SRCDIR := $(APPSDIR)/plugins/ebt
EBT_BUILDDIR := $(BUILDDIR)/apps/plugins/ebt
EBT_SRC := $(call preprocess, $(EBT_SRCDIR)/SOURCES)
EBT_CPP := $(EBT_SRCDIR)/rockbox/ebt_core.cpp
EBT_OBJ := $(call c2obj, $(EBT_SRC) $(EBT_CPP))
OTHER_SRC += $(EBT_SRC)
ROCKS += $(EBT_BUILDDIR)/ebt.rock
$(EBT_BUILDDIR)/ebt.rock: $(EBT_OBJ)

# C++ objects need only libgcc (no STL, exceptions or RTTI).
EBT_HEADERS := $(wildcard $(EBT_SRCDIR)/upstream/*.h) $(wildcard $(EBT_SRCDIR)/rockbox/*.h)
$(EBT_BUILDDIR)/rockbox/%.o: $(EBT_SRCDIR)/rockbox/%.cpp $(EBT_HEADERS)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CXX $(@F))$(CC) -x c++ $(filter-out -std=% -Wstrict-prototypes -Wno-pointer-sign -Wmissing-prototypes -Wno-override-init,$(PLUGINFLAGS)) \
	 -w -I$(EBT_SRCDIR)/rockbox -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	 -fno-use-cxa-atexit -c $< -o $@
