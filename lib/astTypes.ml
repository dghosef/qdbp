(* Because of the prevalent use of polymorphic variants, we don't
   have a lot of type definitions. These are just here because
   ocamllex/menhir require them *)

type lexer_pos = Lexing.position
type parser_pos = Lexing.position * Lexing.position
type id = string * parser_pos

type ast =
  [ `EmptyPrototype of parser_pos
  | `PrototypeCopy of ast * field * int * parser_pos
  | `TaggedObject of id * ast * parser_pos
  | `MethodInvocation of
    ast * id * (string * ast * parser_pos) list * parser_pos
  | `PatternMatch of ast * case list * parser_pos
  | `Declaration of id * ast * ast * parser_pos
  | `VariableLookup of string * parser_pos
  | `Import of string * parser_pos
  | `ExternalCall of id * ast list * parser_pos
  | `StrProto of string * parser_pos
  | `Abort of parser_pos
  | `IntProto of string * parser_pos ]

and field = id * meth * parser_pos
and meth = id list * ast * parser_pos
and case = id * (id * ast * parser_pos) * parser_pos

module IntSet = Set.Make (struct
  type t = int

  let compare = compare
end)

module IntMap = Map.Make (struct
  type t = int

  let compare = compare
end)

module StringSet = Set.Make (String)
module StringMap = Map.Make (String)

module IntPairSet = Set.Make (struct
  type t = int * int

  let compare = compare
end)
