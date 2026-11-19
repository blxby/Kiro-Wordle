#include <iostream>
#include "tui.h"

using namespace std;

// ANSI escape codes are special text sequences the terminal reads as formatting
// instead of printing. "\033" is the escape character; "\033[42m" sets a green
// background and "\033[0m" turns all formatting back off.
static const char* GREEN  = "\033[1;30;42m";   // bold black text on green
static const char* YELLOW = "\033[1;30;43m";   // bold black text on yellow
static const char* GREY   = "\033[1;37;100m";  // bold white text on dark grey
static const char* KEYUP  = "\033[1;30;47m";   // unused key: black text on light grey
static const char* RESET  = "\033[0m";

// whether to use the colors above; false draws plain ASCII instead
static bool colorOn = true;

void setColorEnabled(bool on) {
    colorOn = on;
}

// pick the color code for a tile state
static const char* colorFor(char state) {
    if (state == 'g') return GREEN;
    if (state == 'y') return YELLOW;
    return GREY;
}

// Draw one tile for a letter. With color: a colored block like " C ".
// Without color: [C] green, (C) yellow, " C " grey, "   " empty.
void drawTile(char letter, char state) {
    char up = letter;
    if (up >= 'a' && up <= 'z') {
        up = up - 'a' + 'A';   // tiles show uppercase letters
    }

    if (state == ' ') {
        if (colorOn) {
            cout << "\033[1;37;40m     " << RESET;   // empty: dark block
        } else {
            cout << "[ ] ";
        }
        return;
    }

    if (colorOn) {
        cout << colorFor(state) << "  " << up << "  " << RESET;
    } else {
        // ASCII fallback: brackets stand in for colors
        char open = ' ';
        char close = ' ';
        if (state == 'g') { open = '['; close = ']'; }
        else if (state == 'y') { open = '('; close = ')'; }
        cout << open << up << close << ' ';
    }
}

void drawBoard(const Game& g) {
    for (int row = 0; row < 6; row++) {
        cout << "  ";
        if (row < g.turns) {
            for (int i = 0; i < WORD_LEN; i++) {
                drawTile(g.guesses[row][i], g.feedback[row][i]);
                cout << ' ';
            }
        } else {
            for (int i = 0; i < WORD_LEN; i++) {
                drawTile(' ', ' ');
                cout << ' ';
            }
        }
        cout << "\n";
    }
}

void drawKeyboard(const Game& g) {
    // rank each letter by the best state seen: green 3, yellow 2, grey 1, unused 0
    int state[26];
    for (int i = 0; i < 26; i++) {
        state[i] = 0;
    }
    for (int t = 0; t < g.turns; t++) {
        for (int i = 0; i < WORD_LEN; i++) {
            int letter = g.guesses[t][i] - 'a';
            if (letter < 0 || letter >= 26) {
                continue;
            }
            char fb = g.feedback[t][i];
            int rank = 1;
            if (fb == 'g') rank = 3;
            else if (fb == 'y') rank = 2;
            if (rank > state[letter]) {
                state[letter] = rank;
            }
        }
    }

    // standard QWERTY rows
    const char* rows[3] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    for (int r = 0; r < 3; r++) {
        cout << "  ";
        if (r == 1) cout << " ";      // small indent, like a real keyboard
        if (r == 2) cout << "   ";
        for (int i = 0; rows[r][i] != '\0'; i++) {
            char letter = rows[r][i];
            int rank = state[letter - 'a'];
            char up = letter - 'a' + 'A';
            if (colorOn) {
                const char* code = KEYUP;
                if (rank == 3) code = GREEN;
                else if (rank == 2) code = YELLOW;
                else if (rank == 1) code = GREY;
                cout << code << ' ' << up << ' ' << RESET << ' ';
            } else {
                // rank shown as a trailing mark when colors are off
                char mark = ' ';
                if (rank == 3) mark = '=';
                else if (rank == 2) mark = '~';
                else if (rank == 1) mark = 'x';
                cout << up << mark << ' ';
            }
        }
        cout << "\n";
    }
}

void drawBar(const std::string& label, double value, double maxValue, int width) {
    int filled = 0;
    if (maxValue > 0 && value > 0) {
        filled = (int)((value / maxValue) * width + 0.5);
    }
    if (filled > width) {
        filled = width;
    }
    cout << label << " ";
    for (int i = 0; i < filled; i++) {
        cout << '#';
    }
    for (int i = filled; i < width; i++) {
        cout << '.';
    }
    cout << "  " << value << " us\n";
}

void clearScreen() {
    // "\033[2J" clears the screen, "\033[H" moves the cursor home (top-left)
    if (colorOn) {
        cout << "\033[2J\033[H";
    } else {
        cout << "\n\n";   // just add space when colors are off
    }
}
