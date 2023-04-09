
let usage_msg = "qdbp [--print_ty] <in_file> [-o <out_file>]"
let in_file = ref ""
let anon_fun filename = 
  assert (!in_file = "");
  in_file := filename
let speclist = []
let () =
  print_newline ();
  Arg.parse speclist anon_fun usage_msg;
  (* Go back to functional style *)
  let in_file = !in_file in
  assert (in_file != "");
  let in_file = Filename.concat (Filename.current_dir_name) (in_file) in
  let files, ast = Qdbp.ParserDriver.parse_file
    Qdbp.ParserDriver.FileMap.empty
    (Unix.realpath in_file) in 
  let (imports, files) = Qdbp.ResolveImports.resolve_imports files ast in
  let tvars, ty = Qdbp.Infer.infer imports files ast in
  let _, ty_str = (Qdbp.Type.str_of_ty tvars ty) in
  prerr_endline (ty_str)
