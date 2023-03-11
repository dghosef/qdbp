## The Little Language That Can't
qdbp is in its infancy. The current implementation of qdbp
- Compiles slowly and produces inefficient code
- Gives terrible error messages
- Lacks basic I/O
- Needs support for string, character, number, and list literals
- Doesn't have an import mechanism
- Has barely any documentation
However, these limitations are all fixable and will come with time. To that end, here is a shortlist of qdbp's agenda for the future
- Performance:
  - [ ] Change the compilation target from OCaml to a lower level language
  - [ ] Implement [Perceus Reference Counting](https://www.microsoft.com/en-us/research/uploads/prod/2020/11/perceus-tr-v1.pdf)
- Error Messages:
  - [ ] Add error states to the parser for better error messages
  - [ ] Keep track of history of type inference to better pinpoint the origin of type errors
- Essential Features:
  - [ ] Add support for basic literals with corresponding manipulation I/O methods
  - [ ] Add an import mechanism
- Documentation - create a website with:
  - [ ] A comprehensive tutorial
  - [ ] A reference manual
  - [ ] A language specification
Longer term goals include the addition of
- [ ] Support for concurrency
- [ ] An LSP
- [ ] A REPL
- [ ] A debugger
- [ ] A package manager
However, all of these features in and of themselves are significant projects, potentially similar in scope to the creation of the language itself. As such, they will not be rushed and take a little more time to design and implement.
