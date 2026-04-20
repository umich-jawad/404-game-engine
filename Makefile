CXX = clang++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Wpedantic
INCLUDES = -Isrc/firstparty -Isrc/thirdparty -Isrc/thirdparty/box2d/src -Isrc $(shell sdl2-config --cflags)
CC = cc
CFLAGS = -O3
LIBRARIES = $(shell sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lSDL2_ttf
BOX2D_SOURCES = $(wildcard src/thirdparty/box2d/src/collision/*.cpp) \
                $(wildcard src/thirdparty/box2d/src/common/*.cpp) \
                $(wildcard src/thirdparty/box2d/src/dynamics/*.cpp) \
                $(wildcard src/thirdparty/box2d/src/rope/*.cpp)
LUA_SOURCES = $(wildcard src/thirdparty/Lua/*.c)
LUA_OBJECTS = $(LUA_SOURCES:.c=.o)
SOURCES = main.cpp $(BOX2D_SOURCES)
TARGET = game_engine_linux

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

main: $(LUA_OBJECTS) $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) $(LUA_OBJECTS) -o $(TARGET) $(LIBRARIES)

run: main
	rm out.txt || true
	@if [ -f sdl_user_input.txt.bak ]; then mv sdl_user_input.txt.bak sdl_user_input.txt; fi
	./$(TARGET) > out.txt

play: main
	rm out.txt || true
	@if [ -f sdl_user_input.txt ]; then mv sdl_user_input.txt sdl_user_input.txt.bak; fi
	./$(TARGET) > out.txt
	@if [ -f sdl_user_input.txt.bak ]; then mv sdl_user_input.txt.bak sdl_user_input.txt; fi

ag: main
	rm out.txt || true
	@if [ -f sdl_user_input.txt.bak ]; then mv sdl_user_input.txt.bak sdl_user_input.txt; fi
	AUTOGRADER=1 ./$(TARGET) > out.txt

.PHONY: clean
clean:
	rm -f $(TARGET) $(LUA_OBJECTS)

.PHONY: help
help:
	@echo "Targets:"
	@echo "  main   - Build the game engine"
	@echo "  run    - Build and run the game engine"
	@echo "  play   - Build and play the game engine"
	@echo "  ag     - Build and run the autograder"
	@echo "  clean  - Remove build artifacts"