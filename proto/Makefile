# Warning: This makefile rampantly assumes it is being run in UNIX
#   Also, there may be some gnu tool chain assumptions: gcc and gmake?

# ***********
# * PREFACE *
# ***********------+
# | Subdirectories |
# +----------------+
# declare subdirectories of the source directory
SUBDIRECTORIES := core headers

# make sure the subdirectories are mirrored in
# the obj/ and debug/ directories
DIRS := bin lib include obj debug
# use this dummy variable to get access to shell commands
SHELL_HACK := $(shell mkdir -p $(DIRS))

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
CXX_STANDARD_FLAGS := -std=c++20
CC  := g++
CXX := g++
ifeq ($(PLATFORM),Darwin) # on mac
    CXX_STANDARD_FLAGS := $(CXX_STANDARD_FLAGS) -stdlib=libc++ -Wno-c++11-extensions
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
CXXFLAGS  := $(CCFLAGS) $(CXX_STANDARD_FLAGS) -MMD -MP
CCDFLAGS  := $(INC) $(CONFIG) -ggdb 
CXXDFLAGS := $(CCDFLAGS) $(CXX_STANDARD_FLAGS) -MMD -MP

LINK         := -Wall $(INC) $(CONFIG) -O2 -DNDEBUG $(PROFILING)
LINKD        := -Wall $(INC) $(CONFIG) -ggdb 
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
    Cell ProtoList ProtoSparseList ParentLink ProtoTuple ProtoString Proto ProtoByteBuffer \
	ProtoContext ProtoExternalPointer ProtoObjectCell \
	ProtoSpace ProtoMethodCell Thread 

# +-----------------------------------+
# | HEADERS defines headers to export |
# +-----------------------------------+
# OTHER_HEADERS := xxx.h xxx.h
HEADERS           := proto.h proto_internal.h ProtoTuple.h
HEADER_COPIES     := $(addprefix include/,$(HEADERS))

# +-----------------------------+
# | stand alone program sources |
# +-----------------------------+
MAIN_SRC := test_proto



# +--------------------------------+
# | Object Aggregation for Targets |
# +--------------------------------+
FULL_SRC		  := $(addprefix core/,$(addsuffix .cpp,$(SRCS)))	
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

debug/test_proto.o: test/test_proto.cpp
	@echo "Compiling for debug $@"
	@$(CXX) $(CXXDFLAGS) -o $@ -c $<

bin/test_proto: $(MAIN_DEBUG) headers/proto.h lib/lib$(LIB_TARGET_NAME)-debug.a
	@echo "Linking test_proto command line tool"
	@$(CXX) -o bin/test_proto -lpthread -pthread $(LINKD) $(MAIN_DEBUG) lib/lib$(LIB_TARGET_NAME)-debug.a

test: bin/test_proto
	@echo "Running test suite"
	@./bin/test_proto

# +------------------------------+
# | Specialized File Build Rules |
# +------------------------------+


# +------------------------------------+
# | Generic Source->Object Build Rules |
# +------------------------------------+
obj/%.o: core/%.cpp
	@echo "Compiling $@"
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

debug/%.o: core/%.cpp
	@echo "Compiling for debug $@"
	@$(CXX) $(CXXDFLAGS) -o $@ -c $<



# +-------------------+
# | include copy rule |
# +-------------------+
includes: $(HEADER_COPIES)

include/%.h: headers/%.h
	@echo "updating header $@"
	@cp $< $@

-include $(OBJ:.o=.d) $(DEBUG:.o=.d) debug/test_proto.d

# +---------------+
# | cleaning rule |
# +---------------+
clean:
	-@$(RM) -r obj debug include bin lib



