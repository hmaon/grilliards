CC=gcc
CXX=codelitegcc clang++
CFLAGS=-O3 -funsigned-char -Wno-write-strings -march=native -ffast-math `pkg-config --cflags sdl SDL_mixer SDL_image glew` -Wall
CXXFLAGS=$(CFLAGS) -fno-rtti -std=c++11
LDFLAGS=-lGL -lGLU -lglut `pkg-config --libs sdl SDL_mixer SDL_image glew` 

CPPSRC=$(wildcard *.cpp)
CSRC=$(wildcard *.c)
OBJ=$(CPPSRC:.cpp=.o) $(CSRC:.c=.o)



%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

grilliards: $(OBJ)
	$(CXX) $(OBJ) $(CFLAGS) $(LDFLAGS) -o $@

all: grilliards

fixcpp:
	perl -p -i -e 's/void main/int main/g' *cpp

clean:
	rm grilliards $(OBJ)
