# Build
Building qdbp requires `ocaml` version 4.14.0 or newer, `menhir` version 20220210 or newer, and `dune` version 3.4.1 or newer.

# Syntax in a nutshell

```javascript
/*
Multiline comment
*/
/**
also a multiline comment
**/
// single line comment
product : {
    self basic_field        : 5
    self number             : [arg1 arg2 | arg1 message param: arg2]
    self call               : self closure arg1: 3 arg2: 4
    self sequencing         : [self call, self call, self call]
    self chaining           : [ self closure arg1: 3 negate arg2: 4 
                                 closure arg1: 5 arg2: 6 
                                 closure arg1: 7 arg2: 8 ]
    self overloaded         : [arg1 arg2 | ]
    self overloaded         : [a b |]
}

person Run
    miles: (3 Multiply times: 4.)
    



ID MESSAGE_NAME MESSAGE_ARG INT MESSAGE_NAME MESSAGE_ARG INT PERIOD MESSAGE_ARG INT PERIOD

sum : [arg1 | arg1 ifTrue: #True{} ifFalse: #False{}] arg1: true.

sum
    #True [value | value]
    #False [value | value].

prototype_concatenation : {
    ...product
        self number : 5.
    ...other_object.
}
```

# TODO
Floats
Operators
Syntax sugar for method references
Strings, Arrays
Unicode