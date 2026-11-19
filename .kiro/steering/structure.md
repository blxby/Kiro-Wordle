---
inclusion: always
---

# Project structure

```
src/
  constraints.h/.cpp   Feedback rules (getFeedback) and the Constraints struct
                       (fixed greens, banned letters per position, min/max letter counts)
  trie.h/.cpp          Node + Trie: insert, lookup, pruned search   <-- Tony's core work, do not edit
  baselines.h/.cpp     bruteForce, incrementalFilter
  solver.h/.cpp        loadWords, makeDictionary (synthetic padding), bestGuess
  main.cpp             entry point for the `solver` binary (terminal UI)
  tui.h/.cpp           NEW: colored terminal drawing helpers
  server.cpp           NEW: entry point for the `server` binary (web API + static files)
web/                   NEW: index.html, style.css, app.js (served by server.cpp)
third_party/           NEW: httplib.h (cpp-httplib, pinned version)
tests/tests.cpp        unit + agreement tests (trie == brute force == incremental)
bench/bench.cpp        CSV benchmark for the report graphs
data/words.txt         14,855 five-letter Wordle words, one per line
Makefile  README.md  REPORT.md  AI_LOG.md
```

## Existing public API (use these; do not change their signatures)

```cpp
// constraints.h
const int WORD_LEN = 5;
struct Constraints {
    int fixed[WORD_LEN];  bool banned[WORD_LEN][26];  int minCount[26];  int maxCount[26];
    void reset();
    void update(const std::string& guess, const std::string& feedback); // feedback: 'g','y','b'
    bool matches(const std::string& word) const;
};
std::string getFeedback(const std::string& guess, const std::string& answer);

// trie.h
class Trie {
    void insert(const std::string& word);
    bool lookup(const std::string& word) const;
    int  search(const Constraints& c, std::string* out, long long& nodesVisited) const; // returns match count
    int  getNodeCount() const;
    int  getWordCount() const;
};

// baselines.h
int bruteForce(const std::string* words, int n, const Constraints& c, std::string* out);
int incrementalFilter(std::string* candidates, int count, const Constraints& c);

// solver.h
std::string* loadWords(const std::string& filename, int& n);           // caller delete[]s
std::string* makeDictionary(const std::string* real, int realCount, int size); // caller delete[]s
std::string  bestGuess(const std::string* candidates, int count);
```

Notes:
- `search` and `bruteForce` write into an `out` array that must have room for every word.
- Timing uses `std::chrono::high_resolution_clock`, reported in microseconds, as in `main.cpp`.
