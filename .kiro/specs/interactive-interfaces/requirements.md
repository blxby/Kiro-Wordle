# Requirements: Interactive Interfaces

## Introduction

Add two interactive front ends to the existing C++ Wordle solver for the CS Showcase:
a colored terminal UI (`solver` binary) and a web page served by a C++ HTTP server
(`server` binary). Both use the existing trie, constraints, baselines, and solver functions
unchanged. Both must make the trie's advantage visible (words left, time, nodes visited).

## Requirement 1: Colored terminal board

**User story:** As Tony demoing in a terminal, I want guesses drawn as Wordle-style colored
tiles so the audience can follow the game at a glance.

**Acceptance criteria**
1. WHEN a guess and its feedback are recorded THE SYSTEM SHALL draw the guess as 5 tiles with
   green, yellow, or grey backgrounds and uppercase black/white letters using ANSI escape codes.
2. THE SYSTEM SHALL redraw the full board (all previous guesses) after every turn.
3. THE SYSTEM SHALL show an on-screen QWERTY keyboard where each letter is colored by the best
   state known for it (green > yellow > grey > unused).
4. WHEN the program is started with `--no-color` THE SYSTEM SHALL draw tiles as `[C]` green,
   `(C)` yellow, ` C ` grey instead of colors.

## Requirement 2: Terminal interactive solve mode

**User story:** As a player of the real daily Wordle, I want the solver to suggest guesses and
narrow the words based on the feedback I type.

**Acceptance criteria**
1. WHEN a turn starts THE SYSTEM SHALL show the number of words left, up to 20 of them,
   the trie search time in microseconds, the nodes visited, and a suggested guess.
2. WHEN the user presses Enter without typing THE SYSTEM SHALL use the suggested guess.
3. WHEN the user types a guess that is not 5 lowercase letters THE SYSTEM SHALL re-prompt.
4. WHEN the user types feedback that is not 5 characters of `g`/`y`/`b` THE SYSTEM SHALL re-prompt.
5. WHEN the feedback is `ggggg` THE SYSTEM SHALL show a solved message with the guess count.
6. WHEN no words remain THE SYSTEM SHALL say the feedback was probably mistyped and offer
   to undo the last turn.
7. THE SYSTEM SHALL allow typing `undo` to remove the last guess and recompute.

## Requirement 3: Terminal auto play and race modes

**Acceptance criteria**
1. WHEN auto play is chosen THE SYSTEM SHALL pick a random answer (or one typed by the user),
   play the solver against it, and draw each turn on the board with a short pause between turns.
2. WHEN race mode is chosen THE SYSTEM SHALL ask for a dictionary size, build that dictionary
   with `makeDictionary`, run one query, and show trie vs. brute force vs. incremental filter:
   matches, time (us), and work (nodes visited / words checked) in an aligned table, plus a
   text bar chart of the three times and the speedup.
3. WHEN the size is larger than the real word list THE SYSTEM SHALL say how many words are synthetic.

## Requirement 4: Web server

**User story:** As a showcase visitor, I want to use the solver in a browser.

**Acceptance criteria**
1. WHEN `./server` starts THE SYSTEM SHALL load `data/words.txt`, build the trie once, print
   `Open http://localhost:8080`, and serve files from `web/`.
2. THE SYSTEM SHALL accept an optional port argument (`./server 9000`).
3. THE SYSTEM SHALL keep no per-user state; every API request carries the full guess history.
4. WHEN a request has invalid input THE SYSTEM SHALL return HTTP 400 with `{"error": "<message>"}`.
5. THE SYSTEM SHALL return all API responses as JSON with `Content-Type: application/json`.

## Requirement 5: Web solve page

**Acceptance criteria**
1. THE SYSTEM SHALL show a 6x5 Wordle grid. The user types a guess (or clicks "Use suggestion"),
   then clicks each tile to cycle its color grey -> yellow -> green -> grey.
2. WHEN the user clicks Submit THE SYSTEM SHALL send the history to the server and display
   words left, the candidate list (first 100), the suggestion, trie time, and nodes visited.
3. THE SYSTEM SHALL provide Undo and New game buttons.
4. THE SYSTEM SHALL show an on-screen keyboard colored by known letter states, and typing on it
   or the physical keyboard SHALL fill the current row.

## Requirement 6: Web auto play page

**Acceptance criteria**
1. WHEN the user clicks Auto play THE SYSTEM SHALL request a full game from the server
   (random answer, or one typed in) and animate the rows one at a time.
2. THE SYSTEM SHALL show words left and search time for each turn.

## Requirement 7: Web race page

**Acceptance criteria**
1. THE SYSTEM SHALL let the user choose a dictionary size (presets: 1,000 / 14,855 real /
   50,000 / 100,000) and run trie vs. brute force vs. incremental filter on one query.
2. THE SYSTEM SHALL show the three times as horizontal bars, the work counts, the speedup,
   and how many words were synthetic.

## Requirement 8: Build, tests, docs

**Acceptance criteria**
1. `make` SHALL build `solver`, `server`, `run_tests`, and `run_bench` with zero warnings.
2. `./run_tests` SHALL still report 0 failures; new feedback-parsing helpers SHALL get tests.
3. README SHALL document how to run both interfaces, the httplib version, and the WSL browser URL.
4. `AI_LOG.md` SHALL list what was generated for each task.

## Out of scope

- Changes to the trie, constraints, baselines, or benchmark.
- Accounts, databases, deployment, mobile layout polish beyond a basic responsive grid.
- Entropy-based guess ranking.
