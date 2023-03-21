qdbp attempts to be simple, safe, and expressive. The philosophy, type system, syntax, and semantics are all guided by these goals. 
# Syntax
The syntax of qdbp is what immediately stands out first. Although initially unfamiliar, it is small enough that it can be picked up quickly.
## Method Invocation
The method invocation syntax, inspired by smalltalk, is qdbp's biggest syntactic variation compared to other languages. While most languages have
```c++
foo.Bar(a, b)
```
qdbp has
```ocaml
foo Bar arg1: a arg2: b.
```
There are a few reasons for this. 
### Documentation
Forcing users to specify the names of arguments makes it easier to read and understand their code. Consider the following two snippets:
```ocaml
rectangle Make width: 3 height: 4.
```
versus
```c++
rectangle.Make(3, 4)
```
In the first example, we can clearly see what the `3` and `4` mean, whereas in the second example, we have to look at the definition of `rectangle.Make` to understand what the arguments mean.
### Overloading
Overloading by type would be tricky to implement and reason about in a language like qdbp with a structural type system. Overloading by parameter name, which is only possible with named arguments, is much easier. Having and overloading on named arguments allows us to write code like:
```ocaml
circle1 := circle Make radius: 3.
circle2 := circle Make area: 15.
circle3 := circle Make diameter: 4.
```
### Operators
Operators are notoriously hard to do well. In particular, they often lead to programmers having to memorize a large number of precedences, introducing a lot of mental overhead. As a result, qdbp does not have operators. However, operator-like syntax like below is natural and unambiguous in qdbp because of its method invocation syntax. 
```ocaml
(3 + 4) * 2. / 18.
```
### "Extensibility"
qdbp is not extensible. It has no macros or facilities for compile-time metaprogramming. However, its method invocation syntax makes the language feel extensible. For example, we can easily add an equivalent to the `if` construct to the language
```ocaml
if := {val then else|
  val
    True? [then!.]
    False? [else!.].
}
```
and use it like so
```ocaml
if! condition: ...
  then: {
    ...
  }
  else: {
    ...
  }.
```
Similarly `while`, `for`, `switch`, monads, iterators, `defer`, etc can all be implemented as objects and used naturally. We provide some examples of this below and [here](demo.md)

The variety of constructs that the syntax makes possible and natural to implement extends beyond normal general purpose language features. qdbp's syntax makes domain specific language(dsl) implementation easy. For example, there currently is a work in progress dsl for a build system using qdbp that looks like this:
```ocaml
my_project
  AddLibDirectory "./lib".
  AddTestDirectory "./test".
  AddExecutable "bin/main.qdbp".
  SetOptimizationLevels performance: 3 size: 2.
```
Users of this build system don't need to learn a new syntax and developers don't need to implement a parser or macros - it can all be done naturally in qdbp. 

## `.`, `@`, `#` and `?`
These symbols were all picked because their linguistic meaning corresponds to their meaning in qdbp.

- Much like periods end sentences in the English language, they end method invocation and pattern matching expressions
- Similarly, pattern matching can be thought of like asking questions about tagged objects. Hence the `?` symbol
- When we import a file, we get the expression at(`@`) the file.
- `#` is used for making tagged objects because `#` is the hash*tag* symbol

## `[]`
We chose
```ocaml
MethodName[arg1 arg2 ...| body]
```
as opposed to
```c++
MethodName(arg1, arg2, ...) {
  body
}
```
because the former is simpler and more concise.

The decision to make tagged object pattern matching have similar syntax to methods was intentional. Tagged objects are the dual of prototypes; tagged objects express a "this *or* this" relationship while prototypes express a "this *and* this" one. Their syntax similarities reflect this duality.
# Structural Type System
qdbp uses a simple yet powerful type system that is often referred to as "static duck typing." The type system(described in detail [here](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/scopedlabels.pdf)) is based on the concept that types of objects are defined by what they can do, not their name. This allows qdbp to be as easy to use as dynamically typed languages while retaining the safety of static typing
# Language Constructs
## The role of methods
Unlike other languages, objects in qdbp are only comprised of methods. Prototypes can't have "regular data" fields. This is because "regular data" fields are trivially emulatable with methods that just return the data. Thus, there was no need to add them to the language.
## Literals as Libraries
One of qdbp's most unique features is that its literals are implemented as libraries rather than built into the language. In practice for most users, this makes no difference because they can just use an existing library. However, this allows programmers to easily inspect the behavior of literals and change their behavior because there is no one size fits all, even for common operations like division(should division by `0` abort or return `#Error{}` or return `infinity`?). 
## The Lack Of
Many of qdbp's design decisions were made using the following principle: "If a construct can be emulated with another existing feature, it should not be added to the language." The converse, "A construct can be added to the language if it cannot be emulated with another existing feature," was also used to guide design decisions.
## (switch/for/while/if/defer/...)
Because all of these constructs are naturally implementable as libraries as shown [here](demo.md), in keeping with the guiding principle above, they are not part of the language primitives. 
### Classes
Classes can be emulated as functions that return prototypes, removing the need to add them separately
### Inheritance
Inheritance introduces a *lot* of complexity to the type system. It requires subtyping, and inference of subtypes is notoriously difficult to theoretically impossible. Instead, prototype extension is much simpler and captures most of the uses of single inheritance. qdbp has no way of mimicking multiple inheritance, but multiple inheritance is controversial at best and often unnecessary.
### Mutablity
Mutable variables introduce complexity, from the syntactic complexity of adding an assignment syntax to the semantic complexity of having to reason about variables whose values are constantly changing. qdbp avoids this by not having mutable variables. As an added bonus, this makes it impossible to form circular references, allowing qdbp to have predictable, reliable reference counting. This does have unavoidable performance implications, but for most applications, the performance hit is more than tolerable.
### Macros
Macros are a powerful tool, but they are also a source of complexity. They often make programs impossible to reason about and can be a source of subtle bugs. qdbp does not have macros, but it does have a powerful, expressive syntax that makes it easy to use objects for many of the same purposes as macros.
### Non-Local Control Flow(exceptions, coroutines, algebraic effects, etc)
Non-local control flow often makes programs much harder to reason about and adds a lot of complexity to languages. In addition, such constructs are much trickier to implement on the compiler side. As a result, qdbp does not have any non-local control flow(other than `ABORT.`). 

However, non-local control cannot be emulated with any existing feature. So, while qdbp doesn't have it right now, we don't rule out adding it in the future.
### Purely Functional I/O
qdbp does not have any purely functional I/O. This is because qdbp is not a purely functional language. In fact, qdbp is arguably not even a functional language(it doesn't even have first class functions). Purely functional I/O, both in the form of algebraic effects and monads, is a powerful tool for reasoning about programs, but qdbp's core value of simplicity makes it a poor fit for the language.
### Sequence Expressions
qdbp does not have explicit syntactic support for sequence expressions. Instead, they have to be done in the following manner
```
ignore := expr1
ignore := expr2
expr3
```
Admittedly, this is unsatisfying. The reason qdbp doesn't have sequence expressions yet is because we haven't found a good syntax for them. In particular, we want to avoid the `;` for expression separators because people find it annoying and it adds too much punctuation to the language(for example, `1+1+2..;` just looks ugly). If you have any ideas for this, please let us know.
### Concurrency
qdbp is simple and concurrency is complex. It will be added in the future, but it is arguably the hardest feature to add while keeping with the ethos of the language, so we are taking our time with it.