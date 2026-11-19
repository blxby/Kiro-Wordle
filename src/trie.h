#pragma once
#include <string>
#include "constraints.h"

struct Node {
    Node* children[26];
    bool isEnd;
    unsigned int mask;  // bit i set if letter 'a'+i appears anywhere below this node

    Node();
};

// Everything the recursive search needs to carry along, kept in one place
// so the helper function doesn't need a long parameter list.
struct SearchState {
    const Constraints* c;   // the constraints being searched for
    char path[WORD_LEN];    // letters chosen so far
    int pathCount[26];      // how many of each letter are in path
    std::string* out;       // where matches are written
    int found;              // number of matches so far
    long long nodesVisited; // work counter for the benchmark
};

class Trie {
public:
    Trie();
    ~Trie();

    void insert(const std::string& word);
    bool lookup(const std::string& word) const;

    // Find every word that matches the constraints.
    // Matches are written into out (must have room for every word in the trie).
    // Returns how many were found. nodesVisited is set to the number of nodes the search touched.
    int search(const Constraints& c, std::string* out, long long& nodesVisited) const;

    int getNodeCount() const;
    int getWordCount() const;

private:
    Node* root;
    int nodeCount;
    int wordCount;

    void deleteNodes(Node* node);

    // recursive depth-first part of search
    // neededMask: bit set for each letter we still need to place
    // stillNeeded: total number of letters still needed (counts duplicates)
    void searchHelper(Node* node, int depth, unsigned int neededMask, int stillNeeded,
                      SearchState& s) const;
};
