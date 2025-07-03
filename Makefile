flags = -g -O0 --std=c++17 -Wall -pedantic -fsanitize=address,leak,undefined
.PHONY: clean

output: main.o scanner.o image.o png.o bitstream.o inflate.o huffman_tree.o
	g++ $(flags) -o output -lz $^

test: test.o inflate.o bitstream.o huffman_tree.o scanner.o png.o image.o
	g++ $(flags) -o test -lz $^

%.o: %.cpp
	g++ $(flags) -c $^ -o $@

clean:
	rm *.o output *.out