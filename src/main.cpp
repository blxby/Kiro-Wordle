#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include "constraints.h"
#include "trie.h"
#include "baselines.h"
#include "solver.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

// microseconds between two time points
double microsBetween(Clock::time_point start, Clock::time_point end) {
    return chrono::duration<double, micro>(end - start).count();
}

// true if s is exactly 5 lowercase letters
bool isWord(const string& s) {
    if ((int)s.length() != WORD_LEN) {
        return false;
    }
    for (int i = 0; i < WORD_LEN; i++) {
        if (s[i] < 'a' || s[i] > 'z') {
            return false;
        }
    }
    return true;
}

// true if s is exactly 5 of g / y / b
bool isFeedback(const string& s) {
    if ((int)s.length() != WORD_LEN) {
        return false;
    }
    for (int i = 0; i < WORD_LEN; i++) {
        if (s[i] != 'g' && s[i] != 'y' && s[i] != 'b') {
            return false;
        }
    }
    return true;
}

// Mode 1: you play a real Wordle, the solver suggests guesses
void interactive(const Trie& trie, int n) {
    Constraints c;
    string* candidates = new string[n];

    cout << "\nFeedback format: 5 letters, g = green, y = yellow, b = grey. Example: bgybb\n";

    for (int turn = 1; turn <= 6; turn++) {
        long long visited = 0;
        Clock::time_point start = Clock::now();
        int count = trie.search(c, candidates, visited);
        Clock::time_point end = Clock::now();

        cout << "\n--- Turn " << turn << " ---\n";
        cout << "Words left: " << count << "  (trie search: " << microsBetween(start, end)
             << " us, " << visited << " nodes visited)\n";

        if (count == 0) {
            cout << "No words match. Double-check the feedback you typed.\n";
            break;
        }

        int show = count;
        if (show > 20) {
            show = 20;
        }
        for (int i = 0; i < show; i++) {
            cout << candidates[i] << " ";
        }
        if (count > show) {
            cout << "...";
        }
        cout << "\n";

        string suggestion = bestGuess(candidates, count);
        cout << "Suggested guess: " << suggestion << "\n";

        string guess;
        while (true) {
            cout << "Word you guessed (Enter = " << suggestion << "): ";
            getline(cin, guess);
            if (guess == "") {
                guess = suggestion;
            }
            if (isWord(guess)) {
                break;
            }
            cout << "Type 5 lowercase letters.\n";
        }

        string feedback;
        while (true) {
            cout << "Feedback: ";
            getline(cin, feedback);
            if (isFeedback(feedback)) {
                break;
            }
            cout << "Type 5 characters, each g, y, or b.\n";
        }

        if (feedback == "ggggg") {
            cout << "Solved in " << turn << " guesses!\n";
            break;
        }
        c.update(guess, feedback);
    }

    delete[] candidates;
}

// Mode 2: the solver plays a random game against itself
void autoPlay(const Trie& trie, const string* words, int n) {
    string answer = words[rand() % n];
    Constraints c;
    string* candidates = new string[n];
    double totalTime = 0;

    cout << "\nSecret answer: " << answer << "\n";

    for (int turn = 1; turn <= 20; turn++) {
        long long visited = 0;
        Clock::time_point start = Clock::now();
        int count = trie.search(c, candidates, visited);
        Clock::time_point end = Clock::now();
        totalTime += microsBetween(start, end);

        string guess = bestGuess(candidates, count);
        string feedback = getFeedback(guess, answer);

        cout << "Turn " << turn << ": " << guess << "  " << feedback
             << "   (" << count << " words left before guess, "
             << microsBetween(start, end) << " us)\n";

        if (feedback == "ggggg") {
            cout << "Solved in " << turn << " guesses. Total search time: "
                 << totalTime << " us\n";
            break;
        }
        c.update(guess, feedback);
    }

    delete[] candidates;
}

// Mode 3: time the trie against brute force on one query
void compare(const string* words, int n) {
    cout << "\nDictionary size (Enter = " << n << " real words, bigger adds synthetic words): ";
    string line;
    getline(cin, line);
    int size = n;
    if (line != "") {
        size = atoi(line.c_str());
        if (size < 1) {
            size = n;
        }
    }

    string* dict = makeDictionary(words, n, size);
    if (size > n) {
        cout << (size - n) << " of the " << size << " words are synthetic (random letters).\n";
    }

    Trie trie;
    Clock::time_point start = Clock::now();
    for (int i = 0; i < size; i++) {
        trie.insert(dict[i]);
    }
    Clock::time_point end = Clock::now();
    cout << "Built trie in " << microsBetween(start, end) / 1000.0 << " ms ("
         << trie.getNodeCount() << " nodes)\n";

    // one guess against a random answer
    string answer = dict[rand() % size];
    string guess = bestGuess(dict, size);
    string feedback = getFeedback(guess, answer);
    Constraints c;
    c.update(guess, feedback);
    cout << "Answer: " << answer << "   Guess: " << guess << "   Feedback: " << feedback << "\n";

    string* out1 = new string[size];
    string* out2 = new string[size];

    long long visited = 0;
    start = Clock::now();
    int trieCount = trie.search(c, out1, visited);
    end = Clock::now();
    double trieTime = microsBetween(start, end);

    start = Clock::now();
    int bruteCount = bruteForce(dict, size, c, out2);
    end = Clock::now();
    double bruteTime = microsBetween(start, end);

    cout << "\nMethod        Matches   Time (us)   Work\n";
    cout << "Trie          " << trieCount << "   " << trieTime << "   "
         << visited << " nodes visited\n";
    cout << "Brute force   " << bruteCount << "   " << bruteTime << "   "
         << size << " words checked\n";
    if (trieTime > 0) {
        cout << "Trie was " << bruteTime / trieTime << "x faster\n";
    }

    delete[] out1;
    delete[] out2;
    delete[] dict;
}

int main(int argc, char* argv[]) {
    string filename = "data/words.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    int n = 0;
    string* words = loadWords(filename, n);
    if (words == nullptr || n == 0) {
        cout << "Could not load words from " << filename << "\n";
        return 1;
    }

    Trie trie;
    for (int i = 0; i < n; i++) {
        trie.insert(words[i]);
    }
    cout << "Loaded " << n << " words from " << filename << "\n";

    srand(time(0));

    while (true) {
        cout << "\n1) Interactive (solve a real Wordle)\n";
        cout << "2) Auto play a random game\n";
        cout << "3) Compare trie vs brute force\n";
        cout << "q) Quit\n";
        cout << "Choice: ";

        string choice;
        if (!getline(cin, choice) || choice == "q") {
            break;
        }
        if (choice == "1") {
            interactive(trie, n);
        } else if (choice == "2") {
            autoPlay(trie, words, n);
        } else if (choice == "3") {
            compare(words, n);
        }
    }

    delete[] words;
    return 0;
}
