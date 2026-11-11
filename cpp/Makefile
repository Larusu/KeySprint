.SILENT:

ifeq ($(OS),Windows_NT) # set for cmd and MSYS2
    TARGET = keysprint.exe
    RUN    = $(TARGET)
    RM     = del /f /q
    TEST_TARGET = core-tests.exe
    TEST_RUN = $(TEST_TARGET)
else
    TARGET = keysprint
    RUN    = ./$(TARGET)
    RM     = rm -f
    TEST_TARGET = core-tests
    TEST_RUN = ./$(TEST_TARGET)
endif

CXX      ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations
CPPFLAGS += -Iinclude
SOURCES  = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp word_bank.cpp
HEADERS  = game.h terminal.h scoring.h render.h config.h word_bank.h include/nlohmann/json.hpp

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

run: $(TARGET)
	$(RUN)

clean:
	$(RM) $(TARGET) $(TEST_TARGET)

$(TEST_TARGET): tests/core_tests.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/core_tests.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp word_bank.cpp $(LDFLAGS) $(LDLIBS) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	$(TEST_RUN)

.PHONY: all run clean test
