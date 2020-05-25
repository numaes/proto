# Warning: This makefile rampantly assumes it is being run in UNIX
#   Also, there may be some gnu tool chain assumptions: gcc and gmake?

# ***********
# * PREFACE *
# ***********------+
# | Subdirectories |
# +----------------+
# declare subdirectories of the source directory
SUBDIRECTORIES := core header

# make sure the subdirectories are mirrored in
# the obj/ debug/ and depend/ directories
# (HACK: use this dummy variable to get access to shell commands)
SHELL_HACK := $(shell mkdir -p bin lib include)
SHELL_HACK := $(shell mkdir -p obj debug depend)

# +----------+
# | Platform |
# +----------+-------------------
# | evaluates to 'Darwin' on mac
# | evaluates to ??? on ???
# +------------------------------
PLATFORM := $(shell uname)

-include makeConstants

# *********
# * TOOLS *
# *********--+
# | Programs |
# +----------+
CPP11_FLAGS := -std=c++11
CC  := gcc
CXX := g++
ifeq ($(PLATFORM),Darwin) # on mac
    CPP11_FLAGS := $(CPP11_FLAGS) -stdlib=libc++ -Wno-c++11-extensions
endif
RM  := rm
CP  := cp

# +--------------+
# | Option Flags |
# +--------------+
# Let the preprocessor know which platform we're on
ifeq ($(PLATFORM),Darwin)
  CONFIG  := $(CONFIG) -DMACOSX
else
  CONFIG  := $(CONFIG)
endif

# include paths (flattens the hierarchy for include directives)
INC       := -I headers/ 

# use the second line to disable profiling instrumentation
# PROFILING := -pg
PROFILING :=
CCFLAGS   := -Wall $(INC) $(CONFIG) -O2 -DNDEBUG $(PROFILING)
CXXFLAGS  := $(CCFLAGS) $(CPP11_FLAGS)
CCDFLAGS  := -Wall $(INC) $(CONFIG) -ggdb 
CXXDFLAGS := $(CCDFLAGS) $(CPP11_FLAGS)

LINK         := $(CXXFLAGS)
LINKD        := $(CXXDFLAGS)
ifeq ($(PLATFORM),Darwin)
  LINK  := $(LINK) -Wl,-no_pie
  LINKD := $(LINK) -Wl,-no_pie
endif


# ***********************
# * SOURCE DECLARATIONS *
# ***********************-----------------+
# | SRCS defines a generic bag of sources |
# +---------------------------------------+
# OTHER_SRCS   := xxx xxx xxx
SRCS         := \
    Cell IdentityDict ParentLink Proto ProtoByteBuffer \
	ProtoContext ProtoExternalPointer ProtoObjectCell ProtoSet \
	ProtoSpace ProtoList ProtoMethodCell Thread

# +-----------------------------------+
# | HEADERS defines headers to export |
# +-----------------------------------+
# OTHER_HEADERS := xxx.h xxx.h
HEADERS           := proto.h
HEADER_COPIES     := $(addprefix headers/,$(HEADERS))

# +-----------------------------+
# | stand alone program sources |
# +-----------------------------+
MAIN_SRC := test_proto

# +---------------------------------------+
# | all sources for dependency generation |
# +---------------------------------------+
ALL_SRCS := \
    $(SRCS) $(MAIN_SRC)

DEPENDS := $(addprefix depend/,$(addsuffix .d,$(SRCS)))

# +--------------------------------+
# | Object Aggregation for Targets |
# +--------------------------------+

OBJ               := $(addprefix obj/,$(addsuffix .o,$(SRCS)))
DEBUG             := $(addprefix debug/,$(addsuffix .o,$(SRCS)))
MAIN_OBJ          := $(addprefix obj/,$(addsuffix .o,$(MAIN_SRC)))
MAIN_DEBUG        := $(addprefix debug/,$(addsuffix .o,$(MAIN_SRC)))

LIB_TARGET_NAME   := proto

# *********
# * RULES *
# *********------+
# | Target Rules |
# +--------------+
all: lib/lib$(LIB_TARGET_NAME).a includes 
debug: lib/lib$(LIB_TARGET_NAME)-debug.a bin/test_proto includes

lib/lib$(LIB_TARGET_NAME).a: $(OBJ)
	@echo "Bundling $@"
	@ar rcs $@ $(OBJ)

lib/lib$(LIB_TARGET_NAME)-debug.a: $(DEBUG)
	@echo "Bundling debug $@"
	@ar rcs $@ $(DEBUG)

bin/test_proto: $(MAIN_DEBUG) headers/proto.h lib/lib$(LIB_TARGET_NAME)-debug.a
	@echo "Linking test_proto command line tool"
	@$(CXX) -o bin/test_proto -lpthread -pthread test/test_proto.cpp lib/lib$(LIB_TARGET_NAME)-debug.a $(LINKD)

# +------------------------------+
# | Specialized File Build Rules |
# +------------------------------+


# +------------------------------------+
# | Generic Source->Object Build Rules |
# +------------------------------------+
obj/%.o: $(addprefix core/,$(addsuffix .cpp,$(SRCS)))
	@echo "Compiling $@"
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

debug/%.o: $(addprefix core/,$(addsuffix .cpp,$(SRCS)))
	@echo "Compiling for debug $@"
	@$(CXX) $(CXXDFLAGS) -o $@ -c $<

# dependency file build rules
depend/%.d: $(addprefix core/,$(addsuffix .cpp,$(SRCS)))
	@$(CXX) $(CXXFLAGS) -MM $< | \
        sed -e 's@^\(.*\)\.o:@depend/$*.d debug/$*.o obj/$*.o:@' > $@

# +-------------------+
# | include copy rule |
# +-------------------+---------------------
# | This rule exists to safely propagate
# | header file dependencies to other
# | targets that depend on the common code
# +-----------------------------------------
includes: $(HEADER_COPIES)

include/%.h: src/%.h
	@echo "updating $@"
	@cp $< $@

# +---------------+
# | cleaning rule |
# +---------------+
clean:
	-@$(RM) -r obj depend debug include bin lib
	-@$(RM) bin/test_proto
	-@$(RM) lib/lib$(LIB_TARGET_NAME).a
	-@$(RM) lib/lib$(LIB_TARGET_NAME)-debug.a

-include $(DEPENDS)

