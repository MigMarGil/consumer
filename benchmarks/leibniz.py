# Leibniz series for pi: 10M iterations
pi = 0.0
sign = 1.0
i = 1
n = 10_000_000

while i < n * 2:
    pi += sign * (4.0 / i)
    sign *= -1.0
    i += 2

print(pi)
