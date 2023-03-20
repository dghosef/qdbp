## The Little Language That Can't
qdbp is in its infancy. The current implementation of qdbp
- Compiles slowly and produces inefficient code
- Gives terrible error messages
- Has barely any documentation

However, these limitations are all fixable and removing them will come with time. To that end, here is a shortlist of qdbp's agenda for the future. It is currently going through a rewrite that, when finished, will have the following changes:

- Performance:
  - [ ] Change the compilation target from OCaml to C and allow user to include external C files
  - [ ] Implement [Perceus Reference Counting](https://www.microsoft.com/en-us/research/uploads/prod/2020/11/perceus-tr-v1.pdf)
- Error Messages:
  - [ ] Add error states to the parser for better error messages
  - [ ] Make the type inference functional-style
  - [ ] Annotate each ast node with a type, potentially being an error type
  - [ ] Keep track of history during type inference to better pinpoint the origin of type errors
  - [ ] Use a recursive descent parser
- Documentation
  - Create a website with:
    - [ ] A language specification
  - [ ] Code Samples Directory
  - [ ] Clean up the compiler implementation

Longer term goals include the addition of

- [ ] Support for concurrency(Adding this while keeping within the philosophy of the language will be tricky)
- [ ] An LSP
- [ ] A REPL
- [ ] A debugger
- [ ] A package manager