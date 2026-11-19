// Benchmark: prints CSV you can paste into Sheets.
// Build with -O2 (make bench) and run from the project folder: ./bench
#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>
#include "../src/constraints.h"
#include "../src/trie.h"
#include "../src/baselines.h"
#include "../src/solver.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

const int ANSWERS = 21;   // random answers per dictionary size
const int REPEATS = 5;    // times each query is repeated

double microsBetween(Clock::time_point start, Clock::time_point end) {
    return chrono::duration<double, micro>(end - start).count();
}

// median of an array (sorts it with insertion sort)
double median(double* a, int n) {
    for (int i = 1; i < n; i++) {
        double key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
    return a[n / 2];
}

// Section 1 and 2: query time and work vs dictionary size, plus memory
void benchSizes(const string* real, int realCount) {
    int sizes[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000};
    int numSizes = 7;

    cout << "# Query time vs dictionary size (second guess of a game, median of "
         << ANSWERS * REPEATS << " runs, microseconds)\n";
    cout << "size,synthetic_words,trie_us,brute_us,incremental_us,"
            "trie_nodes_visited,brute_words_checked,incremental_words_checked,matches\n";

    // memory results are saved and printed after
    int memNodes[7];

    for (int s = 0; s < numSizes; s++) {
        int size = sizes[s];
        string* dict = makeDictionary(real, realCount, size);

        Trie trie;
        for (int i = 0; i < size; i++) {
            trie.insert(dict[i]);
        }
        memNodes[s] = trie.getNodeCount();

        string* out = new string[size];
        string* survivors = new string[size];
        string* work = new string[size];

        // same opening guess for every game at this size
        string opener = bestGuess(dict, size);

        double trieTimes[ANSWERS * REPEATS];
        double bruteTimes[ANSWERS * REPEATS];
        double incTimes[ANSWERS * REPEATS];
        double visitedList[ANSWERS];
        double incCheckedList[ANSWERS];
        double matchList[ANSWERS];
        int t = 0;

        for (int a = 0; a < ANSWERS; a++) {
            string answer = dict[rand() % size];

            // turn 1
            Constraints c;
            c.update(opener, getFeedback(opener, answer));
            int survivorCount = bruteForce(dict, size, c, survivors);

            // turn 2: the solver's next guess
            string guess2 = bestGuess(survivors, survivorCount);
            c.update(guess2, getFeedback(guess2, answer));

            long long visited = 0;
            int matches = 0;
            for (int r = 0; r < REPEATS; r++) {
                Clock::time_point start = Clock::now();
                matches = trie.search(c, out, visited);
                Clock::time_point end = Clock::now();
                trieTimes[t] = microsBetween(start, end);

                start = Clock::now();
                bruteForce(dict, size, c, out);
                end = Clock::now();
                bruteTimes[t] = microsBetween(start, end);

                // incremental only re-checks the turn-1 survivors
                // (copy them first so every repeat starts the same; copy is not timed)
                for (int i = 0; i < survivorCount; i++) {
                    work[i] = survivors[i];
                }
                start = Clock::now();
                incrementalFilter(work, survivorCount, c);
                end = Clock::now();
                incTimes[t] = microsBetween(start, end);

                t++;
            }
            visitedList[a] = visited;
            incCheckedList[a] = survivorCount;
            matchList[a] = matches;
        }

        int synthetic = 0;
        if (size > realCount) {
            synthetic = size - realCount;
        }

        cout << size << "," << synthetic << ","
             << median(trieTimes, t) << ","
             << median(bruteTimes, t) << ","
             << median(incTimes, t) << ","
             << median(visitedList, ANSWERS) << ","
             << size << ","
             << median(incCheckedList, ANSWERS) << ","
             << median(matchList, ANSWERS) << "\n";

        delete[] out;
        delete[] survivors;
        delete[] work;
        delete[] dict;
    }

    cout << "\n# Memory vs dictionary size (bytes)\n";
    cout << "size,trie_nodes,trie_bytes,flat_char_array_bytes,string_array_bytes\n";
    for (int s = 0; s < numSizes; s++) {
        long long trieBytes = (long long)memNodes[s] * sizeof(Node);
        long long flatBytes = (long long)sizes[s] * WORD_LEN;          // char words[N][5]
        long long stringBytes = (long long)sizes[s] * sizeof(string); // string words[N]
        cout << sizes[s] << "," << memNodes[s] << "," << trieBytes << ","
             << flatBytes << "," << stringBytes << "\n";
    }
}

// Section 3: play full games on the real word list with each method
void benchGames(const string* words, int n) {
    const int GAMES = 500;

    Trie trie;
    for (int i = 0; i < n; i++) {
        trie.insert(words[i]);
    }

    string* answers = new string[GAMES];
    for (int g = 0; g < GAMES; g++) {
        answers[g] = words[rand() % n];
    }

    string* cands = new string[n];

    cout << "\n# Full game simulation on the real list (" << n << " words, "
         << GAMES << " games; time counts only the candidate searches)\n";
    cout << "method,games,average_guesses,max_guesses,games_over_6,total_search_ms\n";

    for (int method = 0; method < 3; method++) {
        int totalGuesses = 0;
        int maxGuesses = 0;
        int over6 = 0;
        double totalTime = 0;

        for (int g = 0; g < GAMES; g++) {
            Constraints c;
            int count = 0;

            // incremental starts from the whole list
            if (method == 2) {
                for (int i = 0; i < n; i++) {
                    cands[i] = words[i];
                }
                count = n;
            }

            int turn = 0;
            while (true) {
                turn++;
                Clock::time_point start = Clock::now();
                if (method == 0) {
                    long long visited = 0;
                    count = trie.search(c, cands, visited);
                } else if (method == 1) {
                    count = bruteForce(words, n, c, cands);
                } else {
                    count = incrementalFilter(cands, count, c);
                }
                Clock::time_point end = Clock::now();
                totalTime += microsBetween(start, end);

                string guess = bestGuess(cands, count);
                string feedback = getFeedback(guess, answers[g]);
                if (feedback == "ggggg" || turn == 20) {
                    break;
                }
                c.update(guess, feedback);
            }

            totalGuesses += turn;
            if (turn > maxGuesses) {
                maxGuesses = turn;
            }
            if (turn > 6) {
                over6++;
            }
        }

        string name = "trie";
        if (method == 1) {
            name = "brute_force";
        } else if (method == 2) {
            name = "incremental";
        }
        cout << name << "," << GAMES << "," << (double)totalGuesses / GAMES << ","
             << maxGuesses << "," << over6 << "," << totalTime / 1000.0 << "\n";
    }

    delete[] cands;
    delete[] answers;
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
    cout << "# Real words loaded: " << n << " from " << filename << "\n\n";

    srand(212);   // fixed seed so runs are repeatable

    benchSizes(words, n);
    benchGames(words, n);

    delete[] words;
    return 0;
}
