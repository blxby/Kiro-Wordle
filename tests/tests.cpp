#include <iostream>
#include <string>
#include <cstdlib>
#include "../src/constraints.h"
#include "../src/trie.h"
#include "../src/baselines.h"
#include "../src/solver.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name) {
    if (condition) {
        passed++;
    } else {
        failed++;
        cout << "FAILED: " << name << "\n";
    }
}

// true if the word has some letter more than once
bool hasDoubleLetter(const string& word) {
    for (int i = 0; i < WORD_LEN; i++) {
        for (int j = i + 1; j < WORD_LEN; j++) {
            if (word[i] == word[j]) {
                return true;
            }
        }
    }
    return false;
}

void testInsertLookup() {
    Trie t;
    t.insert("crane");
    t.insert("crate");
    t.insert("slate");
    t.insert("crane");   // duplicate should not count twice

    check(t.lookup("crane"), "lookup crane");
    check(t.lookup("crate"), "lookup crate");
    check(t.lookup("slate"), "lookup slate");
    check(!t.lookup("cran"), "prefix cran is not a word");
    check(!t.lookup("crank"), "crank was never inserted");
    check(!t.lookup("zzzzz"), "zzzzz was never inserted");
    check(t.getWordCount() == 3, "word count is 3");
    // root + c,r,a,n,e + t,e (crate shares "cra") + s,l,a,t,e = 1 + 5 + 2 + 5
    check(t.getNodeCount() == 13, "node count is 13");
}

void testFeedback() {
    check(getFeedback("crane", "crane") == "ggggg", "feedback all green");
    check(getFeedback("fjord", "crane") == "bbbyb", "feedback one yellow");
    // two e's in the guess, one e in the answer: only one gets colored
    check(getFeedback("speed", "abide") == "bbyby", "feedback duplicate e");
    // two l's that are both green, third l is grey
    check(getFeedback("lolly", "hello") == "byggb", "feedback triple l");
}

void testConstraintsByHand() {
    // answer hello, guess lolly -> byggb
    Constraints c;
    c.update("lolly", "byggb");
    check(c.matches("hello"), "hello matches lolly feedback");
    check(!c.matches("lolly"), "lolly does not match its own non-green feedback");
    check(c.minCount['l' - 'a'] == 2 && c.maxCount['l' - 'a'] == 2, "exactly two l's");
    check(c.matches("jello"), "jello also fits (two l's, o not at pos 1)");
    check(!c.matches("hollo"), "hollo fails (o banned at pos 1)");
    check(!c.matches("llama"), "llama fails (l banned at pos 0)");

    // a small trie: search should match brute force exactly
    string small[6] = {"hello", "jello", "cello", "hollo", "lolly", "llama"};
    Trie t;
    for (int i = 0; i < 6; i++) {
        t.insert(small[i]);
    }
    string out[6];
    string out2[6];
    long long visited = 0;
    int trieCount = t.search(c, out, visited);
    int bruteCount = bruteForce(small, 6, c, out2);
    check(trieCount == 3, "small trie finds hello, jello, cello");
    check(trieCount == bruteCount, "small trie count == brute force count");
}

// Play many random games and check all three methods give the same set every turn
void testAgreement(const string* words, int n) {
    Trie trie;
    for (int i = 0; i < n; i++) {
        trie.insert(words[i]);
    }

    string* trieOut = new string[n];
    string* bruteOut = new string[n];
    string* incremental = new string[n];

    int games = 300;
    int turnsChecked = 0;
    int doubleGuesses = 0;
    bool allAgree = true;

    for (int g = 0; g < games; g++) {
        string answer = words[rand() % n];
        Constraints c;

        // incremental filter starts with the whole dictionary
        for (int i = 0; i < n; i++) {
            incremental[i] = words[i];
        }
        int incCount = n;

        for (int turn = 0; turn < 6; turn++) {
            // random guess; every other turn force a guess with a double letter
            string guess = words[rand() % n];
            if (turn % 2 == 1) {
                while (!hasDoubleLetter(guess)) {
                    guess = words[rand() % n];
                }
            }
            if (hasDoubleLetter(guess)) {
                doubleGuesses++;
            }

            c.update(guess, getFeedback(guess, answer));

            long long visited = 0;
            int trieCount = trie.search(c, trieOut, visited);
            int bruteCount = bruteForce(words, n, c, bruteOut);
            incCount = incrementalFilter(incremental, incCount, c);
            turnsChecked++;

            bool ok = (trieCount == bruteCount) && (incCount == bruteCount);
            // every trie result must really match (the trie has no duplicates,
            // so same count + all valid means the same set as brute force)
            for (int i = 0; i < trieCount && ok; i++) {
                if (!c.matches(trieOut[i])) {
                    ok = false;
                }
            }
            for (int i = 0; i < bruteCount && ok; i++) {
                if (!trie.lookup(bruteOut[i])) {
                    ok = false;
                }
            }
            // the real answer must never be filtered out
            if (!c.matches(answer)) {
                ok = false;
            }

            if (!ok) {
                allAgree = false;
                cout << "Mismatch: answer " << answer << ", guess " << guess
                     << ": trie " << trieCount << ", brute " << bruteCount
                     << ", incremental " << incCount << "\n";
            }
        }
    }

    check(allAgree, "trie, brute force, incremental agree on every turn");
    cout << "Agreement test: " << games << " games, " << turnsChecked << " turns, "
         << doubleGuesses << " guesses with double letters\n";

    delete[] trieOut;
    delete[] bruteOut;
    delete[] incremental;
}

int main() {
    srand(212);

    testInsertLookup();
    testFeedback();
    testConstraintsByHand();

    int n = 0;
    string* words = loadWords("data/words.txt", n);
    if (words == nullptr || n == 0) {
        cout << "Could not load data/words.txt (run from the project folder)\n";
        failed++;
    } else {
        testAgreement(words, n);
        delete[] words;
    }

    cout << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
