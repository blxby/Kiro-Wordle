#include "trie.h"

Node::Node() {
    for (int i = 0; i < 26; i++) {
        children[i] = nullptr;
    }
    isEnd = false;
    mask = 0;
}

Trie::Trie() {
    root = new Node();
    nodeCount = 1;
    wordCount = 0;
}

Trie::~Trie() {
    deleteNodes(root);
}

void Trie::deleteNodes(Node* node) {
    if (node == nullptr) {
        return;
    }
    for (int i = 0; i < 26; i++) {
        deleteNodes(node->children[i]);
    }
    delete node;
}

void Trie::insert(const std::string& word) {
    int len = word.length();

    // suffix[i] = mask of every letter in word[i..end]
    unsigned int suffix[64];
    suffix[len] = 0;
    for (int i = len - 1; i >= 0; i--) {
        suffix[i] = suffix[i + 1] | (1u << (word[i] - 'a'));
    }

    Node* node = root;
    for (int i = 0; i < len; i++) {
        // every letter from here to the end of the word is "below" this node
        node->mask = node->mask | suffix[i];

        int letter = word[i] - 'a';
        if (node->children[letter] == nullptr) {
            node->children[letter] = new Node();
            nodeCount++;
        }
        node = node->children[letter];
    }

    if (!node->isEnd) {
        node->isEnd = true;
        wordCount++;
    }
}

bool Trie::lookup(const std::string& word) const {
    Node* node = root;
    for (int i = 0; i < (int)word.length(); i++) {
        int letter = word[i] - 'a';
        if (node->children[letter] == nullptr) {
            return false;
        }
        node = node->children[letter];
    }
    return node->isEnd;
}

int Trie::search(const Constraints& c, std::string* out, long long& nodesVisited) const {
    SearchState s;
    s.c = &c;
    for (int letter = 0; letter < 26; letter++) {
        s.pathCount[letter] = 0;
    }
    s.out = out;
    s.found = 0;
    s.nodesVisited = 0;

    // work out once which letters are needed (from greens and yellows)
    unsigned int neededMask = 0;
    int stillNeeded = 0;
    for (int letter = 0; letter < 26; letter++) {
        if (c.minCount[letter] > 0) {
            neededMask = neededMask | (1u << letter);
            stillNeeded += c.minCount[letter];
        }
    }

    searchHelper(root, 0, neededMask, stillNeeded, s);
    nodesVisited = s.nodesVisited;
    return s.found;
}

void Trie::searchHelper(Node* node, int depth, unsigned int neededMask, int stillNeeded,
                        SearchState& s) const {
    s.nodesVisited++;
    const Constraints& c = *s.c;

    // Pruning 1: not enough positions left to fit the letters we still need
    if (stillNeeded > WORD_LEN - depth) {
        return;
    }
    // Pruning 2: some needed letter doesn't appear anywhere below this node
    if ((node->mask & neededMask) != neededMask) {
        return;
    }

    // reached a full-length word
    if (depth == WORD_LEN) {
        if (node->isEnd) {
            s.out[s.found] = std::string(s.path, WORD_LEN);
            s.found++;
        }
        return;
    }

    // green position: only one child is possible
    int first = 0;
    int last = 25;
    if (c.fixed[depth] != -1) {
        first = c.fixed[depth];
        last = c.fixed[depth];
    }

    for (int letter = first; letter <= last; letter++) {
        Node* child = node->children[letter];
        if (child == nullptr) {
            continue;
        }
        if (c.banned[depth][letter]) {
            continue;
        }
        if (s.pathCount[letter] + 1 > c.maxCount[letter]) {
            continue;   // would use too many of this letter
        }

        // does this letter fill one of the needed slots?
        unsigned int newMask = neededMask;
        int newNeeded = stillNeeded;
        if (s.pathCount[letter] < c.minCount[letter]) {
            newNeeded--;
            if (s.pathCount[letter] + 1 == c.minCount[letter]) {
                newMask = newMask & ~(1u << letter);   // no longer need this letter
            }
        }

        s.path[depth] = 'a' + letter;
        s.pathCount[letter]++;
        searchHelper(child, depth + 1, newMask, newNeeded, s);
        s.pathCount[letter]--;   // undo before trying the next letter
    }
}

int Trie::getNodeCount() const {
    return nodeCount;
}

int Trie::getWordCount() const {
    return wordCount;
}
