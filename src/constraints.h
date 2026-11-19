#pragma once
#include <string>

const int WORD_LEN = 5;   // Wordle words are always 5 letters

// Everything we know about the answer from the feedback so far.
// Letters are stored as 0..25 ('a' = 0).
struct Constraints {
    int fixed[WORD_LEN];          // green letter at each position, or -1 if unknown
    bool banned[WORD_LEN][26];    // banned[pos][letter] = letter cannot be at pos
    int minCount[26];             // answer has at least this many of each letter
    int maxCount[26];             // answer has at most this many of each letter

    Constraints();

    // start over with no information
    void reset();

    // add the information from one guess and its feedback
    // feedback is 5 chars: 'g' = green, 'y' = yellow, 'b' = grey (black)
    void update(const std::string& guess, const std::string& feedback);

    // true if word is consistent with everything we know
    bool matches(const std::string& word) const;
};

// The feedback Wordle would give for a guess against the answer.
// Handles duplicate letters the same way the real game does.
std::string getFeedback(const std::string& guess, const std::string& answer);
