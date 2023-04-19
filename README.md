## The Little Language That Could

qdbp is small. Really, really small. The language has just 2 keywords and 7 core constructs(along with full type inference and a little bit of syntax sugar to sweeten the deal). In fact here is a small program, implementing a basic boolean object, that demonstrates *every single primitive* of the language:
```ocaml
true := {
  BoolVal [#True{}]
  EagerIf [then else|
    self BoolVal.
      True? [then]
      False? [else].
  ]
  Negate [
    self EagerIf
      then: { self BoolVal[#False{}] }
      else: { self BoolVal[#True{}] }.
  ]
}
false := (true Negate)
false Negate. Negate. Negate. Negate.
```
As a comparison, Go 1.20 has 25 keywords, Python 3.10 has 38, C++14 has 84, and Lua 5.4 has 22. Neither keyword nor feature count are sufficient to judge language simplicity, but they are good approximations, and the magnitude of the difference between qdbp and its closest competitor is striking. Though the code snippet above may seem unfamiliar, understanding those 15 lines of code, along with learning a little bit of syntax sugar, is all that is required to understand the *entire* language.

Of course, just being small and simple is not sufficient. If it were, the world would run on [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck). qdbp's beauty comes from its ability to compose its small set of primitives to express complex abstractions. [Here](https://www.qdbplang.org/docs/examples) is a list and demonstration of common language that qdbp can emulate. The list includes

- Infinite lists
- If/then/else
- All sorts of loops
- Inheritance
- Iterators
- Switch
- Operators
- Functional programming Patterns(list, map, filter, etc)
- Modules
- Domain specific language creation, despite not having macros

That qdbp's core feature set can be so naturally wielded to express complex abstractions is testament to the elegance of the language. The goal of qdbp is to be the distillation of programming into only its necessary components, and in doing so, become greater than the sum of its parts. If this sounds like something you want from a language, clone this repo, checkout [qdbp's website](https://qdbplang.org), and follow the quickstart below. Happy qdbping!

## Quick Start

qdbp requires ocaml, dune, ocamllex, and menhir. To use the compiler, write your program in your text editor or IDE of your choice, clone this repo and run
```bash
dune exec ../bin/main.exe -- <filename> --execute
```
from the `samples` directory. Note that your program *must* be located in the `samples` directory.