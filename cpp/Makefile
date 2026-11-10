.SILENT:

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations -Iinclude
TARGET   = keysprint
SOURCES  = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp word_bank.cpp
HEADERS  = game.h terminal.h scoring.h render.h config.h word_bank.h include/nlohmann/json.hpp

all: $(TARGET) run

.PHONY: all run clean

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
