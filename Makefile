CXX = g++
DEBUG = -std=c++17 -Wall -g
FAST = -std=c++17 -Wall -O2

SRC = src/constraints.cpp src/trie.cpp src/baselines.cpp src/solver.cpp src/game.cpp
HDR = src/constraints.h src/trie.h src/baselines.h src/solver.h src/game.h

all: solver tests bench

solver: $(SRC) src/main.cpp $(HDR)
	$(CXX) $(FAST) $(SRC) src/main.cpp -o solver

tests: $(SRC) tests/tests.cpp $(HDR)
	$(CXX) $(DEBUG) $(SRC) tests/tests.cpp -o run_tests

bench: $(SRC) bench/bench.cpp $(HDR)
	$(CXX) $(FAST) $(SRC) bench/bench.cpp -o run_bench

clean:
	rm -f solver run_tests run_bench

.PHONY: all clean tests bench
