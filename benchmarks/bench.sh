#!/bin/bash
set -e

echo "Building Consumer..."
cd ..
make > /dev/null
cd benchmarks

echo ""
echo "=== Leibniz pi, 10M iterations ==="
echo ""

echo "Python 3:"
time python3 leibniz.py

echo ""
echo "Consumer (JIT):"
time ../consumer leibniz.cons

echo ""
echo "C (-O2):"
gcc -O2 -o leibniz_c leibniz.c
time ./leibniz_c

rm -f leibniz_c
