open Ast_utils
let read_whole_file filename =
  let ch = open_in filename in
  let s = really_input_string ch (in_channel_length ch) in
  close_in ch;
  s

let parse_file file = 
  Ast_utils.ast_of_string (read_whole_file file)
let rec resolve_imports ast =
  match ast with
  | Ast.RecordExtension _ -> ast_map resolve_imports ast
  | Ast.EmptyRecord -> Ast.EmptyRecord
  | Ast.Abort -> Ast.Abort
  | Ast.IntLiteral i -> Ast.IntLiteral i
  | Ast.StringLiteral s -> Ast.StringLiteral s
  | Ast.Declaration _ -> ast_map resolve_imports ast
  | Ast.Variable _ -> ast_map resolve_imports ast
  | Ast.Record_Message _ -> ast_map resolve_imports ast
  | Ast.Sequence _ ->  ast_map resolve_imports ast
  | Ast.OcamlCall _ ->  ast_map resolve_imports ast
  (* FIXME: Add cycle detection *)
  (* FIXME: Make file extensions optional *)
  (* FIXME: Should search subdirectories or something *)
  (* FIXME: Cache ast's *)
  | Ast.Import im ->
    let expr = parse_file im.filename in
    let expr = resolve_imports expr in
    Ast.Import {im with import_expr = expr}
