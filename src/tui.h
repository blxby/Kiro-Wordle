#pragma once
#include <string>
#include "game.h"

// Turn ANSI colors on or off. When off (the --no-color flag), tiles are drawn
// with plain ASCII brackets instead of colored backgrounds.
void setColorEnabled(bool on);

// Draw one tile for a letter. state is 'g' green, 'y' yellow, 'b' grey,
// or ' ' for an empty tile.
void drawTile(char letter, char state);

// Draw the whole board: one row per turn played, then blank rows up to 6.
void drawBoard(const Game& g);

// Draw a QWERTY keyboard, each letter colored by the best state known for it
// (green beats yellow beats grey beats unused).
void drawKeyboard(const Game& g);

// Draw one labeled horizontal bar for the race chart. value sets the filled
// length relative to maxValue over the given width in characters.
void drawBar(const std::string& label, double value, double maxValue, int width);

// Clear the screen and move the cursor to the top-left.
void clearScreen();
