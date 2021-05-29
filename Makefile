all: main.cpp
	g++ -o a main.cpp -lX11 -lGL -lpthread -lpng -g
	./a
