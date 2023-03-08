<!--- FIXME: Change the order of prototypes throughout
 FIXME: Comment syntax is different too --->
# A case for the little language that could

A lot can be learned from the tagline of a language. qdbp's went through a number of iterations - "object orientation done right," "safe, small, and expressive," and "simplicity that excites" were all trialed. That these were all considered was no accident. Each of these taglines describes a goal or feature of qdbp. However, none of these taglines encapsulates the fundamental philosophy of the language quite as well as its final iteration, "the little language that could," does.

Programming language design is as much a human computer interaction problem as it is a systems and theory problem. Every  language has to grapple with the problem of how to present what is essentially a sequence of bit manipulation operations in a manner that is easy to understand to users. qdbp does this through a small set of core constructs that, when combined with its innovative syntax, can be combined to build up a host of intuitive abstractions.

# Little Language
qdbp is little. In fact, here is an example simple implementation of a stack of booleans that demonstrates *every* core construct of the language
```ocaml
empty_stack := {
  Stack [ #Empty{} ]
  Push [ val |
    curr_stack := self
    {
      Stack [ 
        #NonEmpty {Val[val] Next[curr_stack]} 
      ]
      curr_stack
    }
  ]
  Peek [
    self Stack.
      Empty? [#None {}]
      NonEmpty? [ val | #Some (val Val.) ].
  ]
}
(empty_stack Push val: #True {}.) Push: #False {}.
```

Note that the syntax is very different from most other languages. But, because the language is so small, the cognitive burden to picking up the language is minimal. 

qdbp has just 7 core constructs, and thus, the following language reference spans just a few pages.

##### Variable Declaration
Like other languages, in qdbp variables can be declared
```ocaml
x := some expression
some expression
```
If the same name is used multiple times, the variable is reassigned each time.
##### Variable Referencing
Variables can also be referenced
```ocaml
x := some expression
x
```
##### Prototype Creation
qdbp has two types of objects. The first one is a 'prototype object' which can be thought of a list of (label, method) pairs. The syntax to create a prototype object is as follows:
```ocaml
{
  Label1[param1 param2 | body]
  Label2[body] // `Label2` has 0 params
}
```
Labels must start with either an uppercase letter or a symbol(like `U` or `*`) and parameters must start with a lowercase letter. 

Because in qdbp it is so common to create prototypes with just the single label `!`(as seen in the next section),
```ocaml
{ param1 param2 | body }
```
is syntax sugar for
```ocaml
{ ! = [param1 param2 | body] }
```

All methods also include an implicit `self` parameter that can be referenced in their bodies. The value of the `self` parameter is the prototype the body is contained in.

In the presence of identical labels whose methods have the identical paramater names, the value of the label declared earlier overwrites the value of the label declared later.
```ocaml
{
  Label[param1 param2 | ...] // The final method associated with `Label` is this one
  Label[param1 param2 | ...]
}
```
The return types of the two methods associated with `Label` *must* be the same.
##### Prototype Extension
Fields in prototypes can be both extended and replaced. Consider the following code:
```ocaml
prototype1 := {
  Label1[{}]
}
prototype2 := {
  Label2[{}]
  prototype1
}
```
Notice that the `prototype1` at the bottom of `prototype2` is not inside `[]` and is not affiliated with a label. Instead, it is *extending* the enclosing prototype. It can conceptually be thought of as copy and pasting the contents of `prototype1` into the enclosing prototype. 

The rule of disambiguating in prototype extension is identical to that of simple prototype creation
```ocaml
prototype1 := 
{
  Label[...]
}
prototype2 :=
{
  Label[...] // This method beats the one in `prototype1` because this comes before `prototype1` and must have the same type as the other method
  prototype1
}
```

There is one subtle nuance with record extension. The type system of `qdbp` requires that, when extending a record with a value whose labels depends on a parameter, all the labels in the record must also be contained in the extension like shown with the same parameter names:
```ocaml
{
  [param |
    {
      Label [param1 param2 | ...]
      param // param must contain the label `Label` with the parameters `param1` and `param2`
    }
  ]
}
```

##### Prototype Selection
Labels within prototypes can also be selected. The syntax for prototype selection is as follows
```ocaml
myprototype := 
{
  Label[param1 param2| body]
}
myprototype Label param1: expr1 param2: expr2.
// Note the `.` symbol. qdbp uses a period to end a prototype selection statement, much like English uses a period to end sentences
```
and the result of the above program is `body` evaluated with `self` equal to `myprototype`, `param1` equal to `expr1` and `param2` equal to `expr2`. 

Prototype selection can be "overloaded" by parameter names - if two methods have the same label but different parameter names, at the selection, the method with the same parameters as those supplied during selection will be selected.

If the param name is not supplied during selection and just a `:` is supplied, the the name is automatically assumed to be `val`. The utility of this simple syntax sugar is shown in the following section.

##### Tagged Object Creation
The second type of object is the tagged object. A tagged object is created in the following manner
```
initial_object := 
{
  Label1 [{}]
  Label2 [{}]
}
tagged_object := #Tag initial_object
...
```
##### Tagged Object Selection
Tagged object selection is as follows
```ocaml
tagged_object := ...
tagged_object
  Tag1? [val1| body1]
  Tag2? [val2| body2].
  // Note the `.`
```
The result of the above program depends on the tag assigned to `tagged_object`. If the tag is `#Tag1`, result is `body1` with `val1` equal to the associated object. And the same is true with `#Tag2`, `body2`, and `val2` respectively

##### Types
qdbp is statically typed. The type system is beyond the scope of this document, but is detailed in [this paper](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/scopedlabels.pdf). While `qdbp` is statically typed, it is structurally typed and all types are inferred, giving it the feel and simplicity of dynamically typed languages while retaining the safety of statically typed languages.

# That Could
qdbp simply being little doesn't make it great. For example, the joke language [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck) is even smaller, and nobody even considers using it. What makes qdbp useful is that its 7 core constructs can be composed to make a variety of useful abstractions. The rest of this section shows how qdbp's features combine to emulate a host of abstractions that other languages have.

##### Functions
qdbp doesn't include functions. However, functions can be emulated by prototypes with a single label. By convention, we use `!` For example, here is the identity function
```ocaml
identity := {val | val}
identity! val: x.
```
Even though qdbp has no concept of first class functions, for the rest of the document we refer to "function" as a prototype with the label `!`.

##### If statements
By using the tags `#True` and `#False`, `if ... then ... else` can be implemented as a function.
```ocaml
if := 
  {cond then else | 
    cond 
      True? [then!.]
      False? [else!.].
  }
if! cond: #True{} then: {...} else: {...}.
```ocaml
Alternatively, rather than having `if` be a function, we could just have it be a member of a boolean prototype object
true :=
  {
    BoolVal[#True{}]
    If [ then else | 
      self BoolVal.
        True? [then!.]  
        False? [else!.].
    ]
  }
false := {BoolVal[#False{}] true}
ifTrue := true If then: {...} else: {...}.
ifFalse := false If then: {...} else: {...}.

From now on, when we refer to booleans, we refer to an object tagged with either `#True` or `#False`
```

##### Custom containers
Containers can be implemented in qdbp. Here is a linked list of booleans
```ocaml
empty_list := {
  CurrVal[#None{}]
  Next[#None{}]
  Prepend[val| 
    curr_list := self
    {
      CurrVal[#Some val]
      Next[#Some curr_list]
      curr_list
    }
  ]
}
empty_list Prepend: #True{}. Prepend: #False{}. Prepend: #True{}.
```
##### Single inheritance
Prototype extension mimics inheritance. For example, if we wanted to add a `pop_front` method to our empty list that removes the first element. Rather than copying `empty_list` and adding a new method, we can simply use extension.
```ocaml
list_with_pop := {
  Pop[self Next.]
  empty_list
}
```
##### Operators
Operators are simply labels made of symbols. Here is how we would add an and operator(`&&`) to a boolean
```ocaml
true := {
  && [val | 
    self BoolVal.
      False? [false]
      True? [
        val BoolVal.
          True? [true]
          False? [false].
      ].
  ]
  true
}
true &&: false.
```
##### Looping Infinitely
Recursion can be done with the implicit `self` parameter. Here is an example of the simplest type of loop - the infinite loop. Tail call elimination is guaranteed, so stack overflows won't occur in this scenario.
```ocaml
object := 
{
  LoopForever [self LoopForever.]
}
object LoopForever.
```
Similarly, a non-terminating function would look like
```ocaml
{self!.}!.
```
##### Functional Programming
Not all iteration has to be infinite. Common functional patterns, like `map`, `reduce`, and `filter` can all be implemented either as separate functions or as methods within the list prototype using recursion.

##### Generic Functions
All methods assigned to variables are generic. For example, consider the following identity function
```ocaml
identity := {val | val}
empty_prototype := identity! :{}.
true := identity! :#True{}.
```
Each time a variable is used, its methods can take on new types. This is why `identity` can take on parameters of different types.

Here is another example of a generic function that takes in 2 parameters that contain a `<` method and returns the greater of the two

```ocaml
max := {a b| (a <: b.)
  True? [b]
  False? [a].
}
```
##### Method Reuse
Sometimes, many different prototypes will share some common methods. We can separate the logic of the methods into functions and then reuse them. For example, it is common to want to derive the `>=` operators for an object with the `>` operator and the `==` operator(assume that we have implemented an Or method in booleans).
```ocaml
gte := {me other | 
  (me >: other.) Or: (me ==: other.).
}
my_object := {
  >[...]
  =[...]
  >= [val | gte! me: self other: val.]
}
```
##### Overloading by parameter names
Here is an example of a max function that can be overloaded based on the number of params to either find the max of 2 or 3 elements:
```ocaml
max := {
    ![a b| ...]
    ![a b c| ...]
}
max_of_two := max! a: ... b: ... .
max_of_three := max! a: ... b: ... c: ... .
```
This can also be used to emulate default params

##### Error handling
Error handling can be done with error values. For example, here is a stack method that returns `#Error` on failure and `#Success` otherwise
my_stack := {
  Next[...]
  Peek[
    (self Next.)
      None? [#Error #EmptyStackPeek]
      Some? [v| #Success v].
  ]
}
##### Error
Error propogation can be made automatic
```ocaml
error := 
{
  ErrorVal[#Error {}]
  <<=[fn| 
    (self ErrorVal.)
      #Error [error | #Error error]
      #Success [val | 
        {ErrorVal[#Success (fn! val: val.)] self}].
}
// This will propogate the divide by zero error and the additions will never occur
fail := (one /: zero) <<=: {val | val +: one.}. <<=: {val | val +: one.}.
// but this return three
succeed := (one /: one) <<=: {val | val +: one.}. <<=: {val | val +: one.}.
```
##### And more...
Modules, iterators, list comprehension, for loops, while loops, tuples, and so, so much more can all be implemented in qdbp. qdbp is the ultimate customizable language. Despite coming with very relatively few constructs, the abstractions that can be implemented in the language are limitless. Users can shape and mold the language to their needs

# The
qdbp is not "A little language that could" - it is "*The* little language that could." It solves a problem that no other language does - it is small and simple enough for the whole language to easily fit completely in a developer's brain while being expressive enough to implement a myriad of abstractions per its usecase. qdbp's expressive power rivals that of python, Haskell, Ocaml, and most other mainstream languages while being far, far smaller than any of its counterparts.

##### A note about lisps
Lisps are frequently designed with the same goal in mind as qdbp. Most of them have pretty small base languages that can be extended with macros. This approach is unsatisfying because macros, in effect, grow the language itself. As a result, while lisps start as small languages, as developers add more macros, the language itself becomes larger, harder to reason about, and more and more complex. 

# The future of qdbp
Compiler and language ecosystem implementation is a project that takes tens of man years. Here is a shortlist, in no particular order, of next steps for qdbp:
- Better compiler error messages
- Imports
- Standard Library
- Syntax for Integer, List, and String literals and native Int and String prototypes
- An abort function whose return type is always automatically correct
- A cleaner compiler implementation
- Compilation to a lower level language
- Perceus reference counting(perfect because cycles are not possible in qdbp)
- O(1) record and variant selection
- A website
- Debugger
- An LSP

# The Little Language that Could
Virtually every language designer pours hours and hours of their own personal time into their language. They read countless papers, fret over small syntax decisions, and wrestle with parser generators, to name a few, knowing from the get-go that, by far, the most likely outcome is that they are going to be the only user. As over-dramatic as it sounds, designing a programming language often brings up a host of existential questions. What is the point? What makes a language successful? How can I justify my commitment?

For qdbp, the answer to all of these questions is neither adoption nor recognition. qdbp will likely never gain a single user nor reach the attention of anyone outside of [r/ProgrammingLanguages](reddit.com/r/programminglanguages), a forum for language design nerds. qdbp will never be published in a reputable academic journal and its expected lifetime monetary value is less than that of searching the streets for dropped money for an hour.

The goal of qdbp was to discover something beautiful, and by that measure, the language is an overwhelming success. qdbp elegantly combines a small set of constructs with an innovative syntax into something that is much, much greater than the sum of its parts. It is an extistential proof of the oxymoron that extremely expressive languages can be extremely simple, and that in and of itself is beautiful.

Of course, getting the language to a more mature point, gaining adopters, or even inspiring other languages would all be added bonuses. But qdbp is already a success because it is the little language that could.