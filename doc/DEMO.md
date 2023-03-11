# THE WRITING AND CONTENTS HERE ARE VERY MUCH A WIP
## That Could

qdbp isn't just "The Little Language." It is "The Little Language That Could." Despite its size, qdbp is very expressive. This document demonstrates a subset of the popular abstractions that qdbp either natively supports or can emulate.

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
max_of_two := (max! a: ... b: ...)
max_of_three := (max! a: ... b: ... c: ...)
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
dynamic dispatch