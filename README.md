## The Little Language That Could

qdbp is small. Really, really small. The language has just 7 core constructs(along with type inference and a little bit of syntax sugar to sweeten the deal). In fact here is a small program, implementing a basic boolean object, that demonstrates *every single feature* of the language:
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
false := true Negate.
false Negate. Negate. Negate. Negate.
```
As a comparison, Go has 25 keywords, Python 3.10 has 38, C++14 has 84, and Lua has 22. Lisps, though they start small, can become arbitrarily large and complex via macros. Even Smalltalk has conceptual complexities like metaclasses. Neither keyword nor feature count are sufficient to judge language simplicity, but they are a good heuristic, and the magnitude of the difference between qdbp and its closest competitor is striking. In fact, qdbp's feature count of 7 is most comparable to (https://en.wikipedia.org/wiki/Brainfuck)[Brainfuck], the joke programming language with 8 constructs. Though the code snippet above may seem unfamiliar, understanding those 15 lines of code, along with learning a little bit of syntax sugar, is all that is required to understand the *entire* language.

Of course, just being small and simple is not sufficient. If it were, the world would run on Brainfuck. qdbp's beauty comes from its ability to compose its small set of primitives to express complex abstractions. [Here](todo.md) is a list and demonstration of features other languages have that qdbp can naturally emulate. The list includes

- Infinite Lists
- Loops
- Inheritance
- Iterators
- Operators
- So much more

That qdbp's core feature set can be so naturally wielded to express complex abstractions is a testament to the elegance of the language. The goal of qdbp is to be the distillation of programming into only its necessary components, and in doing so, become greater than the sum of its parts. I believe it succeeded, but I'm biased. You should clone this repo, try it out, and let me know whether or not I am right.

## Quick Start