#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include "httplib.h"          // cpp-httplib: single-header HTTP server (MIT license)
#include "constraints.h"
#include "trie.h"
#include "baselines.h"
#include "solver.h"
#include "game.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

// microseconds between two time points
double microsBetween(Clock::time_point start, Clock::time_point end) {
    return chrono::duration<double, micro>(end - start).count();
}

// Loaded once at startup and only read afterward. search() is const, so many
// request threads can share this trie safely.
static Trie gTrie;
static string* gWords = nullptr;
static int gWordCount = 0;

// Wrap a string in double quotes for JSON. Our words and feedback are only
// a-z and g/y/b, so there are no characters that need escaping (noted on purpose).
string jsonString(const string& s) {
    return "\"" + s + "\"";
}

// Split a comma-separated list into parts[]. Returns the number of parts,
// capped at MAX_TURNS. An empty input gives zero parts.
int splitCommas(const string& s, string parts[MAX_TURNS]) {
    if (s.length() == 0) {
        return 0;
    }
    int count = 0;
    string current = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] == ',') {
            if (count < MAX_TURNS) {
                parts[count++] = current;
            }
            current = "";
        } else {
            current += s[i];
        }
    }
    if (count < MAX_TURNS) {
        parts[count++] = current;
    }
    return count;
}

// Build a Game from the guesses/feedback query parameters. Returns false and
// sets err if the counts differ or any entry is invalid.
bool parseGame(const string& guessesParam, const string& feedbackParam,
               Game& g, string& err) {
    resetGame(g);
    string gParts[MAX_TURNS];
    string fParts[MAX_TURNS];
    int gc = splitCommas(guessesParam, gParts);
    int fc = splitCommas(feedbackParam, fParts);

    if (gc != fc) {
        err = "guesses and feedback counts differ";
        return false;
    }
    for (int i = 0; i < gc; i++) {
        if (!isValidWord(gParts[i])) {
            err = "bad guess: " + gParts[i];
            return false;
        }
        if (!isValidFeedback(fParts[i])) {
            err = "bad feedback: " + fParts[i];
            return false;
        }
        addTurn(g, gParts[i], fParts[i]);
    }
    return true;
}

// send a 400 error response as JSON
void sendError(httplib::Response& res, const string& message) {
    res.status = 400;
    res.set_content("{\"error\":" + jsonString(message) + "}", "application/json");
}

// GET /api/info -> word and node counts of the loaded trie
void handleInfo(const httplib::Request&, httplib::Response& res) {
    string json = "{\"words\":" + to_string(gTrie.getWordCount())
                + ",\"nodes\":" + to_string(gTrie.getNodeCount()) + "}";
    res.set_content(json, "application/json");
}

// GET /api/feedback?guess=..&answer=.. -> the feedback the game would give
void handleFeedback(const httplib::Request& req, httplib::Response& res) {
    string guess = req.get_param_value("guess");
    string answer = req.get_param_value("answer");
    if (!isValidWord(guess) || !isValidWord(answer)) {
        sendError(res, "guess and answer must be 5 lowercase letters");
        return;
    }
    string fb = getFeedback(guess, answer);
    res.set_content("{\"feedback\":" + jsonString(fb) + "}", "application/json");
}

// GET /api/solve?guesses=..&feedback=.. -> candidates (capped 100), suggestion, timing
void handleSolve(const httplib::Request& req, httplib::Response& res) {
    Game g;
    string err;
    if (!parseGame(req.get_param_value("guesses"), req.get_param_value("feedback"), g, err)) {
        sendError(res, err);
        return;
    }

    Constraints c;
    buildConstraints(g, c);

    string* out = new string[gWordCount];
    long long visited = 0;
    Clock::time_point start = Clock::now();
    int count = gTrie.search(c, out, visited);
    Clock::time_point end = Clock::now();
    double us = microsBetween(start, end);

    int show = count;
    if (show > 100) {
        show = 100;   // cap the candidate list the client receives
    }

    string json = "{\"count\":" + to_string(count) + ",\"candidates\":[";
    for (int i = 0; i < show; i++) {
        if (i > 0) json += ",";
        json += jsonString(out[i]);
    }
    json += "]";

    string suggestion = "";
    if (count > 0) {
        suggestion = bestGuess(out, count);
    }
    json += ",\"suggestion\":" + jsonString(suggestion);
    json += ",\"trie_us\":" + to_string(us);
    json += ",\"nodes\":" + to_string(visited) + "}";

    res.set_content(json, "application/json");
    delete[] out;
}

// GET /api/autoplay?answer=.. -> a full game the solver plays (answer optional)
void handleAutoplay(const httplib::Request& req, httplib::Response& res) {
    string answer = req.get_param_value("answer");
    if (answer == "") {
        answer = gWords[rand() % gWordCount];
    } else if (!isValidWord(answer) || !gTrie.lookup(answer)) {
        sendError(res, "answer must be a 5-letter word from the list");
        return;
    }

    Game g;
    resetGame(g);
    string* out = new string[gWordCount];

    string json = "{\"answer\":" + jsonString(answer) + ",\"turns\":[";
    bool first = true;
    for (int turn = 1; turn <= MAX_TURNS; turn++) {
        Constraints c;
        buildConstraints(g, c);
        long long visited = 0;
        Clock::time_point start = Clock::now();
        int count = gTrie.search(c, out, visited);
        Clock::time_point end = Clock::now();
        double us = microsBetween(start, end);

        string guess = bestGuess(out, count);
        string fb = getFeedback(guess, answer);
        addTurn(g, guess, fb);

        if (!first) json += ",";
        first = false;
        json += "{\"guess\":" + jsonString(guess)
              + ",\"feedback\":" + jsonString(fb)
              + ",\"left\":" + to_string(count)
              + ",\"us\":" + to_string(us) + "}";

        if (fb == "ggggg") {
            break;
        }
    }
    json += "]}";

    res.set_content(json, "application/json");
    delete[] out;
}

// GET /api/race?size=N -> trie vs brute force vs incremental on one query
void handleRace(const httplib::Request& req, httplib::Response& res) {
    string sizeParam = req.get_param_value("size");
    int size = gWordCount;
    if (sizeParam != "") {
        size = atoi(sizeParam.c_str());
    }
    if (size < 1 || size > 200000) {
        sendError(res, "size must be between 1 and 200000");
        return;
    }

    string* dict = makeDictionary(gWords, gWordCount, size);
    int synthetic = 0;
    if (size > gWordCount) {
        synthetic = size - gWordCount;
    }

    // build a local trie for this dictionary size
    Trie trie;
    for (int i = 0; i < size; i++) {
        trie.insert(dict[i]);
    }

    // one guess against a random answer, benchmark style
    string answer = dict[rand() % size];
    string guess = bestGuess(dict, size);
    string fb = getFeedback(guess, answer);
    Constraints c;
    c.update(guess, fb);

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

    start = Clock::now();
    int incCount = incrementalFilter(incremental, size, c);
    end = Clock::now();
    double incTime = microsBetween(start, end);

    string json = "{\"size\":" + to_string(size)
                + ",\"synthetic\":" + to_string(synthetic)
                + ",\"guess\":" + jsonString(guess)
                + ",\"feedback\":" + jsonString(fb)
                + ",\"trie\":{\"matches\":" + to_string(trieCount)
                    + ",\"us\":" + to_string(trieTime)
                    + ",\"work\":" + to_string(visited) + "}"
                + ",\"brute\":{\"matches\":" + to_string(bruteCount)
                    + ",\"us\":" + to_string(bruteTime)
                    + ",\"work\":" + to_string(size) + "}"
                + ",\"incremental\":{\"matches\":" + to_string(incCount)
                    + ",\"us\":" + to_string(incTime)
                    + ",\"work\":" + to_string(size) + "}}";

    res.set_content(json, "application/json");
    delete[] out1;
    delete[] out2;
    delete[] incremental;
    delete[] dict;
}

int main(int argc, char* argv[]) {
    // optional port argument: ./server 9000
    int port = 8080;
    if (argc > 1) {
        int p = atoi(argv[1]);
        if (p > 0) {
            port = p;
        }
    }

    gWords = loadWords("data/words.txt", gWordCount);
    if (gWords == nullptr || gWordCount == 0) {
        cout << "Could not load data/words.txt (run from the project folder)\n";
        return 1;
    }
    for (int i = 0; i < gWordCount; i++) {
        gTrie.insert(gWords[i]);
    }
    srand(time(0));

    httplib::Server svr;   // a Server routes URL paths to handler functions

    // a route maps a URL path (GET here) to a function
    svr.Get("/api/info", handleInfo);
    svr.Get("/api/feedback", handleFeedback);
    svr.Get("/api/solve", handleSolve);
    svr.Get("/api/autoplay", handleAutoplay);
    svr.Get("/api/race", handleRace);

    // serve the static web page files from ./web at the site root
    svr.set_mount_point("/", "./web");

    cout << "Loaded " << gWordCount << " words, " << gTrie.getNodeCount() << " trie nodes\n";
    cout << "Open http://localhost:" << port << "\n";
    // listen blocks until the server stops; it returns false if the port is taken
    if (!svr.listen("0.0.0.0", port)) {
        cout << "Could not bind to port " << port << " (is it already in use?)\n";
        delete[] gWords;
        return 1;
    }

    delete[] gWords;
    return 0;
}
