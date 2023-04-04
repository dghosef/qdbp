open Types

(* Variables *)
let make_declaration name rhs post_decl_expr loc : ast =
  `Declaration (name, rhs, post_decl_expr, loc)
let make_variable_lookup name loc : ast =
  `VariableLookup (name, loc)

(* Prototype stuff *)
let full_field_name field_name arg_names =
  (fst field_name) ^ ":" ^ (String.concat ":" arg_names), (snd field_name)
let make_prototype_field name meth loc =
  let args, _, _ = meth in
  let arg_names = List.map (fun (name, _) -> name) args in
  (full_field_name name arg_names, meth, loc)
let make_empty_prototype loc : ast =
  `EmptyPrototype loc
let make_prototype maybe_extension fields loc : ast =
  let extension = match maybe_extension with
    | Some extension -> extension
    | None -> make_empty_prototype loc
  in 
  List.fold_left (fun acc field -> `PrototypeExtension (acc, field, loc))
    extension
    fields
let make_prototype_invoke_arg name value loc =
  (name, value, loc)
let make_meth args body loc =
  (("self", loc) :: args, body, loc)
let make_method_invocation receiver field_name arg1 args loc : ast =
  let args = match arg1 with
    | Some arg -> ("val", arg, loc) :: args
    | None -> args
  in
  let arg_names = List.map (fun (name, _, _) -> name) args in
  let field_name = full_field_name field_name arg_names
  in
  make_declaration
    ("Self", loc)
    receiver
    (`MethodInvocation (
        (make_variable_lookup "Self" loc),
        field_name,
        ("self", make_variable_lookup "Self" loc, loc) :: args,
        loc))
    loc
let make_closure args body loc : ast = 
  let meth = make_meth args body loc in 
  let field = make_prototype_field ("!", loc) meth loc in
  make_prototype None [field] loc
let make_param name loc = 
  (name, loc)

(* Tagged object stuff *)
let make_tagged_object tag value loc : ast =
  `TaggedObject (tag, value, loc)
let make_pattern_match receiver cases loc : ast = 
  `PatternMatch (receiver, cases, loc)
let make_pattern_match_meth arg body loc =
  (arg, body, loc)
let make_pattern_match_atom name meth loc = (name, meth, loc)

(* Imports, literals, abort, external call *)
let make_import filename (loc: Lexing.position * Lexing.position) : ast =
  (* FIXME: Should raise a different type of exception or return err? *)
  if not (Filename.is_relative filename) then
    Error.internal_error "Absolute paths not allowed in imports"
  else
    try
      let relative_path = filename in
      let directory = Filename.dirname (Unix.realpath (fst loc).pos_fname) in
      let absolute_path = Filename.concat directory relative_path in
      let absolute_path = Unix.realpath absolute_path in
      `Import (absolute_path, loc)
    with
      Unix.Unix_error (Unix.ENOENT, _, _) ->
      Error.internal_error ("File not found: " ^ filename)
let make_literal literal_template_filename literal_value loc =
  let import = make_import literal_template_filename loc in
  make_method_invocation import ("!", loc) (Some literal_value) [] loc
let make_int_literal i loc : ast = make_literal "int" (`IntLiteral (i, loc)) loc
let make_float_literal f loc : ast = make_literal "float" (`FloatLiteral (f, loc)) loc
let make_string_literal s loc : ast = make_literal "string" (`StringLiteral (s, loc)) loc
let make_external_call fn_name args loc : ast = 
  `ExternalCall (fn_name, args, loc)
let make_abort loc : ast = `Abort loc