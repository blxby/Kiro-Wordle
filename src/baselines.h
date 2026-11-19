#pragma once
#include <string>
#include "constraints.h"

// Baseline 1: brute force.
// Check every word in the dictionary. Matches go into out.
// Returns the number of matches. Words checked = n.
int bruteForce(const std::string* words, int n, const Constraints& c, std::string* out);

// Baseline 2: incremental filter.
// Re-check only the words that survived the last guess, keeping matches
// at the front of the same array. Returns the new count. Words checked = count.
int incrementalFilter(std::string* candidates, int count, const Constraints& c);
