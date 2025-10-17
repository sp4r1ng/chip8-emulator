APP := chip8
SRC := src/chip8.cpp src/platform_sdl.cpp src/main.cpp
OBJ := $(SRC:.cpp=.o)
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -pipe
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL2_LIBS   := $(shell pkg-config --libs sdl2)

.PHONY: all clean run debug

all: $(APP)

$(APP): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(SDL2_LIBS)

src/%.o: src/%.cpp src/%.h
	$(CXX) $(CXXFLAGS) $(SDL2_CFLAGS) -c $< -o $@

src/main.o: src/main.cpp src/chip8.h src/platform_sdl.h
	$(CXX) $(CXXFLAGS) $(SDL2_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(APP)

run: $(APP)
	./$(APP) roms/PONG --scale 12 --hz 700

debug: CXXFLAGS := -std=c++17 -O0 -g3 -Wall -Wextra -Wpedantic
debug: clean all
