all: build

build: main.cpp
	g++ main.cpp -o ./out/tedit

run:
	make build
	./out/tedit

test:
	g++ test.cpp -o ./out/test
	./out/test