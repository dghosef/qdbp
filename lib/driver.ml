type args =
  {
    print_ty: bool;
    execute: bool;
    ml_output_file: string option;
    bin_output_file: string option;
    input_file: string;
  }

let print_ty = ref false
let execute = ref false
let ml_output_file = ref None
let bin_output_file = ref None
let input_file = ref ""

let speclist = [
  ("--print-type", Arg.Set print_ty,
   "Print the inferred type of the program");
  ("--execute", Arg.Set execute,
   "Execute the program");
  ("--ml-output-file", Arg.String (fun s -> ml_output_file := Some s),
   "Specify the output file for the generated ML code");
  ("--bin-output-file", Arg.String (fun s -> bin_output_file := Some s),
   "Specify the output file for the generated binary");
]

let string_of_speclist speclist =
  let speclist_strings = List.map
      (fun (flag, _, desc) ->
         flag ^ "\t" ^ desc
      ) speclist
  in
  "--help\tPrint this message\n"
  ^ String.concat "\n" speclist_strings

let usage_msg =
  "Usage: [<options>] file
options:\n" ^ (string_of_speclist speclist)

let anon_fun filename = 
  assert (!input_file = "");
  input_file := filename

let args () = 
  Arg.parse speclist anon_fun usage_msg;
  (if !input_file = "" then
     Error.abort ("error: no input file specified\n" ^ usage_msg));
  {
    print_ty = !print_ty;
    execute = !execute;
    ml_output_file = !ml_output_file;
    bin_output_file = !bin_output_file;
    input_file = !input_file;
  }

let compile args =
  let full_input_file =
    if Filename.is_relative args.input_file then
      Filename.concat (Filename.current_dir_name) args.input_file
    else
      args.input_file
  in
  let full_input_file = Unix.realpath full_input_file in
  let files, ast = ParserDriver.parse_file
      ParserDriver.FileMap.empty
      full_input_file in
  let (imports, files) = ResolveImports.build_import_map files ast in
  let ast = ResolveImports.resolve_imports imports ast in
  let tvars, ty, ast = Infer.infer files ast in
  let loc = Infer.loc_of ast in

  let _, ast = FreeVariablesStr.free_variables ast in
  let ast = Inline.inline ast in
  let _, ast = FreeVariablesStr.free_variables ast in
  let ast = Inline.inline ast in

  let ast, max_label = NamesToInts.names_to_ints ast in
  let fvs, ast = FreeVariables.free_variables ast in
  let ml = CodegenML.codegen_ml ast in
  let ast = Refcount.refcount Refcount.FvSet.empty Refcount.FvSet.empty ast in
  let ast = Anf.anf ast (fun x -> x) in
  let ast = Refcount.fusion ast in
  let methods, ast = CollectMethods.collect_methods ast in
  let bvs, ast = LetBoundVariables.let_bound_variables ast in
  let methods = CollectMethods.IntMap.map
      ( fun (args, body, loc, fvs) ->
          let bvs, body = LetBoundVariables.let_bound_variables body in 
          (args, body, loc, fvs, bvs)
      ) methods in
  let main_method = ([], ast, loc, fvs, bvs) in
  let main_method_id = (Oo.id (object end)) in
  let methods = CollectMethods.IntMap.add main_method_id main_method methods in
  print_endline (CodegenC.codegen_c methods main_method_id max_label);
  let ml_output_file = match args.ml_output_file with
    | Some f -> f
    | None -> args.input_file ^ ".ml"
  in
  let bin_output_file = match args.bin_output_file with
    | Some f -> f
    | None -> args.input_file ^ ".bin"
  in
  let bin_output_file = 
    if Filename.is_relative bin_output_file then
      Filename.concat (Filename.current_dir_name) bin_output_file
    else
      bin_output_file
  in
  let oc = open_out ml_output_file in
  output_string oc ml;
  close_out oc;
  let compile_cmd = "ocamlc -o " ^ bin_output_file ^ " " ^ ml_output_file in
  let _ = Sys.command compile_cmd in
  if args.print_ty then
    print_endline (Type.str_of_ty tvars ty);
  if args.execute then
    let _ = Sys.command (bin_output_file) in
    ()
