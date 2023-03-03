open Ast
let varname id =
  match id with 
  | Some id -> "var_" ^ string_of_int id ^ "_QDBP_VAR"
  | None -> "error"
(* FIXME: Name better *)
(* FIXME: Use a paren function for parenthesizing stuff *)
(* FIXME: Use a quote function for qutoing stuff *)
let varname2 origin =
  match origin with 
  | Some id -> varname id 
  | None -> "error"
let rec ocaml_of_ast ast =
  match ast with
  | RecordExtension re -> 
    let name = re.field.field_name in 
    let meth = ocaml_fn_of_method re.field.field_value in
    let rest = ocaml_of_ast re.extension in
    "(Extension {" ^
    "field_name = \"" ^ name ^ "\"; " ^
    "field_method = " ^ meth ^ "; " ^
    "extension = (" ^ rest ^ ")})"
  | EmptyRecord _ -> "(emptyrecord)"
  | Record_Message rm -> 
    "((select (\"" ^ rm.rm_message ^ "\") (" ^ (ocaml_of_ast rm.rm_receiver) ^ ")) [" ^
    List.fold_left (fun acc param -> 
        acc ^ 
        (if ((String.length acc) = 0) then "" else ";")
        ^ " (" ^ ocaml_of_ast param.value ^ ")") 
      "" rm.rm_arguments ^ "])"
  | Sequence s ->
    "(\n" ^ (ocaml_of_ast s.l) ^ " ; \n" ^ (ocaml_of_ast s.r) ^ "\n)"
  | Declaration d ->
    "let " ^ (varname d.decl_id) ^ " = " ^ (ocaml_of_ast d.decl_rhs) ^ " in emptyrecord"
  | Variable v -> varname2 v.origin
  | OcamlCall oc -> 
    "((" ^ oc.fn_name ^ " (" ^ (ocaml_of_ast oc.fn_arg) ^ ")); emptyrecord)"
and ocaml_fn_of_method meth =
  let param_decls = List.fold_left
      (fun acc arg_id -> acc ^ 
                         "let " ^ (varname arg_id) ^ " = " ^ "List.hd params" ^ " in\n" ^
                         "let params = List.tl params in\n") "" meth.arg_ids in
  let body = (ocaml_of_ast meth.method_body) in
  let fn_decl = "\n(fun (params: (record list)): record ->\n" ^ param_decls ^ body ^ ")" in
  fn_decl
let read_whole_file filename =
  let ch = open_in filename in
  let s = really_input_string ch (in_channel_length ch) in
  close_in ch;
  s
let prelude = "
type base_object =
  | Int of int
  | EmptyRecord
  
type record =
  | Extension of record_extension
  | Empty of base_object
and record_extension =
  {field_name: string;
   field_method: meth;
   extension: record}
(* Change from record list to array or something*)
and meth = (record list) -> record
let emptyrecord = Empty EmptyRecord
let select field_name record =
  let rec select field_name record =
    match record with
    | Empty _ -> raise Not_found
    | Extension {field_name = field_name'; field_method = field_method;
                 extension = extension} ->
      if field_name = field_name' then
        field_method
      else
        select field_name extension
  in
  select field_name record
  let print_hi _ = print_endline \"hi\"
"
let emptymap = Name_resolver.VarIdMap.empty
let ocaml_of_qdbp_program in_file =
  let ast = Ast_utils.ast_of_string (read_whole_file in_file) in 
  let _, ast = Tag_ast.tag_ast 0 ast in 
  let ast, _ = Name_resolver.name_resolve ast emptymap in 
  let ty, _ = Infer.infer (Infer.Env.empty) 0 ast in
  prerr_endline (Type.string_of_ty 0 ty);
  let ocaml_code = ocaml_of_ast ast in
  prelude ^ 
  "[@@@warning \"-10\"]\n" ^
  "[@@@warning \"-26\"]\n" ^
  "let result =\n"  ^
  ocaml_code ^ "\n" ^
  "[@@@warning \"+10\"]\n" ^
  "[@@@warning \"+26\"]\n"
