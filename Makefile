.PHONY: clean

output: main.o scanner.o
	g++ -g -O0 -o output -lz $^

%.o: %.cpp
	g++ -g -O0 -c $^ -o $@

clean:
	rm *.o output *.out