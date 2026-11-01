.SILENT:
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra 
TARGET   = keysprint
SOURCES  = main.cpp

all: $(TARGET) run

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) 

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
