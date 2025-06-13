.PHONY: clean

output: main.o scanner.o image.o png.o
	g++ -g -O0 -o output -lz $^

%.o: %.cpp
	g++ -g -O0 -c $^ -o $@

clean:
	rm *.o output *.out