# Consumer

**Write like Python. Run like C.**

Consumer is a numerical scripting language that compiles to native code via LLVM. No interpreter. No GIL. No dependencies at runtime. Just a JIT that turns your `.cons` script into machine code before the first loop iteration.

## Why

Python is great for numerical scripts until it isn't. A Monte Carlo simulation that takes 40 seconds in Python takes milliseconds when JIT-compiled. Consumer gives you Python-like syntax with C-like speed, without rewriting your script in C.

## Install

Requirements: GCC and LLVM (with `llvm-config` in PATH).

On Ubuntu/WSL:

    sudo apt install llvm-dev clang

Then:

    git clone https://github.com/MigMarGil/consumer.git
    cd consumer
    make

## Run

    ./consumer examples/fibonacci.cons

## Syntax

    let x = 10
    let y = 3.5
    print(x + y)

    while i < 100: {
      print(i)
      let i = i + 1
    }

    if x > 5: {
      print(1)
    } else: {
      print(0)
    }

## Benchmark

    cd benchmarks
    ./bench.sh

| Calculation | Time |
|---|---|
| Leibniz pi, 10M iterations (Python 3.12) | ~1.8 s |
| **Leibniz pi, 10M iterations (Consumer)** | **~15 ms** |
| Leibniz pi, 10M iterations (C -O2) | ~12 ms |

*Numbers on a 2024 laptop. Run `bench.sh` to reproduce on yours.*

## Status

v0.1. Not a general-purpose language. Numbers and control flow only. Strings, functions, and arrays are on the roadmap.

## License

MIT
