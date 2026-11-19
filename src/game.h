#pragma once
#include <string>
#include "constraints.h"

const int MAX_TURNS = 20;   // plenty of rows for any solve or auto play

// A game is just the list of (guess, feedback) pairs played so far.
// The current candidates come from replaying every pair into a fresh
// Constraints, so undo is simply "drop the last pair".
struct Game {
    std::string guesses[MAX_TURNS];
    std::string feedback[MAX_TURNS];
    int turns;
};

// start a new game with no turns
void resetGame(Game& g);

// add one (guess, feedback) pair; returns false if the game is already full
bool addTurn(Game& g, const std::string& guess, const std::string& fb);

// remove the last turn (does nothing if there are none)
void undoTurn(Game& g);

// build Constraints from scratch: reset, then update for each turn so far
void buildConstraints(const Game& g, Constraints& c);

// true if s is exactly 5 lowercase letters
bool isValidWord(const std::string& s);

// true if s is exactly 5 characters, each g / y / b
bool isValidFeedback(const std::string& s);
