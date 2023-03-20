qdbp attempts to be simple, safe, and expressive. The philosophy, type system, syntax, and semantics are all guided by these goals. 

# Syntax
The syntax of qdbp is probably what immediately stands out. Although initially unfamiliar, qdbp is small enough that we are confident that users will be able to learn it quickly. 
## Method Invocation
The method invocation syntax, inspired by smalltalk, is qdbp's biggest syntactic variation compared with other languages. While most languages do
```c++
foo.Bar(a, b)
```
we do
```ocaml
foo Bar arg1: a arg2: b.
```
There are a few reasons for this. 
#### Documentation
Forcing users to specify the names of arguments makes it easier to read and understand their code. Consider the following two snippets:
```ocaml
rectangle Make width: 3 height: 4.
```
versus
```c++
rectangle.Make(3, 4)
```
In the first example, we can clearly see what the `3` and `4` mean, whereas in the second example, we have to look at the definition of `rectangle.Make` to understand what the arguments mean.
#### Overloading
Overloading by type would be tricky to implement and reason about in a language like qdbp because types are based on the structure of objects. Overloading by parameter name, which is only possible with named arguments, is much easier to implement and reason about. Having and overloading on named arguments allows us to write code like:
```ocaml
circle1 := circle Make radius: 3.
circle2 := circle Make area: 15.
circle3 := circle Make diameter: 6.
```
#### Operators
Operators are notoriously hard to do well. Particularly, they often lead to programmers having to memorize a large number of precedences and as a result introduce a lot of mental overhead. As a result, qdbp does not have operators. However, operator-like syntax like below is natural and unambiguous in qdbp. 
```ocaml
(3 + 4) * 2. / 18.
```
#### "Extensibility"
qdbp is not extensible. It has no macros or any facilities for compile-time metaprogramming. Yet its method invocation syntax the language feel extensible. For example, we can easily add an equivalent to the `if` construct to the language
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
Similarly `while`, `for`, `switch`, monads, iterators, scope guards, etc can all be implemented as objects and used naturally. We some examples of this below.

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
- Similarly, pattern matching can be thought of asking questions about tagged objects. Hence the `?` symbol
- When we import a file, we get the expression **at** the file. Hence the `@` symbol
- `#` is used for making tagged objects because `#` is the commonly used as the symbol for making (hash)tags

## `[]`
The specific syntax decisions in
```ocaml
MethodName[arg1 arg2 ...| body]
```
were a little bit arbitrary. There isn't really a deeper reason compared to other method syntaxes like
```c++
MethodName(arg1, arg2, ...) {
  body
}
```
apart from the fact that the former is shorter and more concise.

The decision to make variant pattern matching have similar syntax to methods was intentional. Variants are the dual of records because records are "this field *and* this field *and* this field," while variants are "this field *or* this field *or* this field." To emphasize this, we used similar syntax for both

# Structural Type System
qdbp uses a simple yet powerful type system that is often referred to as "static duck typing." The type system(descibed in detail [here](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/scopedlabels.pdf)) is based on the concept that types of objects are defined by what they have, not their name. This allows qdbp to be as easy to use as dynamically typed languages(which do the same thing, just dynamically), while eliminating the vast majority of runtime errors.
# Language Constructs
The vast majority of qdbp's design decisions were made using the following principle: "If a construct can be emulated with another existing feature, it should not be added to the language." The converse, "A construct should be added to the language if it cannot be emulated with another existing feature," was also used to guide design decisions, with the caveat that expressiveness and safety were still guaranteed.
## Prototypes vs classes
qdbp chose to use prototypes rather than objects because prototypes are conceptually simpler. They allow programmers to omit the boilerplate that comes from defining classes. Class-like objects do have a place, but they can be easily emulated with prototypes, while the reverse is not true.
## The role of methods
One of the most unique parts of qdbp is its role of methods. In qdbp, each field corresponds to a method, unlike other languages where fields can correspond to any type of data. This decision was made for simplicity. If the only way to access a field is through a method invocation, the only syntax that needs to be learned for method access is method invocation. And it is trivial to emulate traditional "data fields" with methods - simply just make the field a method with no argument that returns the value.
## Why no
qdbp makes a number of omissions that other languages have. Here is rationale for some of them
### Inheritance
Inheritance introduces a *lot* of complexity in the type system. Inheritance requires subtyping, and inference of type systems with subtyping is notoriously difficult. Instead, qdbp's "static duck typing," along with prototype extension, can be used to mimic single inheritance.
### Mutablity
Mutable variables introduce complexity, from the syntactic complexity of adding an assignment syntax to the semantic complexity of having to reason about variables whose values are constantly changing. qdbp avoids this by not having mutability anywhere. As an added bonus, this makes it impossible to form circular references, allowing qdbp to have predictable, reliable reference counting.
### Macros
Macros are a powerful tool, but they are also a source of complexity. They often make programs impossible to reason about and can be a source of subtle bugs. qdbp does not have macros, but it does have a powerful, expressive syntax that makes it easy to implement many features that usually would be done with macros with objects.
### Non-Local Control Flow(exceptions, coroutines, algebraic effects, etc)
Non-local control flow makes programs much harder to reason about and adds a lot of complexity to programs. In addition, such constructs are much trickier to implement on the compiler side. As a result, qdbp does not have any non-local control flow(other than `ABORT.`). 

However, non-local control cannot be emulated with any existing feature. So, while it doesn't have it right now, we don't rule out adding it in the future.
### Purely Functional I/O
qdbp does not have any purely functional I/O. This is because qdbp is not a purely functional language. In fact, qdbp is arguably not even a functional language(it doesn't even have first class functions). Purely functional I/O, both in the form of algebraic effects and monads, is a powerful tool for reasoning about programs, but qdbp's core value of simplicity makes it a poor fit for the language.

### Sequences

# Lack of Standard Library