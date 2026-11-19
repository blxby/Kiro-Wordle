#include "baselines.h"

int bruteForce(const std::string* words, int n, const Constraints& c, std::string* out) {
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (c.matches(words[i])) {
            out[found] = words[i];
            found++;
        }
    }
    return found;
}

int incrementalFilter(std::string* candidates, int count, const Constraints& c) {
    int kept = 0;
    for (int i = 0; i < count; i++) {
        if (c.matches(candidates[i])) {
            candidates[kept] = candidates[i];
            kept++;
        }
    }
    return kept;
}
