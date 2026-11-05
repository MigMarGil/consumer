# Contributing to Consumer

Thanks for your interest. Consumer is a small project with a clear goal: fast numerical scripts with Python-like syntax.

## Build

    make

## Test

    ./consumer examples/fibonacci.cons

## Areas that need help

- `%` modulo operator
- `for` loops
- User-defined functions (`fn name(args) { ... }`)
- String literals
- Better error messages (line numbers, column numbers)
- Unit tests for the lexer and parser

## How to submit a change

1. Fork the repo.
2. Create a branch: `git checkout -b feature/modulo`.
3. Make the change.
4. Verify `make` and `./consumer examples/fibonacci.cons` still work.
5. Open a pull request with a one-line description of what it does and why.

Keep changes small. One feature per PR. If you are unsure whether a change fits, open an issue first.
