## The Little Language That Can't
qdbp is in its infancy. The current implementation of qdbp
- Compiles slowly and produces inefficient code
- Gives terrible error messages
- Has a bad compiler ui
- Lacks basic I/O
- Needs support for string, character, number, and list literals
- Doesn't have an import mechanism
- Has barely any documentation

However, these limitations are all fixable and removing them will come with time. To that end, here is a shortlist of qdbp's agenda for the future

- Usability:
  - [ ] Make the compiler ui better with options for printing the type, AST, ocaml code, and just executing
  - [ ] Get rid of temp files
- Performance:
  - [ ] Change the compilation target from OCaml to a lower level language
  - [ ] Implement [Perceus Reference Counting](https://www.microsoft.com/en-us/research/uploads/prod/2020/11/perceus-tr-v1.pdf)
- Error Messages:
  - [ ] Add error states to the parser for better error messages
  - [ ] Annotate each ast node with a type, potentially being an error type
  - [ ] Keep track of history during type inference to better pinpoint the origin of type errors
- Essential Features:
  - [ ] Add support for basic literals with corresponding manipulation I/O methods
  - [ ] Add an import mechanism
- Documentation - create a website with:
  - Create a website with:
    - [ ] A comprehensive tutorial
    - [ ] A reference manual
    - [ ] A language specification
  - [ ] Clean up the compiler implementation - move to bin dir

Longer term goals include the addition of

- [ ] Support for concurrency(Adding this while keeping within the philosophy of the language will be tricky)
- [ ] An LSP
- [ ] A REPL
- [ ] A debugger
- [ ] A package manager

