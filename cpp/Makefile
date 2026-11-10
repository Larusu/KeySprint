.SILENT:

ifeq ($(OS),Windows_NT) # set for cmd and MSYS2
    TARGET = keysprint.exe
    RUN    = $(TARGET)
    RM     = del /f /q
else
    TARGET = keysprint
    RUN    = ./$(TARGET)
    RM     = rm -f
endif

CXX      ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations
SOURCES  = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp
HEADERS  = game.h terminal.h scoring.h render.h config.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) 

run: $(TARGET)
	$(RUN)

clean:
	$(RM) $(TARGET)

.PHONY: all run clean
