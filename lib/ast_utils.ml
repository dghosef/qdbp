(* FIXME: Rename File. Check all filenames *)
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
    let message = "Parse error at " ^ (pos_string lexbuf.lex_curr_p) ^ "\n" ^
                  List.nth (String.split_on_char '\n' s) (lexbuf.lex_curr_p.pos_lnum - 1) in
    raise (Failure (message ^ "\n"))

let fst pair =
  let ret, _ = pair in ret
(* TODO: Get rid of this? *)
(* FIXME: This name is sorta disingenuous cuz we dont ignore empty records *)
let ast_local_fold_map fn ast acc=
  match ast with 
  | Ast.RecordExtension re -> 
    let f = re.field in
    let f = {f with field_value = 
                      {f.field_value with method_body =
                                            fst (fn f.field_value.method_body acc)}} in
    let extension = fst (fn re.extension acc) in
    let variant_expr, acc = (match re.variant_expr with 
        | None -> None, acc
        | Some (name, expr) -> 
          let expr, acc = fn expr acc in
          Some (name, (expr)), acc) in
    (Ast.RecordExtension {variant_expr = variant_expr; 
                          field = f; extension = extension}), acc
  | EmptyRecord -> (Ast.EmptyRecord), acc
  | Abort -> (Ast.Abort), acc
  | IntLiteral i -> (IntLiteral i), acc
  | StringLiteral s -> (StringLiteral s), acc
  | Ast.Record_Message message ->
    (* No threading accumulator do all the args *)
    let cases = 
      (match message.cases with 
       | None -> None
       | Some cases -> 
         let process_case (str1, str2, e) =
           let expr, _ = fn e acc in 
           (str1, str2, expr)
         in
         let variant, cases_list = cases in
         let new_cases = List.map process_case cases_list in
         let variant, _ = fn variant acc in
         Some (variant, new_cases))
    in
    let process_arg (arg: Ast.argument) =
      let expr, _ = fn arg.value acc in 
      {arg with value = expr}
    in
    let args = List.map process_arg message.rm_arguments in
    (* Then do the receiver*)
    let receiver, acc' = fn message.rm_receiver acc in
    Ast.Record_Message {message with cases = cases; rm_arguments = args; rm_receiver = receiver}, acc'

  (* Map to expr and return new acc *)
  | Ast.Sequence sequence -> 
    let lhs, acc' = fn sequence.l acc in 
    let rhs, acc'' = fn sequence.r acc' in 
    Ast.Sequence {l = lhs; r = rhs}, acc''
  | Ast.Declaration decl ->
    let rhs, acc' = fn decl.decl_rhs acc in 
    Ast.Declaration {decl with decl_rhs = rhs}, acc'
  | Ast.Variable variable -> Ast.Variable variable, acc
  | Ast.OcamlCall call ->
    let args = List.map (fun arg -> fst (fn arg acc)) call.fn_args in
    Ast.OcamlCall {call with fn_args = args}, acc
  | Ast.Import import ->
    let expr, acc' = fn import.import_expr acc in 
    Ast.Import {import with import_expr = expr}, acc'

let id_to_str = function 
  | None -> "-1"
  | Some id -> string_of_int id

let origin_to_str = function 
  | None -> "-2"
  | Some id ->
    (match id with 
     | None -> "-1"
     | Some id -> string_of_int id)

let rec string_repeat i =
  if i == 0 then "" else "  " ^ string_repeat (i - 1)
let do_indent i = 
  "\n" ^ string_repeat i
(* FIXME: Make this an inner function so we can omit indent *)
let rec string_of_ast ast indent =
  let meth_to_string (m: Ast.meth) =
    let body = string_of_ast m.method_body indent in
    let args = List.fold_left2 (fun sofar name id -> sofar ^ " (" ^ name ^")" ^ (id_to_str id)) "" m.args m.arg_ids in 
    "[" ^ args ^ "|" ^ body ^ "]"
  in
  match ast with
  | Ast.RecordExtension re -> 
    let f = re.field in
    let name = f.field_name in 
    let value = meth_to_string f.field_value in 
    let extension = string_of_ast re.extension (indent + 1) in 
    "{" ^ name ^ " " ^  " = " ^ value ^ "..." ^ do_indent (indent + 1) ^ extension ^ "}"
  | EmptyRecord ->
    "{EmptyRecord}"
  | Abort  ->
    "ABORT"
  | IntLiteral i -> string_of_int i
  | StringLiteral s -> "\"" ^ s ^ "\""
  | Record_Message rm ->
    let process_arg acc (arg: Ast.argument) =
      let name = arg.name in 
      let value = string_of_ast arg.value indent in 
      acc ^ "(" ^ name ^ " = " ^ value ^ ")"
    in
    let receiver = string_of_ast rm.rm_receiver indent in 
    let params = List.fold_left process_arg "" rm.rm_arguments in 
    "(" ^ receiver ^ ")."^ rm.rm_message ^ "(" ^ params ^ ")"
  | Sequence s ->
    let indent = indent + 1 in 
    let l = string_of_ast s.l indent in
    let r = string_of_ast s.r indent in
    "(" ^ do_indent indent ^ l ^ do_indent indent ^ r ^ ")"
  | Declaration d ->
    let lhs = d.decl_lhs in 
    let rhs = string_of_ast d.decl_rhs indent in 
    lhs ^ id_to_str d.decl_id^ " = " ^ rhs
  | Variable v -> 
    "(" ^ v.var_name ^ " origin=" ^ origin_to_str v.origin ^ ")"
  | OcamlCall c -> 
    let args = List.map (fun arg -> string_of_ast arg 0) c.fn_args in 
    let args = String.concat ", " args in
    "$" ^ c.fn_name ^ "(" ^ args ^ ")"
  | Import i ->
    "import " ^ (string_of_ast i.import_expr indent)
let ast_map fn ast =
  let fn' ast _ =
    fn ast, ()
  in
  let ast, _ = ast_local_fold_map fn' ast () in 
  ast