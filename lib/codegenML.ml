module StringMap = Map.Make(String)
(* FIXME: Change to angle bracket, square bracket, etc, combine w/ other file*)
let paren s = "(" ^ s ^ ")"
let brace s = "[" ^ s ^ "]"
let quoted s = 
  "\"" ^ (String.escaped s) ^ "\""


let codegen_ml imports ast =
  let varid = ref 0 in
  let add_variable varnames name =
    let id = !varid in
    varid := id + 1;
    StringMap.add name (id) varnames in
  let varname varnames name = 
    "__qdbp_var_" ^ (string_of_int (StringMap.find name varnames))
  in
  let rec codegen_method varnames (args, body, _) =
    let arg_names = List.map (fun (name, _) -> name) args in
    let varnames = List.fold_left add_variable varnames arg_names in
    let arg_strs = List.map (
        fun name ->
          "let " ^ (varname varnames name) ^ " = " ^ "__qdbp_first __qdbp_args" ^ " in\n" ^
          "let __qdbp_args = (__qdbp_rest __qdbp_args) in\n"
      ) arg_names in
    "\n" ^ paren ("fun __qdbp_args -> \n" ^
                  (String.concat "" arg_strs) ^
                  (codegen_ml varnames body))
  and codegen_ml varnames ast =
    match ast with
    | `EmptyPrototype _ -> "__qdbp_empty_prototype"
    | `PrototypeExtension (ext, ((name, _), meth, _), _) -> 
      let meth = codegen_method varnames meth in
      let ext = codegen_ml varnames ext in
      paren ("__qdbp_extend " ^ ext ^ " " ^ (quoted name) ^ " " ^ meth)
    | `TaggedObject ((tag, _), value, _) -> 
      paren ("__qdbp_tagged_object " ^ (quoted tag) ^ " " ^ (codegen_ml varnames value))
    | `MethodInvocation (receiver, (name, _), args, _) ->
      let args = List.map (
          fun (_, value, _) -> codegen_ml varnames value
        ) args in 
      let args_str = brace (String.concat "; " args) in
      paren ("__qdbp_method_invocation " ^
             (codegen_ml varnames receiver) ^
             " " ^ (quoted name) ^ " " ^ args_str)

    | `PatternMatch (receiver, cases, _) -> 
      let cases_methods = List.map (
          fun ((name, _), (arg, body, _), loc) ->
            (name, ([arg], body, loc)))
          cases in
      let cases_methods_strings = List.map (
          fun (tag, meth) ->
            paren ((quoted tag) ^ ", " ^ (codegen_method varnames meth))
        ) cases_methods in
      let lst = brace (
          String.concat ";" cases_methods_strings
        ) in
      paren ("__qdbp_pattern_match " ^ (codegen_ml varnames receiver) ^ " " ^ lst)
    | `Declaration ((name, _), rhs, body, _) ->
      let varnames' = add_variable varnames name in
      paren (
        "let " ^ (varname varnames' name) ^ " = " ^ (codegen_ml varnames rhs)
        ^ " in \n" ^ (codegen_ml varnames' body)
      )
    | `VariableLookup (name, _) ->
      varname varnames name
    | `Import (filename, _) ->
      codegen_ml varnames (ResolveImports.ImportMap.find filename imports)
    | `ExternalCall ((name, _), args, _) ->
      let args = List.map (codegen_ml varnames) args in
      let args = String.concat " " args in
      paren (name ^ " " ^ args)
    | `IntLiteral (i, _) -> paren (
        "__qdbp_int_literal " ^ (string_of_int i)
      )
    | `FloatLiteral (f, _) -> 
      paren ("__qdbp_float_literal " ^ (string_of_float f))
    | `StringLiteral (s, _) ->
      paren ("__qdbp_string_literal " ^ (quoted s))
    | `Abort _ -> "(__qdbp_abort ())"
  in 
  let body = codegen_ml (StringMap.empty) ast in
  Runtime.prelude ^
  "let __qdbp_result = \n" ^ body ^ "\n"
