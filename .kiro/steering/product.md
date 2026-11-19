---
inclusion: always
---

# Product: Wordle Solver (CSC 212 Final Project, Fall 2026)

Solo project by Tony Livingston, University of Rhode Island. Instructor: Prof. Marco Alvarez.

## What it is

A Wordle solver in C++ that finds every dictionary word consistent with all feedback so far.
The core data structure is a **trie** (prefix tree) with a constraint-pruned depth-first search.
It is compared against two baselines that use identical constraint logic:

1. **Brute force**: scan every word in a plain array every turn.
2. **Incremental filter**: re-check only the words that survived the previous turn.

## Who uses it and why

- Tony, demoing live at the **CS Showcase (Dec 18)**. The demo must show the solver working,
  show the numbers (trie vs. baselines), and Tony must be able to explain every line of code.
- Visitors at the showcase, who should be able to play with it themselves.

## Grading facts that shape every decision

- The course allows AI tools, but **Tony must be able to explain every line submitted**.
  Prefer simple, readable code over clever code. Explain any new concept in a short comment.
- The **core data structure (trie) must be written by Tony**. Do not rewrite or "improve"
  `src/trie.*` or `src/constraints.*` unless Tony explicitly asks. Interfaces are built *around*
  the existing public functions.
- Commit history counts toward the grade, so work in small, separate steps (one task at a time).
- The report has an "AI usage" section, so record what AI generated in `AI_LOG.md` (see tech.md).

## Current feature work

Two interactive front ends on top of the existing solver (see `.kiro/specs/interactive-interfaces/`):

1. A **colored terminal UI** with Wordle-style tiles (pure C++).
2. A **web page** front end served by a small C++ HTTP server.
