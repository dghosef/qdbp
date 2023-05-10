module StringMap = Map.Make(String)

let add_variable varnames name =
  let id = Oo.id (object end) in
  StringMap.add name (id) varnames

let varname varnames name = 
  StringMap.find name varnames

let var_names_to_ints ast =
  let rec var_names_to_ints varnames ast =
    match ast with
    | `EmptyPrototype loc ->
      `EmptyPrototype loc
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op) -> 
      let ext = var_names_to_ints varnames ext in
      let arg_names = List.map (fun (name, _) -> name) args in
      let varnames = List.fold_left add_variable varnames arg_names in
      let body = var_names_to_ints varnames body in
      let args = List.map (fun (name, loc) -> (varname varnames name, loc)) args in
      `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op)
    | `TaggedObject ((tag, tagLoc), value, loc) -> 
      let value = var_names_to_ints varnames value in
      `TaggedObject ((tag, tagLoc), value, loc)
    | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let args = List.map (fun (name, arg, loc) -> name, var_names_to_ints varnames arg, loc) args in
      let receiver = var_names_to_ints varnames receiver in
      `MethodInvocation (receiver, (name, labelLoc), args, loc)
    | `PatternMatch (receiver, cases, loc) -> 
      let receiver = var_names_to_ints varnames receiver in
      let cases = List.map (
          fun ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let varnames = add_variable varnames arg in
            let body = var_names_to_ints varnames body in
            ((name, nameLoc), ((varname varnames arg, argLoc), body, patternLoc), loc)
        ) cases in
      let receiver_id = Oo.id (object end) in
      `Declaration ((receiver_id, loc), receiver, 
                    `PatternMatch (receiver_id, cases, loc), loc)
    | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs = var_names_to_ints varnames rhs in
      let varnames = add_variable varnames name in
      let body = var_names_to_ints varnames body in
      `Declaration (((varname varnames name), nameLoc), rhs, body, loc)
    | `VariableLookup (name, loc) ->
      let name = varname varnames name in
      `VariableLookup (name, loc)
    | `ExternalCall ((name, nameLoc), args, loc) ->
      let args = List.map (var_names_to_ints varnames) args in
      `ExternalCall ((name, nameLoc), args, loc)
    | `IntLiteral (i, loc) ->
      `IntLiteral (i, loc)
    | `FloatLiteral (f, loc) -> 
      `FloatLiteral (f, loc)
    | `StringLiteral (s, loc) ->
      `StringLiteral (s, loc)
    | `Abort loc ->
      `Abort loc
  in 
  var_names_to_ints StringMap.empty ast