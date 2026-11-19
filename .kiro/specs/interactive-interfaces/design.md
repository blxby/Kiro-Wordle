# Design: Interactive Interfaces

## Overview

```
                 +--------------------------------------+
                 |  existing core (unchanged)           |
                 |  trie  constraints  baselines  solver|
                 +-----------------+--------------------+
                                   |
            +----------------------+----------------------+
            |                                             |
   solver binary (main.cpp + tui.cpp)         server binary (server.cpp + httplib.h)
   colored terminal UI                         JSON API + serves web/ files
                                                          |
                                               browser: web/index.html, style.css, app.js
```

Both front ends share one idea: **a game is a list of (guess, feedback) pairs.** To get the
current candidates, start from a fresh `Constraints`, call `update` for each pair, then call
`trie.search`. This makes undo trivial (drop the last pair) and lets the server stay stateless.

## Shared helper (new): `src/game.h/.cpp`

```cpp
const int MAX_TURNS = 20;

struct Game {
    std::string guesses[MAX_TURNS];
    std::string feedback[MAX_TURNS];
    int turns;
};

void resetGame(Game& g);
bool addTurn(Game& g, const std::string& guess, const std::string& fb); // false if full
void undoTurn(Game& g);
void buildConstraints(const Game& g, Constraints& c);  // c.reset() then update per turn
bool isValidWord(const std::string& s);       // exactly 5 lowercase letters
bool isValidFeedback(const std::string& s);   // exactly 5 of g/y/b
```

`isWord`/`isFeedback` in the current `main.cpp` move here. Add tests for these in `tests/tests.cpp`
(new test function only; existing tests untouched).

## Terminal UI: `src/tui.h/.cpp` + `src/main.cpp`

ANSI escape codes are special character sequences that terminals interpret as formatting.
`"\033[42m"` sets a green background, `"\033[0m"` resets. Constants:

| purpose | code |
|---|---|
| green tile | `\033[1;30;42m` |
| yellow tile | `\033[1;30;43m` |
| grey tile | `\033[1;37;100m` |
| unused key | `\033[1;30;47m` |
| reset | `\033[0m` |
| clear screen + home | `\033[2J\033[H` |

```cpp
void setColorEnabled(bool on);                       // --no-color
void drawTile(char letter, char state);              // state: 'g','y','b',' '
void drawBoard(const Game& g);                       // all rows, blank rows up to 6
void drawKeyboard(const Game& g);                    // QWERTY, best state per letter
void drawBar(const std::string& label, double value, double maxValue, int width); // race chart
void clearScreen();
```

Keyboard letter state: loop over every turn and position; rank `g`=3, `y`=2, `b`=1, unused=0;
keep the max per letter in an `int state[26]` array.

`main.cpp` keeps the existing menu loop and modes, rewritten to use `Game` and the draw helpers:
1. Interactive solve (with `undo`)
2. Auto play (optional typed answer; pause between turns with
   `std::this_thread::sleep_for(std::chrono::milliseconds(600))`)
3. Race (trie vs. brute force vs. incremental filter, aligned table + bars)

Race uses the same query style as the benchmark: answer = random dictionary word,
guess = `bestGuess(dict, size)`, one `update`, then time each method once.
Incremental filter is timed on a copy of the full dictionary (it has no previous turn here).

## Web server: `src/server.cpp`

cpp-httplib provides `httplib::Server`. A **route** maps a URL path to a function.
A **query string** is the `?key=value&key2=value2` part of a URL; `req.get_param_value("key")`
reads it. All endpoints are `GET` so they can be tested by pasting URLs into the browser.

Startup: `loadWords("data/words.txt")`, build one global `Trie`, `svr.set_mount_point("/", "./web")`,
`svr.listen("0.0.0.0", port)`.

History format in query strings: `guesses=crane,slate&feedback=bbgby,gbbyb`
(comma-separated, same count). Parsed into a `Game` with a small `splitCommas` helper
(writes into a `std::string parts[MAX_TURNS]` array, returns count).

### Endpoints

| route | params | response |
|---|---|---|
| `GET /api/info` | none | `{"words":14855,"nodes":27842}` |
| `GET /api/solve` | `guesses`, `feedback` (both may be empty) | `{"count":324,"candidates":["..."],"suggestion":"soare","trie_us":312.5,"nodes":3025}` (candidates capped at 100) |
| `GET /api/feedback` | `guess`, `answer` | `{"feedback":"bbgby"}` |
| `GET /api/autoplay` | optional `answer` (must be in word list) | `{"answer":"whose","turns":[{"guess":"aeros","feedback":"bybyy","left":14855,"us":3921.4}, ...]}` |
| `GET /api/race` | `size` (1 to 200000) | `{"size":50000,"synthetic":35145,"guess":"...","feedback":"...","trie":{"matches":120,"us":381.5,"work":6451},"brute":{...},"incremental":{...}}` |

Errors: `res.status = 400; res.set_content("{\"error\":\"...\"}", "application/json");`

JSON is built with a helper `std::string jsonString(const std::string& s)` that wraps in quotes
(words are a-z only, so no escaping is needed; note that in a comment).

Concurrency note: httplib handles requests on multiple threads. The global trie is only read
after startup (`search` is `const`), so sharing it is safe. Each request allocates its own
`new std::string[n]` output array and `delete[]`s it before returning.

## Web front end: `web/`

- `index.html`: header, three tabs (Solve / Auto play / Race), a `#grid` of 30 tile `div`s,
  an on-screen keyboard, info panel, buttons. Links `style.css` and `app.js`.
- `style.css`: CSS grid for the 6x5 board, tile colors matching Wordle
  (`#6aaa64` green, `#c9b458` yellow, `#787c7e` grey), flip animation on reveal, bar chart styles.
  Dark background, works on a laptop screen at the showcase.
- `app.js` (vanilla JS, no modules):
  - State: `history = [{guess, feedback}]`, `currentGuess`, `currentFeedback` (array of 5 states).
  - `fetch("/api/solve?guesses=...&feedback=...")` then `.json()` then render.
    (`fetch` asks the server for a URL and returns a promise; use `async/await` with a comment.)
  - Tile click cycles `b -> y -> g -> b` on the current row only.
  - Keyboard input: letters fill the row, Backspace deletes, Enter submits when the row is full.
  - Auto play: fetch `/api/autoplay`, reveal one row every 700 ms with `setTimeout`.
  - Race: fetch `/api/race?size=N`, draw bars with widths proportional to the max time.

## Makefile additions

```make
SRC = src/constraints.cpp src/trie.cpp src/baselines.cpp src/solver.cpp src/game.cpp

solver: $(SRC) src/main.cpp src/tui.cpp
	$(CXX) $(FAST) $(SRC) src/tui.cpp src/main.cpp -o solver

server: $(SRC) src/server.cpp third_party/httplib.h
	$(CXX) $(FAST) -Ithird_party $(SRC) src/server.cpp -o server -pthread
```

Add `server` to `all`, `clean`, `.PHONY`, and `.gitignore`. Tests and bench also need `src/game.cpp`
added to `SRC`.

## Testing

- Unit tests: `isValidWord`, `isValidFeedback`, `addTurn`/`undoTurn`, and that
  `buildConstraints` + `search` gives the same count as calling `update` directly.
- Manual checklist (in tasks.md): each endpoint via browser URL, each page, terminal modes,
  `--no-color`, a real daily Wordle solved through both interfaces.
