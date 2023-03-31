(* Because of the prevalent use of polymorphic variants, we don't
   have a lot of type definitions. These are just here because
   ocamllex/menhir require them *)
  
type lexer_pos = Lexing.position
type parser_pos = Lexing.position * Lexing.position
type lexer_error =
  [ 
    `BadInteger of (string * lexer_pos)
  | `BadFloat of (string * lexer_pos)
  | `BadCharacter of (char * lexer_pos)
  | `EofInComment of lexer_pos
  | `EofInString of lexer_pos
  ]
type id = (string * parser_pos)
type ast =
  [
    `EmptyPrototype of parser_pos
  | `PrototypeExtension of (ast * field * parser_pos)
  | `TaggedObject of (id * ast * parser_pos)
  | `MethodInvocation of (ast * id * ((string * ast * parser_pos) list) * parser_pos)
  | `PatternMatch of (ast * (case list) * parser_pos)
  | `Declaration of (id * ast * ast * parser_pos)
  | `VariableLookup of (string * parser_pos)
  | `Import of (string * parser_pos)
  | `ExternalCall of (id * (ast list) * parser_pos)
  | `IntLiteral of int
  | `FloatLiteral of float
  | `StringLiteral of string
  | `Abort of parser_pos
  ]
and field = id * meth * parser_pos
and meth = (id list) * ast * parser_pos
and case = id * (id * ast * parser_pos) * parser_pos