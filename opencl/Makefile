.SUFFIXES:

CXX := g++

CXXFLAGS := -g -pipe -O2 -Wall \
	-I. `sdl2-config --cflags` \
	-std=c++0x
CXXFLAGS += -Wpointer-arith -Wfloat-equal -g3 -ffor-scope -I. -I"/opt/AMDAPP/include"

TEST_BIN := test
TEST_LIBS := `sdl2-config --libs`
TEST_LIBS += -L/usr/lib/fglrx \
	-L/opt/AMDAPP/lib/x86_64/ \
	-L/usr/X11R6/lib \
	-lpthread -ldl -lglut  -lGLEW  -lGL -lOpenCL
TEST_OBJ = \
	main.o \
	sdl_context.o \
	random.o

ALL_BIN := $(TEST_BIN)
ALL_OBJ := $(TEST_OBJ)
ALL_DEP := $(patsubst %.o, .%.d, $(ALL_OBJ))
ALL_TARGETS := $(ALL_BIN)

TARGET: all

.PHONY: all clean

all: $(ALL_BIN)

ifeq ($(filter clean, $(MAKECMDGOALS)),clean)
CLEAN_DEP := clean
else
CLEAN_DEP :=
endif

%.o %.d: %.cpp $(CLEAN_DEP) $(CONFIG_MAK) Makefile
	@echo " [C] $<"
	@$(CXX) $(CXXFLAGS) -MMD -MF $(patsubst %.o, .%.d, $@) \
		-MT $(patsubst .%.d, %.o, $@) \
		-c -o $(patsubst .%.d, %.o, $@) $<

$(TEST_BIN): $(TEST_OBJ)
	@echo " [LINK] $@"
	@$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ) $(TEST_LIBS)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJ) $(ALL_DEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif
