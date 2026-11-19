#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#include "constraints.h"
#include "trie.h"
#include "baselines.h"
#include "solver.h"
#include "game.h"
#include "tui.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

// microseconds between two time points
double microsBetween(Clock::time_point start, Clock::time_point end) {
    return chrono::duration<double, micro>(end - start).count();
}

// Run the trie search for the current game state. Fills candidates, sets the
// search time (us) and nodes visited, and returns the match count.
int searchGame(const Trie& trie, const Game& g, string* candidates,
               double& timeUs, long long& visited) {
    Constraints c;
    buildConstraints(g, c);
    visited = 0;
    Clock::time_point start = Clock::now();
    int count = trie.search(c, candidates, visited);
    Clock::time_point end = Clock::now();
    timeUs = microsBetween(start, end);
    return count;
}

// Mode 1: you play a real Wordle, the solver suggests guesses
void interactive(const Trie& trie, int n) {
    Game g;
    resetGame(g);
    string* candidates = new string[n];

    while (true) {
        double timeUs = 0;
        long long visited = 0;
        int count = searchGame(trie, g, candidates, timeUs, visited);

        clearScreen();
        cout << "Interactive solve  (feedback: g = green, y = yellow, b = grey)\n\n";
        drawBoard(g);
        cout << "\n";
        drawKeyboard(g);
        cout << "\n--- Turn " << (g.turns + 1) << " ---\n";
        cout << "Words left: " << count << "  (trie search: " << timeUs
             << " us, " << visited << " nodes visited)\n";

        if (count == 0) {
            cout << "No words match. The feedback was probably mistyped.\n";
            cout << "Type 'undo' to drop the last turn, or Enter to quit: ";
            string line;
            getline(cin, line);
            if (line == "undo") {
                undoTurn(g);
                continue;
            }
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

        // read a guess; Enter uses the suggestion, 'undo' drops the last turn
        string guess;
        bool doUndo = false;
        while (true) {
            cout << "Word you guessed (Enter = " << suggestion << ", or 'undo'): ";
            if (!getline(cin, guess)) {
                delete[] candidates;
                return;
            }
            if (guess == "undo") {
                doUndo = true;
                break;
            }
            if (guess == "") {
                guess = suggestion;
            }
            if (isValidWord(guess)) {
                break;
            }
            cout << "Type 5 lowercase letters.\n";
        }
        if (doUndo) {
            undoTurn(g);
            continue;
        }

        string feedback;
        while (true) {
            cout << "Feedback: ";
            getline(cin, feedback);
            if (isValidFeedback(feedback)) {
                break;
            }
            cout << "Type 5 characters, each g, y, or b.\n";
        }

        addTurn(g, guess, feedback);

        if (feedback == "ggggg") {
            clearScreen();
            cout << "Solved!\n\n";
            drawBoard(g);
            cout << "\nSolved in " << g.turns << " guesses!\n";
            break;
        }
    }

    delete[] candidates;
}

// Mode 2: the solver plays a game against an answer (random or typed)
void autoPlay(const Trie& trie, const string* words, int n) {
    cout << "Answer (Enter = random): ";
    string answer;
    getline(cin, answer);
    if (!isValidWord(answer) || !trie.lookup(answer)) {
        if (answer != "") {
            cout << "Not a 5-letter dictionary word; picking a random answer.\n";
        }
        answer = words[rand() % n];
    }

    Game g;
    resetGame(g);
    string* candidates = new string[n];
    double totalTime = 0;

    for (int turn = 1; turn <= MAX_TURNS; turn++) {
        double timeUs = 0;
        long long visited = 0;
        int count = searchGame(trie, g, candidates, timeUs, visited);
        totalTime += timeUs;

        string guess = bestGuess(candidates, count);
        string feedback = getFeedback(guess, answer);
        addTurn(g, guess, feedback);

        clearScreen();
        cout << "Auto play  (secret answer: " << answer << ")\n\n";
        drawBoard(g);
        cout << "\n";
        drawKeyboard(g);
        cout << "\nTurn " << turn << ": " << count << " words left before the guess, "
             << timeUs << " us\n";

        if (feedback == "ggggg") {
            cout << "Solved in " << turn << " guesses. Total search time: "
                 << totalTime << " us\n";
            break;
        }

        // short pause so the audience can follow each row
        this_thread::sleep_for(chrono::milliseconds(600));
    }

    delete[] candidates;
}

// print a value left-padded into a fixed-width column
void printCol(const string& s, int width) {
    cout << s;
    for (int i = (int)s.length(); i < width; i++) {
        cout << ' ';
    }
}

// Mode 3: race the trie against brute force and incremental filter on one query
void race(const string* words, int n) {
    cout << "Dictionary size (Enter = " << n << " real words, bigger adds synthetic): ";
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
    for (int i = 0; i < size; i++) {
        trie.insert(dict[i]);
    }

    // one guess against a random answer, same style as the benchmark
    string answer = dict[rand() % size];
    string guess = bestGuess(dict, size);
    string feedback = getFeedback(guess, answer);
    Constraints c;
    c.update(guess, feedback);
    cout << "Answer: " << answer << "   Guess: " << guess << "   Feedback: " << feedback << "\n\n";

    string* out1 = new string[size];
    string* out2 = new string[size];
    string* incremental = new string[size];
    for (int i = 0; i < size; i++) {
        incremental[i] = dict[i];
    }

    long long visited = 0;
    Clock::time_point start = Clock::now();
    int trieCount = trie.search(c, out1, visited);
    Clock::time_point end = Clock::now();
    double trieTime = microsBetween(start, end);

    start = Clock::now();
    int bruteCount = bruteForce(dict, size, c, out2);
    end = Clock::now();
    double bruteTime = microsBetween(start, end);

    // incremental filter has no previous turn here, so it checks the full copy
    start = Clock::now();
    int incCount = incrementalFilter(incremental, size, c);
    end = Clock::now();
    double incTime = microsBetween(start, end);

    // aligned table
    printCol("Method", 14); printCol("Matches", 10); printCol("Time (us)", 14); cout << "Work\n";
    printCol("Trie", 14); printCol(to_string(trieCount), 10);
        printCol(to_string(trieTime), 14); cout << visited << " nodes visited\n";
    printCol("Brute force", 14); printCol(to_string(bruteCount), 10);
        printCol(to_string(bruteTime), 14); cout << size << " words checked\n";
    printCol("Incremental", 14); printCol(to_string(incCount), 10);
        printCol(to_string(incTime), 14); cout << size << " words checked\n";

    // bar chart of the three times (longer bar = slower)
    double maxTime = trieTime;
    if (bruteTime > maxTime) maxTime = bruteTime;
    if (incTime > maxTime) maxTime = incTime;
    cout << "\n";
    drawBar("Trie       ", trieTime, maxTime, 40);
    drawBar("Brute force", bruteTime, maxTime, 40);
    drawBar("Incremental", incTime, maxTime, 40);

    if (trieTime > 0) {
        cout << "\nTrie was " << bruteTime / trieTime << "x faster than brute force, "
             << incTime / trieTime << "x faster than incremental.\n";
    }

    delete[] out1;
    delete[] out2;
    delete[] incremental;
    delete[] dict;
}

int main(int argc, char* argv[]) {
    string filename = "data/words.txt";

    // Parse argv: --no-color turns off ANSI colors; any other arg is a word-list path.
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--no-color") {
            setColorEnabled(false);
        } else {
            filename = arg;
        }
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
        cout << "2) Auto play a game\n";
        cout << "3) Race trie vs brute force vs incremental\n";
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
            race(words, n);
        }
    }

    delete[] words;
    return 0;
}
