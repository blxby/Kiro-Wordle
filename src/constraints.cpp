#include "constraints.h"

Constraints::Constraints() {
    reset();
}

void Constraints::reset() {
    for (int pos = 0; pos < WORD_LEN; pos++) {
        fixed[pos] = -1;
        for (int letter = 0; letter < 26; letter++) {
            banned[pos][letter] = false;
        }
    }
    for (int letter = 0; letter < 26; letter++) {
        minCount[letter] = 0;
        maxCount[letter] = WORD_LEN;
    }
}

void Constraints::update(const std::string& guess, const std::string& feedback) {
    // position rules
    for (int pos = 0; pos < WORD_LEN; pos++) {
        int letter = guess[pos] - 'a';
        if (feedback[pos] == 'g') {
            // green: this letter is fixed here
            fixed[pos] = letter;
        } else {
            // yellow or grey: this letter is not at this position
            banned[pos][letter] = true;
        }
    }

    // count rules (this is what handles duplicate letters)
    for (int letter = 0; letter < 26; letter++) {
        int inWord = 0;        // how many of this letter came back green or yellow
        bool hasGrey = false;  // did any copy of this letter come back grey
        bool inGuess = false;

        for (int pos = 0; pos < WORD_LEN; pos++) {
            if (guess[pos] - 'a' == letter) {
                inGuess = true;
                if (feedback[pos] == 'b') {
                    hasGrey = true;
                } else {
                    inWord++;
                }
            }
        }

        if (!inGuess) {
            continue;
        }
        // the answer has at least as many as were colored green/yellow
        if (inWord > minCount[letter]) {
            minCount[letter] = inWord;
        }
        // a grey copy means we found the exact count
        if (hasGrey && inWord < maxCount[letter]) {
            maxCount[letter] = inWord;
        }
    }
}

bool Constraints::matches(const std::string& word) const {
    int count[26] = {0};

    for (int pos = 0; pos < WORD_LEN; pos++) {
        int letter = word[pos] - 'a';
        if (fixed[pos] != -1 && fixed[pos] != letter) {
            return false;
        }
        if (banned[pos][letter]) {
            return false;
        }
        count[letter]++;
    }

    for (int letter = 0; letter < 26; letter++) {
        if (count[letter] < minCount[letter] || count[letter] > maxCount[letter]) {
            return false;
        }
    }
    return true;
}

std::string getFeedback(const std::string& guess, const std::string& answer) {
    std::string result = "bbbbb";
    int left[26] = {0};   // answer letters not already used by a green

    // pass 1: greens
    for (int i = 0; i < WORD_LEN; i++) {
        if (guess[i] == answer[i]) {
            result[i] = 'g';
        } else {
            left[answer[i] - 'a']++;
        }
    }

    // pass 2: yellows, only as many as are left over
    for (int i = 0; i < WORD_LEN; i++) {
        if (result[i] != 'g' && left[guess[i] - 'a'] > 0) {
            result[i] = 'y';
            left[guess[i] - 'a']--;
        }
    }
    return result;
}
