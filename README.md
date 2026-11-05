<div align="center">
  <img src="CONSUmer.png" alt="Consumer Logo" width="400">
</div>

# Consumer

**Write like Python. Run like C.**

Consumer is a numerical scripting language that compiles to native code via LLVM. No interpreter. No GIL. No dependencies at runtime. Just a JIT that turns your `.cons` script into machine code before the first loop iteration.

## Why

Python is great for numerical scripts until it isn't. A loop that takes 1.7 seconds in Python takes 42 milliseconds when JIT-compiled. Consumer gives you Python-like syntax with near-C speed, without rewriting your script in C.

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
    ./consumer examples/prime_sieve.cons
    ./consumer examples/monte_carlo_pi.cons
    ./consumer examples/trapezoidal_integration.cons

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

Operators: `+ - * / %`, comparisons `< > <= >=`, parentheses.

## Benchmark

    cd benchmarks
    ./bench.sh

Leibniz series for pi, 10 million iterations:

| Language | Time |
|---|---|
| Python 3.12 | 1741 ms |
| **Consumer (JIT)** | **42 ms** |
| C (-O2) | 12 ms |

*Measured on a 2024 laptop running WSL2. Run `bench.sh` to reproduce on yours.*

Consumer is **41x faster than Python** and within 3.5x of optimized C.

## Examples

- `fibonacci.cons` — first 15 Fibonacci numbers
- `prime_sieve.cons` — count primes below 1000
- `monte_carlo_pi.cons` — estimate pi with 1M samples
- `trapezoidal_integration.cons` — integrate x^2 from 0 to 1

## Status

v0.1. Numbers and control flow only. Strings, functions, and arrays are on the roadmap.

## License

MIT
