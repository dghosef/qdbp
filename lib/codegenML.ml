(*
  Compile qdbp ast to OCaml
  This is experimental and super inefficient. Will eventually be
  compiled to C.
  The C backend is a work in progress
*)
module StringMap = Map.Make(String)
(* FIXME: Change to angle bracket, square bracket, etc, combine w/ other file. Ditto with lexer/parser naming *)
let paren s = "(" ^ s ^ ")"
let brace s = "[" ^ s ^ "]"
let quoted s = 
  "\"" ^ (String.escaped s) ^ "\""


let codegen_ml ast =
  let varname id = 
    "__qdbp_var_" ^ (string_of_int (id))
  in
  let rec codegen_method (args, body, _) =
    let arg_names = List.map (fun (name, _) -> name) args in
    let arg_strs = List.map (
        fun name ->
          "let " ^ (varname name) ^ " = " ^ "__qdbp_first __qdbp_args" ^ " in\n" ^
          "let __qdbp_args = (__qdbp_rest __qdbp_args) in\n"
      ) arg_names in
    "\n" ^ paren ("fun __qdbp_args -> \n" ^
                  (String.concat "" arg_strs) ^
                  (codegen_ml body))
  and codegen_ml ast =
    match ast with
    | `EmptyPrototype _ -> "__qdbp_empty_prototype"
    | `PrototypeCopy (ext, ((name, _), meth, _), _, _) -> 
      let meth = codegen_method meth in
      let ext = codegen_ml ext in
      paren ("__qdbp_extend " ^ ext ^ " " ^ (quoted name) ^ " " ^ meth)
    | `TaggedObject ((tag, _), value, _) -> 
      paren ("__qdbp_tagged_object " ^ (quoted tag) ^ " " ^ (codegen_ml value))
    | `MethodInvocation (receiver, (name, _), args, _) ->
      let args = List.map (
          fun (_, value, _) -> codegen_ml value
        ) args in 
      let args_str = brace (String.concat "; " args) in
      paren ("__qdbp_method_invocation " ^
             (codegen_ml receiver) ^
             " " ^ (quoted name) ^ " " ^ args_str)

    | `PatternMatch (receiver, cases, _) -> 
      let cases_methods = List.map (
          fun ((name, _), (arg, body, _), loc) ->
            (name, ([arg], body, loc)))
          cases in
      let cases_methods_strings = List.map (
          fun (tag, meth) ->
            paren ((quoted tag) ^ ", " ^ (codegen_method meth))
        ) cases_methods in
      let lst = brace (
          String.concat ";" cases_methods_strings
        ) in
      paren ("__qdbp_pattern_match " ^ (codegen_ml receiver) ^ " " ^ lst)
    | `Declaration ((name, _), rhs, body, _) ->
      paren (
        "let " ^ (varname name) ^ " = " ^ (codegen_ml rhs)
        ^ " in \n" ^ (codegen_ml body)
      )
    | `VariableLookup (name, _) ->
      varname name
    | `ExternalCall ((name, _), args, _) ->
      let args = List.map (codegen_ml) args in
      let args = String.concat " " args in
      paren (name ^ " " ^ args)
    | `IntLiteral (i, _) -> paren (
        "__qdbp_int_literal " ^ (paren (string_of_int i))
      )
    | `FloatLiteral (f, _) -> 
      paren ("__qdbp_float_literal " ^ (string_of_float f))
    | `StringLiteral (s, _) ->
      paren ("__qdbp_string_literal " ^ (quoted s))
    | `Abort _ -> "(__qdbp_abort ())"
  in 
  let body = codegen_ml ast in
  Runtime.prelude ^
  "let __qdbp_result = \n" ^ body ^ "\n"
