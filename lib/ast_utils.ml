open Lexing

let colnum pos =
  (pos.pos_cnum - pos.pos_bol) - 1

let pos_string pos =
  let l = string_of_int pos.pos_lnum
  and c = string_of_int ((colnum pos) + 1) in
  "line " ^ l ^ ", column " ^ c

let ast_of_string s =
  let lexbuf = Lexing.from_string s in
  try
    Parser.program Lexer.token lexbuf
  with Parser.Error ->
    raise (Failure ("Parse error at " ^ (pos_string lexbuf.lex_curr_p)))

(* TODO: fix the closure arg spacing *)
let paren s = "(" ^ s ^ ")"
let doindent indent = "\n" ^ String.make (indent * 2) ' '
let rec pprint_variant_message indent (m: Ast.variant_message list) =
  doindent indent ^ match m with
  | [] -> ""
  | h :: t -> paren (h.tag ^ " " ^ pprint_ast indent h.fn) ^ pprint_variant_message indent t
and pprint_record_arguments indent (a: Ast.argument list) =
  match a with
  | [] -> ""
  | h :: t -> paren (h.name ^ " " ^ pprint_ast indent h.value) ^ pprint_record_arguments indent t
and pprint_closure_args indent (a: string list) =
  match a with
  | [] -> ""
  | h :: t -> h ^ " " ^ pprint_closure_args indent t
and pprint_record_fields indent (f: Ast.record_field list) =
  match f with
  | [] -> ""
  | h :: t -> 
    doindent indent ^ match h with
    | Ast.Field f -> paren (paren f.self ^ " " ^ paren f.field_name ^ " " ^ pprint_ast indent f.field_value) ^ pprint_record_fields indent t
    | Ast.Destructure d -> paren ("destructure " ^ pprint_ast indent d.e) ^ pprint_record_fields indent t
and pprint_ast indent ast =
  paren (match ast with
  | Ast.Record r          -> "Record " ^ pprint_record_fields (indent + 1) r.fields
  | Ast.Variant v         -> "Variant " ^ paren v.tag ^ " " ^ pprint_ast (indent + 1) v.value
  | Ast.Closure c         -> "Closure " ^ paren (pprint_closure_args indent c.args) ^ pprint_ast indent c.body
  | Ast.Record_Message m  -> "Record Message " ^ pprint_ast indent m.receiver ^ " " ^ (paren m.message) ^ (pprint_record_arguments indent m.arguments)
  | Ast.Variant_Message m -> "Variant Message " ^ (pprint_ast indent m.receiver) ^ (pprint_variant_message (indent + 1) m.message)
  | Ast.Sequence s        -> "Sequence" ^ doindent (indent + 1) ^ pprint_ast (indent + 1) s.l ^ doindent (indent + 1) ^ pprint_ast (indent + 1) s.r
  | Ast.Assignment a      -> ":= " ^ (paren a.lhs) ^ " " ^ (pprint_ast indent a.rhs)
  | Ast.Variable v        -> "Var " ^ v.name
  | Ast.Int i             -> "Int " ^ (string_of_int i.value))

let pprint_program s = pprint_ast 0 (ast_of_string s)