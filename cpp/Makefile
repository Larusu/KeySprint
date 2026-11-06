.SILENT:
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations
TARGET   = keysprint
SOURCES  = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp
HEADERS  = game.h terminal.h scoring.h render.h

all: $(TARGET) run

.PHONY: all run clean
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) 

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
