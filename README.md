# Wordle Solver (CSC 212 Final Project)

Finds every word that fits the Wordle feedback so far, using a trie (prefix tree)
with pruning, and compares it against two baselines: brute force (scan every word)
and incremental filter (re-check only the words left from last turn).

Two interactive front ends sit on top of the same solver:

- a **colored terminal UI** (`solver` binary), and
- a **web page** served by a small C++ HTTP server (`server` binary).

## Build

```
make            # builds solver, server, run_tests, run_bench
make solver     # just the terminal UI
make server     # just the web server
make tests      # just the tests
make bench      # just the benchmark
make clean
```

Needs g++ with C++17 (WSL Ubuntu works). Run everything from this folder so
`data/words.txt` and `web/` are found.

## Terminal UI

```
./solver                 # uses data/words.txt
./solver other_list.txt  # any word list, one word per line
./solver --no-color      # ASCII tiles instead of ANSI colors
```

Menu:
1. **Interactive**: solve a real Wordle. The solver draws the board and an on-screen
   keyboard, suggests a guess, and you type the word you used (Enter accepts the
   suggestion) and the feedback (`g` green, `y` yellow, `b` grey), e.g. `bgybb`.
   Type `undo` to drop the last turn.
2. **Auto play**: plays a game against a random answer (or one you type) with a short
   pause between turns.
3. **Race**: times the trie against brute force and incremental filter on one query,
   with an aligned table, bar chart, and speedup. A size bigger than the word list
   pads it with random (synthetic) words.

## Web page

```
./server          # serves on port 8080
./server 9000     # optional port argument
```

Then open **http://localhost:8080** in your browser. On Windows + WSL2, localhost is
forwarded automatically, so the same URL works from the Windows browser.

Three tabs:
- **Solve**: type a guess (or click "Use suggestion"), click each tile to cycle its
  color grey -> yellow -> green, then Submit. Shows words left, the candidate list
  (first 100), the suggestion, trie time, and nodes visited. Undo and New game buttons.
- **Auto play**: watch the solver play a full game, one row at a time.
- **Race**: pick a dictionary size (1,000 / 14,855 real / 50,000 / 100,000) and see the
  three methods as horizontal bars with work counts, speedup, and synthetic count.

The server keeps no per-user state: every request carries the full guess history in the
query string, so it is safe to open in several tabs at once.

### API (all GET, all return JSON)

| route | params | example response |
|---|---|---|
| `/api/info` | none | `{"words":14855,"nodes":27842}` |
| `/api/feedback` | `guess`, `answer` | `{"feedback":"bbgby"}` |
| `/api/solve` | `guesses`, `feedback` | `{"count":324,"candidates":[...],"suggestion":"soare","trie_us":312.5,"nodes":3025}` |
| `/api/autoplay` | optional `answer` | `{"answer":"whose","turns":[{"guess":"aeros","feedback":"bybyy","left":14855,"us":3921.4}, ...]}` |
| `/api/race` | `size` (1..200000) | `{"size":50000,"synthetic":35145,"guess":"...","feedback":"...","trie":{...},"brute":{...},"incremental":{...}}` |

## Tests and benchmark

```
./run_tests   # unit tests + 300 random games checking trie == brute force == incremental
./run_bench   # prints CSV sections to paste into Sheets
```

## Third-party code

- **cpp-httplib v0.67.0** (single header, MIT license) for the HTTP server only, pinned in
  `third_party/httplib.h`. Linked with `-pthread`. Nothing else is used.

## Files

- `src/constraints.*` what the feedback tells us, and Wordle's feedback rules
- `src/trie.*` the trie and the pruned search
- `src/baselines.*` brute force and incremental filter
- `src/solver.*` loading word lists, synthetic words, guess scoring
- `src/game.*` shared game state (guess/feedback history, validation, constraint building)
- `src/tui.*` colored terminal drawing (tiles, board, keyboard, bars)
- `src/main.cpp` the terminal menu program
- `src/server.cpp` the HTTP server (API + serves `web/`)
- `web/` the browser front end: `index.html`, `style.css`, `app.js`
- `third_party/httplib.h` cpp-httplib (MIT)
- `tests/tests.cpp`, `bench/bench.cpp`
- `data/words.txt` Wordle's allowed-guess list (14,855 words)
