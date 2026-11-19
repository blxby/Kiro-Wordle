#include "solver.h"
#include "constraints.h"
#include "trie.h"
#include <fstream>
#include <cstdlib>

std::string* loadWords(const std::string& filename, int& n) {
    n = 0;
    std::ifstream file(filename);
    if (!file) {
        return nullptr;
    }

    // dynamic array that doubles when full
    int capacity = 1024;
    std::string* words = new std::string[capacity];

    Trie seen;   // used only to skip duplicates
    std::string line;
    while (file >> line) {
        if ((int)line.length() != WORD_LEN) {
            continue;
        }

        bool ok = true;
        for (int i = 0; i < WORD_LEN; i++) {
            char ch = line[i];
            if (ch >= 'A' && ch <= 'Z') {
                ch = ch - 'A' + 'a';
                line[i] = ch;
            }
            if (ch < 'a' || ch > 'z') {
                ok = false;
            }
        }
        if (!ok || seen.lookup(line)) {
            continue;
        }
        seen.insert(line);

        if (n == capacity) {
            capacity = capacity * 2;
            std::string* bigger = new std::string[capacity];
            for (int i = 0; i < n; i++) {
                bigger[i] = words[i];
            }
            delete[] words;
            words = bigger;
        }
        words[n] = line;
        n++;
    }
    return words;
}

std::string* makeDictionary(const std::string* real, int realCount, int size) {
    std::string* dict = new std::string[size];
    Trie seen;   // used only to skip duplicates
    int n = 0;

    // real words first
    for (int i = 0; i < realCount && n < size; i++) {
        dict[n] = real[i];
        seen.insert(real[i]);
        n++;
    }

    // then random words until full
    std::string word = "aaaaa";
    while (n < size) {
        for (int pos = 0; pos < WORD_LEN; pos++) {
            word[pos] = 'a' + rand() % 26;
        }
        if (!seen.lookup(word)) {
            seen.insert(word);
            dict[n] = word;
            n++;
        }
    }
    return dict;
}

std::string bestGuess(const std::string* candidates, int count) {
    if (count == 0) {
        return "";
    }

    // freq[letter] = how many candidates contain that letter at least once
    int freq[26] = {0};
    for (int i = 0; i < count; i++) {
        bool used[26] = {false};
        for (int pos = 0; pos < WORD_LEN; pos++) {
            int letter = candidates[i][pos] - 'a';
            if (!used[letter]) {
                used[letter] = true;
                freq[letter]++;
            }
        }
    }

    // score each candidate by its distinct letters, keep the best
    int bestIndex = 0;
    int bestScore = -1;
    for (int i = 0; i < count; i++) {
        bool used[26] = {false};
        int score = 0;
        for (int pos = 0; pos < WORD_LEN; pos++) {
            int letter = candidates[i][pos] - 'a';
            if (!used[letter]) {
                used[letter] = true;
                score += freq[letter];
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    return candidates[bestIndex];
}
