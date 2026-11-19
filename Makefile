CXX = g++
DEBUG = -std=c++17 -Wall -g
FAST = -std=c++17 -Wall -O2

SRC = src/constraints.cpp src/trie.cpp src/baselines.cpp src/solver.cpp src/game.cpp
HDR = src/constraints.h src/trie.h src/baselines.h src/solver.h src/game.h

all: solver server tests bench

solver: $(SRC) src/main.cpp src/tui.cpp $(HDR) src/tui.h
	$(CXX) $(FAST) $(SRC) src/tui.cpp src/main.cpp -o solver

# -Ithird_party finds httplib.h; -pthread is required because httplib
# serves requests on multiple threads
server: $(SRC) src/server.cpp third_party/httplib.h $(HDR)
	$(CXX) $(FAST) -Ithird_party $(SRC) src/server.cpp -o server -pthread

tests: $(SRC) tests/tests.cpp $(HDR)
	$(CXX) $(DEBUG) $(SRC) tests/tests.cpp -o run_tests

bench: $(SRC) bench/bench.cpp $(HDR)
	$(CXX) $(FAST) $(SRC) bench/bench.cpp -o run_bench

clean:
	rm -f solver server run_tests run_bench

.PHONY: all clean tests bench server
