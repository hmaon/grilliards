CC=gcc
CXX=g++
CFLAGS=-O3 -g -funsigned-char -Wno-write-strings -fno-rtti -march=native `sdl-config --cflags` -ffast-math
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lGL -lGLU -lglut `sdl-config --libs` -lSDL_mixer -lSDL_image

CPPSRC=$(wildcard *.cpp)
CSRC=$(wildcard *.c)
OBJ=$(CPPSRC:.cpp=.o) $(CSRC:.c=.o)



%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

final: $(OBJ)
	$(CXX) $(OBJ) $(CFLAGS) $(LDFLAGS) -o $@

all: final

fixcpp:
	perl -p -i -e 's/void main/int main/g' *cpp

clean:
	rm final $(OBJ)
