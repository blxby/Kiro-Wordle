# Implementation Plan

Do one task at a time, build and run `./run_tests` after each, and append to `AI_LOG.md`.
Stop and ask Tony before touching `src/trie.*`, `src/constraints.*`, `src/baselines.*`,
`bench/`, or existing tests.

- [x] 1. Shared game helpers
  - [x] 1.1 Create `src/game.h/.cpp` with `Game`, `resetGame`, `addTurn`, `undoTurn`,
        `buildConstraints`, `isValidWord`, `isValidFeedback` (see design.md)
  - [x] 1.2 Add `src/game.cpp` to `SRC` in the Makefile
  - [x] 1.3 Add a new `testGame()` function to `tests/tests.cpp` and call it from `main`
  - _Requirements: 2.3, 2.4, 2.7, 8.2_

- [x] 2. Terminal drawing helpers
  - [x] 2.1 Create `src/tui.h/.cpp` with ANSI color constants, `setColorEnabled`, `drawTile`,
        `drawBoard`, `drawKeyboard`, `drawBar`, `clearScreen`
  - [x] 2.2 `--no-color` fallback drawing
  - _Requirements: 1.1-1.4_

- [x] 3. Terminal UI modes in `src/main.cpp`
  - [x] 3.1 Interactive solve using `Game` + board + keyboard; Enter = suggestion; `undo`;
        zero-words message
  - [x] 3.2 Auto play with optional typed answer and 600 ms pause
  - [x] 3.3 Race mode: trie vs. brute force vs. incremental, aligned table, bars, speedup,
        synthetic count
  - [x] 3.4 Parse `--no-color` and optional word-list path from `argv`
  - _Requirements: 1, 2, 3_

- [x] 4. Checkpoint: `make solver && ./solver`; solve one real Wordle; commit

- [x] 5. HTTP server
  - [x] 5.1 Download a pinned release of `httplib.h` into `third_party/`; record version in README
  - [x] 5.2 `src/server.cpp`: load words, build trie, mount `./web`, port argument, startup message
  - [x] 5.3 `splitCommas` and history parsing into a `Game`, with 400 errors on bad input
  - [x] 5.4 `/api/info`, `/api/feedback`
  - [x] 5.5 `/api/solve` (candidates capped at 100, suggestion, time, nodes)
  - [x] 5.6 `/api/autoplay`
  - [x] 5.7 `/api/race`
  - [x] 5.8 Makefile `server` target with `-pthread`; add to `all`, `clean`, `.gitignore`
  - _Requirements: 4, 8.1_

- [~] 6. Checkpoint: run `./server`, open each `/api/...` URL in the browser, check the JSON; commit
      (server builds/starts; live browser check deferred to WSL - sandbox has no network)

- [x] 7. Web page
  - [x] 7.1 `web/index.html` layout: tabs, grid, keyboard, info panel, buttons
  - [x] 7.2 `web/style.css`: Wordle colors, grid, flip animation, bars, dark theme
  - [x] 7.3 `web/app.js` Solve tab: typing, tile color cycling, Submit, Undo, New game,
        "Use suggestion", candidate list, stats
  - [x] 7.4 Auto play tab with row-by-row animation
  - [x] 7.5 Race tab with size presets and bar chart
  - _Requirements: 5, 6, 7_

- [~] 8. Checkpoint: solve a real Wordle in the browser; test from the Windows browser via WSL; commit
      (web files complete; browser check deferred to WSL - sandbox has no network)

- [x] 9. Docs and final check
  - [x] 9.1 README: build, `./solver` (+ `--no-color`), `./server` + `http://localhost:8080`,
        screenshots optional, httplib version
  - [x] 9.2 `make clean && make` with zero warnings; `./run_tests` passes; `./run_bench` still runs
  - [x] 9.3 Final `AI_LOG.md` review list: every new file and what Tony should be able to explain
  - _Requirements: 8_
