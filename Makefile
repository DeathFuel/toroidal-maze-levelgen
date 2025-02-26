flags := -Wall -Wextra -Wshadow -O3 -std=c++17 -march=native

default: build
build:
	g++ levelgen.cpp $(flags) -o levelgen.out
