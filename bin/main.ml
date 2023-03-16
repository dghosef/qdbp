
let usage_msg = "qdbp [--print_ty] <in_file> [-o <out_file>]"
let print_ty = ref false
let in_file = ref ""
let out_file = ref ""
let print_code = ref false
let print_ast = ref false
let anon_fun filename = 
  assert (!in_file = "");
  in_file := filename
let speclist = [
  ("--print_ty", Arg.Set print_ty, "Print the type of the program to stdout");
  ("--print_code", Arg.Set print_code, "Print the ocaml source to stdout");
  ("--print_ast", Arg.Set print_ast, "Print the AST to stdout");
  ("-o", Arg.Set_string out_file, "Output file");
]
let () = 
  print_newline ();
  Arg.parse speclist anon_fun usage_msg;
  (* Go back to functional style *)
  let in_file = !in_file in
  let out_file = !out_file in
  let print_ty = !print_ty in
  let print_code = !print_code in
  let print_ast = !print_ast in
  assert (in_file != "");
  (* FIXME: Reduce the number of intermediate string buffers throughout? *)
  let ast = Qdbp.Resolve_imports.parse_file in_file in 
  let ast = Qdbp.Resolve_imports.resolve_imports ast in
  let _, ast = Qdbp.Tag_ast.tag_ast 0 ast in 
  let ast, _ = Qdbp.Name_resolver.name_resolve ast Qdbp.Codegen_ml.emptymap in 
  if print_ast then
    print_endline (Qdbp.Ast_utils.string_of_ast ast 0);
  let ty, _ = Qdbp.Infer.infer (Qdbp.Infer.Env.empty) 0 ast in
  if print_ty then
    print_endline (Qdbp.Type.string_of_ty 0 ty);
  let code = Qdbp.Codegen_ml.ocaml_of_ast ast in
  if print_code then
    print_endline code;
  if out_file != "" then
    let oc = open_out out_file in
    Printf.fprintf oc "%s" code;
    close_out oc
  else
    let oc = open_out ".tmp.ml" in
    Printf.fprintf oc "%s" code;
    close_out oc;
    let cmd = "ocaml .tmp.ml" in
    let _ = Sys.command cmd in
    let _ = Sys.command "rm .tmp.ml" in
    ()