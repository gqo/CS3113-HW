
SDL_HEADER_DIR=/usr/include/SDL2

CFLAGS= -std=c++11 -I$(SDL_HEADER_DIR)
LDFLAGS=-lc -lm -pthread -lSDL2 -lGL

SRCS=ShaderProgram.cpp main.cpp

default:
	g++ $(SRCS) $(CFLAGS) $(LDFLAGS) -o mygame

run:
	g++ $(SRCS) $(CFLAGS) $(LDFLAGS) -o mygame
	./mygame

clean:
	rm mygame
