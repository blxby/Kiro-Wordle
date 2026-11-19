# Wordle Solver (CSC 212 Final Project)

Finds every word that fits the Wordle feedback so far, using a trie (prefix tree)
with pruning, and compares it against two baselines: brute force (scan every word)
and incremental filter (re-check only the words left from last turn).

## Build

```
make            # builds solver, run_tests, run_bench
make solver     # just the solver
make tests      # just the tests
make bench      # just the benchmark
make clean
```

Needs g++ with C++17 (WSL Ubuntu works). Run everything from this folder so
`data/words.txt` is found.

## Run

```
./solver                 # uses data/words.txt
./solver other_list.txt  # any word list, one word per line
```

Menu:
1. **Interactive**: play a real Wordle. The solver suggests a guess, you type the
   word you used and the feedback (`g` green, `y` yellow, `b` grey), e.g. `bgybb`.
2. **Auto play**: picks a random answer and solves it.
3. **Compare**: times the trie against brute force on one query. Type a size bigger
   than the word list to pad it with random (synthetic) words.

Example:

```
--- Turn 1 ---
Words left: 14855  (trie search: 3675 us, 27842 nodes visited)
Suggested guess: aeros
Word you guessed (Enter = aeros): crane
Feedback: bbgby
```

## Tests and benchmark

```
./run_tests   # unit tests + 300 random games checking trie == brute force == incremental
./run_bench   # prints CSV sections to paste into Sheets
```

## Files

- `src/constraints.*` what the feedback tells us, and Wordle's feedback rules
- `src/trie.*` the trie and the pruned search
- `src/baselines.*` brute force and incremental filter
- `src/solver.*` loading word lists, synthetic words, guess scoring
- `src/main.cpp` the menu program
- `tests/tests.cpp`, `bench/bench.cpp`
- `data/words.txt` Wordle's allowed-guess list (14,855 words)
