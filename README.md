# Consumer

Un lenguaje de scripting numérico que compila a código nativo vía LLVM.

## Requisitos

- GCC
- LLVM (con `llvm-config` en el PATH)

En Ubuntu/WSL: `sudo apt install llvm-dev clang`

## Compilar

    make

## Ejecutar

    ./consumer examples/fibonacci.cons

## Sintaxis

    let x = expr
    print(expr)
    while cond: {
      ...
    }
    if cond: {
      ...
    } else: {
      ...
    }

## Ejemplo

    let a = 0
    let b = 1
    let i = 0

    while i < 15: {
      print(a)
      let temp = a + b
      let a = b
      let b = temp
      let i = i + 1
    }
