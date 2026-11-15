.SILENT:

BUILD = build

ifeq ($(OS),Windows_NT)
    EXE  = .exe
    JOBS = $(NUMBER_OF_PROCESSORS)
    ifdef MSYSTEM                       # MSYS2 / Git Bash
        MKDIR = mkdir -p $(BUILD)
        RMDIR = rm -rf $(BUILD)
        RUN   = ./$(BUILD)/keysprint$(EXE)
        TEST_RUN = ./$(BUILD)/core-tests$(EXE)
    else                                # plain cmd
        MKDIR = if not exist $(BUILD) mkdir $(BUILD)
        RMDIR = if exist $(BUILD) rmdir /s /q $(BUILD)
        RUN   = $(BUILD)\keysprint$(EXE)
        TEST_RUN = $(BUILD)\core-tests$(EXE)
    endif
else
    EXE  =
    JOBS = $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
    MKDIR = mkdir -p $(BUILD)
    RMDIR = rm -rf $(BUILD)
    RUN   = ./$(BUILD)/keysprint
    TEST_RUN = ./$(BUILD)/core-tests
endif

MAKEFLAGS += -j$(JOBS)

TARGET      = $(BUILD)/keysprint$(EXE)
TEST_TARGET = $(BUILD)/core-tests$(EXE)

CXX      ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations -pipe
CPPFLAGS += -Iinclude -MMD -MP

SOURCES      = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp word_bank.cpp
OBJECTS      = $(SOURCES:%.cpp=$(BUILD)/%.o)
LIB_OBJECTS  = $(filter-out $(BUILD)/main.o,$(OBJECTS))
TEST_OBJECT  = $(BUILD)/core_tests.o
DEPS         = $(OBJECTS:.o=.d) $(TEST_OBJECT:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TEST_TARGET): $(TEST_OBJECT) $(LIB_OBJECTS)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJECT): tests/core_tests.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	$(MKDIR)

run: $(TARGET)
	$(RUN)

test: $(TEST_TARGET)
	$(TEST_RUN)

clean:
	-$(RMDIR)

-include $(DEPS)

.PHONY: all run clean test
