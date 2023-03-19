# This document is a WIP
# TODO: Actually verify all the code works
## That Could

qdbp isn't just "The Little Language." It is "The Little Language That Could." Despite its size, qdbp is very expressive. This document demonstrates a subset of the popular abstractions that qdbp either natively supports or can emulate.

##### Functions
```python
def add(a, b):
    return a + b
add(1, 2)
```
is equivalent to
```ocaml
add := {a b | a + b}
add a: 1 b: 2.
```
##### Generic Functions
Functions are generic by default. Here is a generic `print` function:
```ocaml
print := {val | val Print.}
ignore := print! 3.
print! "hello".
```
From now on, we refer to "functions" as classes with the `!` field
##### If statements
```ocaml
if := {val then else|
  val
    True? [then!.]
    False? [else!.].
}
if 1 > 2.
  then: { 
    "true" Print. 
 }
  else: {
    "false" Print.
}.
```
Alternatively, `if` can be a method in a boolean object like so:
```ocaml
true := {
  BoolVal[#True{}]
  If[then else|
    self BoolVal.
      True? [then!.]
      False? [else!.].
  ]
}
true If
  then: { 
    "true" Print. 
 }
  else: {
    "false" Print.
}.
```
##### Data Structures
All of the data structures [here](https://en.wikipedia.org/wiki/Purely_functional_data_structure#Examples) can be implemented in qdbp. In addition, qdbp will have Perceus Reference Counting in the near future, allowing data structures to reuse memory when possible, clawing back some of the performance lost to immutability.

As an example, here is an implementation of a stack:
```ocaml
stack := {
  Data[#Empty{}]
  Push[val|
    curr_data := self Data.
    {
      self
      Data[#NonEmpty {
        Val[val]
        Next[curr_data]
      }]
    }
  ]
  Peek[
    self Data.
      Empty?[ABORT.]
      NonEmpty?[data| data Val.].
  ]
}
stack Push 3. Push 2. Peek. Print.
```
##### Operators
```ocaml
true := {
  BoolVal[#True{}]
  &&[val|
    self BoolVal.
      True? [val]
      False? [self].
  ]
}
false := {true BoolVal[#False{}]}
true && false.
```
##### All sorts of loops
```ocaml
while := {val body|
  val!.
    True? [
      ignore := body!.
      self! val: val body: body.
    ]
    False? [{}].
}
while! {1 < 2.}
  body: {
    "hello world\n" Print.
  }.
```
##### Functional List Manipulation
For brevity, the implementation of the list objects are omitted.
```ocaml
double_list := {list | list Map {val | val * 2.}. }
sum_list := {list | list Fold fn: {val acc | val + acc.} initial: 0. }
{}
```
##### Classes
A class is just a function that returns an object. For example, this:
```python
class circle:
    def __init__(self, radius):
        self.radius = radius
    def print(self):
        print(self.radius)
my_circle = circle(3)
my_circle.print()
```
can be implemented in qdbp like so:
```ocaml
make_circle := {radius |
  {
    Radius[radius]
    Print[self Radius. Print.]
  }
}
my_circle := make_circle! radius: 3.
my_circle Print.
```
##### Single inheritance/Dynamic dispatch
Some cases of single inheritance can be mimicked with prototype extension.
```ocaml
basic_circle := {
  Radius[3]
  Print[self Radius. Print.]
}
colored_circle := {
  basic_circle
  Color["red"]
  Print[
    ignore := self Color. Print.
    self Radius. Print.
  ]
}
ignore := basic_circle Print.
colored_circle Print.
```
##### Partial Objects
Consider the following object
```ocaml
comparison := {
  >=[val|
    self < val.
      False? [#True{}]
      True? [#False{}].
  ]
  <=[val|
    self > val.
      False? [#True{}]
      True? [#False{}].
  ]
  =[val|
    self >= val.
      True? [self <= val.]
      False? [#False{}].
  ]
  !=[val|
    self = val.
      True? [#False{}]
      False? [#True{}].
  ]
}
```
Now, any object that extends `comparison` and implements `<` and `>` gets the other comparison operators for free.
##### More Method Logic Reuse
What if we wanted to add these comparison operators to an object that already existed rather than adding methods to the comparison object that already exists? Here is an example:
```ocaml
equals := {val other|
  other >= val.
    True? [other <= val.]
    False? [#False{}].
}
{
  some_object_that_already_has_geq_and_leq
  =[val| equals! val: self other: val.]
}
```
##### For loop with iterator
```ocaml
for := {iter body|
  iter
    HasNext? [value|
      ignore := body! (value Val.).
      self! iter: value Next. body: body.
    ]
    NoNext? [{}].
}
```
##### Overloading
##### List Slicing
##### Function Piping
##### Advanced Type Tricks
##### Error handling
##### Monads
##### Infinite Lists
##### Automatic Memoization
##### List comprehensions
##### Modules
##### Scope Guards
##### Classes
##### Switch Statement
##### Interfaces