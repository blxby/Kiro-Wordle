#pragma once
#include <string>

// Read a word list file (one word per line).
// Keeps only 5-letter words made of letters, lowercased, no duplicates.
// Returns a new[] array of words (caller must delete[]) and sets n to its size.
std::string* loadWords(const std::string& filename, int& n);

// Make a dictionary of exactly size words: the real words first, then
// random 5-letter words (synthetic) until it is big enough. No duplicates.
// Returns a new[] array (caller must delete[]).
std::string* makeDictionary(const std::string* real, int realCount, int size);

// Pick the next guess: the candidate whose distinct letters are the most
// common among the remaining candidates.
std::string bestGuess(const std::string* candidates, int count);
