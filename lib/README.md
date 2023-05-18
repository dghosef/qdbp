# lib
This is where the bulk of the compiler goes. The majority of these files are compiler passes. The general flow of the compiler is:

lexer.mll -> parser.mly -> resolveImports.ml -> infer.ml -> varNamesToInts.ml -> freeVariables.ml -> codegenC.ml/codegenML.ml