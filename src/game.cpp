#include "game.h"

// start a new game with no turns
void resetGame(Game& g) {
    g.turns = 0;
}

// add one (guess, feedback) pair; returns false if the game is already full
bool addTurn(Game& g, const std::string& guess, const std::string& fb) {
    if (g.turns >= MAX_TURNS) {
        return false;
    }
    g.guesses[g.turns] = guess;
    g.feedback[g.turns] = fb;
    g.turns++;
    return true;
}

// remove the last turn (does nothing if there are none)
void undoTurn(Game& g) {
    if (g.turns > 0) {
        g.turns--;
    }
}

// build Constraints from scratch: reset, then update for each turn so far
void buildConstraints(const Game& g, Constraints& c) {
    c.reset();
    for (int i = 0; i < g.turns; i++) {
        c.update(g.guesses[i], g.feedback[i]);
    }
}

// true if s is exactly 5 lowercase letters
bool isValidWord(const std::string& s) {
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

// true if s is exactly 5 characters, each g / y / b
bool isValidFeedback(const std::string& s) {
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
