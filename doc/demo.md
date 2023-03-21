## A demo of qdbp
While the core language of qdbp is small, its features can be composed to express complex abstractions. This document demonstrates some examples of this. Most of this code you wouldn't write regularly but would instead reuse from a library.

### Functions
```python
def add(a, b):
    return a + b
add(1, 2)
```
can be done in qdbp like so:
```ocaml
add := {a b | a + b.}
add! a: 1 b: 2.
```
`add` is technically not a function. It is a prototype with a single method `!`. For convenience, however, we refer to any prototype with `!` as a function.
### Generics
Methods are generic by default. Here is a generic `print` function:
```ocaml
print := {val | val Print.}
ignore := print! 3.
print! "hello".
```
### If/Then/Else
```ocaml
if := {val then else|
  val
    True? [then!.]
    False? [else!.].
}
if! 1 > 2.
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
### Switch
There are a variety of ways to implement `switch`, depending on your needs. Here is one
```ocaml
switch := {val |
  {
    Val[val]
    Result[#None{}]
    Case[val then|
      self Val. = val.
        True? [
          result := then!.
          {self Result[#Some result]}]
        False? [self].
    ]
    Default[then|
      self Result.
        Some? [val| val]
        None? [then!.].
    ]
  }
}
str := switch! 5.
  Case 1 then: {"one"}.
  Case 2 then: {"two"}.
  Case 3 then: {"three"}.
  Case 4 then: {"four"}.
  Case 5 then: {"five"}.
  Default then: {"None of the above"}.
str Print.
```
### Data Structures
All of the data structures [here](https://en.wikipedia.org/wiki/Purely_functional_data_structure#Examples) can be implemented in qdbp. In addition, qdbp will have [Perceus Reference Counting](https://www.microsoft.com/en-us/research/uploads/prod/2020/11/perceus-tr-v1.pdf) in the near future, allowing data structures to reuse memory when possible and clawing back some of the performance lost to immutability.

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
### Operators
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
### While Loops
```ocaml
while := {val body|
  val!.
    True? [
      ignore := body!.
      self! val: val body: body.
    ]
    False? [{}].
}
; Will loop infinitely
while! {1 < 2.}
  body: {
    "hello world\n" Print.
  }.
```
### Functional List Manipulation
For brevity, the implementation of the list objects is omitted.
```ocaml
double_list := {list | list Map {val | val * 2.}. }
sum_list := {list | 
              list FoldLeft fn: {val acc | val + acc.} initial: 0. 
            }
{}
```
### Classes
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
### Single inheritance/Dynamic dispatch
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
### Partial Objects
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
### More Method Logic Reuse
What if we wanted to add these comparison operators to an object that already existed rather than adding methods to the comparison object that already exists? Here is how we could do that
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
### For loop with iterator
```ocaml
for := {iter body|
  iter Data.
    None? [{}]
    Some?[data|
      ignore := body! (data Val).
      self! iter: data Next. body: body.
    ].
}
from := {val to|
  {
    Start[val]
    End[to]
    Val[self Start.]
    Next[
      start := (self Start) + 1.
      {
        self
        Start[start]
      }
    ]
    Data[
      (self Start) <= (self End).
        True? [ #Some self ]
        False? [#None{}].
    ]
  }
}
for!
  iter: (from! 1 to: 10)
  body: {val | val Print.}.
```
### Phantom Fields
Some values may have the same types but semantically mean different things. For example, an int could be a number of cookies or it could be a day of the month. When we perform operations, we can make sure we don't use the wrong values at compile time by using phantom fields.
```ocaml
day_12_of_month := {
  12
  DayOfMonthUnit[{}]
}
five_cookies := {
  5
  CookieUnit[{}]
}
fn_involving_days := {val |
  typecheck := {val DayOfMonthUnit.} ; compiletime check for unit
  ; Do something with val
  {}
}
ignore := fn_involving_days! day_12_of_month. ; ok
fn_involving_days! five_cookies. ; compilation error
```
However, qdbp's type system is limited. We cannot make, for example, methods that multiply units automatically like in c++. We can only handcheck for the existence of specific field names.

### Error handling
#### Returning Errors
```ocaml
safe_divide := {a b|
  b = 0.
    True? [#Error{}]
    False? [#Ok a / b.].
}
result := safe_divide! a: 1 b: 0.
result
  Error? ["error" Print.]
  Ok?[val| val Print.].
```
#### Propagating Errors
```ocaml
error := {
  Transform[fn|
    self Val.
      Error? [err| self]
      Ok? [val| {self Val[#Ok fn! val.]}].
  ]
}
safe_divide := {a b|
  b = 0.
    True? [{error Val[#Error{}]}]
    False? [{error Val[#Ok a / b.]}].
}
safe_divide_6 := {a b c d e f|
  (safe_divide! a: a b: b) 
    Transform fn: {val | val / c.}.
    Transform fn: {val | val / d.}.
    Transform fn: {val | val / e.}.
    Transform fn: {val | val / f.}.
}
safe_divide_6! a: 3996 b: 3 c: 1 d: 2 e: 2 f: 1. Val.
  Error? ["Error" Print.]
  Ok? [val | val Print.].
```
#### Abort
```ocaml
panic := {val |
  ignore := val Print.
  ABORT.
}
safe_divide := {a b|
  b = 0.
    True? [panic! "divide by 0\n".]
    False? [a / b.].
}
ignore := safe_divide! a: 1 b: 0.
"Shouldn't get here!!!" Print.
```
### Infinite Lists
```ocaml
pows_of_2_list := {
  Val[1]
  Next[
    cur_val := self Val.
    { self Val[cur_val * 2.] }
  ]
}
ignore := pows_of_2_list Val. Print.
pows_of_2_list := pows_of_2_list Next.
ignore := pows_of_2_list Val. Print.
pows_of_2_list := pows_of_2_list Next.
ignore := pows_of_2_list Val. Print.
pows_of_2_list := pows_of_2_list Next.
{}
```
### Modules
In `math.qdbp`, for example, we could have the code
```ocaml
{
  Factorial[val|
    val = 0.
      True? [1]
      False? [val * (self Factorial (val - 1)).].
  ]
  Abs[val|
    val < 0.
      True? [-1 * val.]
      False? [val].
  ]
}
```
Then in another file, we can have
```ocaml
math := @math
ignore := math Factorial 5. Print.
math Abs -3 Print.
```
### Defer
```ocaml
defer := {val after|
  result := after!.
  ignore := val!.
  result
}

defer! {"finished " Print.}
  after: {
    ignore := "doing fancy math\n" Print.
    1 + 1. + 3. * 15.
  }.
```
This can be used, for example, to handle file closing or for automatic benchmarking
### DSL Creation
We can create our own DSLs in qdbp, even though we don't have macros. Here is a sample DSL syntax for a build system we could implement
```ocaml
my_project
  AddLibDirectory "./lib".
  AddTestDirectory "./test".
  AddExecutable "bin/main.qdbp".
  SetOptimizationLevels
    performance: 3
    size: 2.
```