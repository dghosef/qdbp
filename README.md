# qdbp: The little language that could
# Next steps
- Unit type
- Ints/Strings
- IO
- Abort function
- Imports
- URGENT/FIXME's
- Error messages
- Syntax highlighting
- Neater codegen to lower level language
- comments
# Build
Building qdbp requires `ocaml` version 4.14.0 or newer, `menhir` version 20220210 or newer, and `dune` version 3.4.1 or newer.

# The Little Language:
```ocaml
stack := {
  Stack [ #Empty{} ]
  Push [ val |
    curr_stack := self
    {
      self
      Stack [ 
        #NonEmpty { Val[val] Next[curr_stack] } 
      ]
    }
  ]
  Peek [
    self Stack.
      Empty? [#None {}]
      NonEmpty? [ val | #Some val Val. ].
  ]
}
stack Push: {}.
```
# That could
## Generics
## Dynamic Dispatch
## Inheritance
## Operators
## Functional Style Programming
## Error Handling

# TODO
Syntax sugar for closures
Floats
Strings, Lists
Unicode