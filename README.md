## The Little Language That Could

qdbp is small. Really, really small. The language has just 1 keyword and 7 core constructs(along with full type inference and a little bit of syntax sugar to sweeten the deal). In fact here is a small program, implementing a basic boolean object, that demonstrates *every single construct* of the language:
```ocaml
true := {
  BoolVal [#True{}]
  If [then else|
    self BoolVal.
      True? [then]
      False? [else].
  ]
  Negate [
    self If
      then: { self BoolVal[#False{}] }
      else: { self BoolVal[#True{}] }.
  ]
}
false := (true Negate)
false Negate. Negate. Negate. Negate.
```
As a comparison, Go 1.20 has 25 keywords, Python 3.10 has 38, C++14 has 84, and Lua 5.4 has 22. Neither keyword nor feature count are sufficient to judge language simplicity, but they are good heuristics, and the magnitude of the difference between qdbp and its closest competitor is striking. Though the code snippet above may seem unfamiliar, understanding those 15 lines of code, along with learning a little bit of syntax sugar, is all that is required to understand the *entire* language.

Of course, just being small and simple is not sufficient. If it were, the world would run on [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck). qdbp's beauty comes from its ability to compose its small set of primitives to express complex abstractions. [Here](doc/DEMO.md) is a list and demonstration of features other languages have that qdbp can naturally emulate. The list includes

- Infinite Lists
- All sorts of Loops
- Inheritance
- Iterators
- Operators
- Functional Programming Patterns(list, map, filter, monads, etc)
- Automatic memoization
- Interfaces
- Modules

That qdbp's core feature set can be so naturally wielded to express complex abstractions is a testament to the elegance of the language. The goal of qdbp is to be the distillation of programming into only its necessary components, and in doing so, become greater than the sum of its parts. I believe it succeeded, but I'm biased. You should clone this repo, try it out keeping this list of [limitations](doc/LIMITATIONS.md) in mind, and let me know whether or not I am right.

## Quick Start

qdbp requires Ocaml, Dune, Ocamllex, and Menhir. Once you have those, clone this repo and run `dune exec ./bin/qdbp.exe <filename>` to run a file or `dune exec ./bin/qdbp.exe --help` for more options. While qdbp doesn't have support for syntax highlighting yet, I have found that using syntax highlighting for Ocaml works well enough. Happy qdbping!
