module IntMap = Map.Make(struct type t = int let compare = compare end)

let add_variable varnames name =
  let id = Oo.id (object end) in
  IntMap.add name (id) varnames

let varname varnames name = 
  IntMap.find name varnames

let resolve_shadows ast =
  (* FIXME: Make functional *)


  (* FIXME: Make this purely functional *)
  let rec resolve_shadows varnames ast =
    match ast with
    | `EmptyPrototype loc ->
      `EmptyPrototype loc
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op) -> 
      let ext = resolve_shadows varnames ext in
      let arg_names = List.map (fun (name, _) -> name) args in
      let varnames = List.fold_left add_variable varnames arg_names in
      let body = resolve_shadows varnames body in
      let args = List.map (fun (name, loc) -> (varname varnames name, loc)) args in
      `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op)
    | `TaggedObject ((tag, tagLoc), value, loc) -> 
      let value = resolve_shadows varnames value in
      `TaggedObject ((tag, tagLoc), value, loc)
    | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let args = List.map (fun (name, arg, loc) -> name, resolve_shadows varnames arg, loc) args in
      let receiver = resolve_shadows varnames receiver in
      `MethodInvocation (receiver, (name, labelLoc), args, loc)
    | `PatternMatch (receiver_id, cases, loc) -> 
      let cases = List.map (
          fun ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let varnames = add_variable varnames arg in
            let body = resolve_shadows varnames body in
            ((name, nameLoc), ((varname varnames arg, argLoc), body, patternLoc), loc)
        ) cases in
      `PatternMatch (receiver_id, cases, loc)
    | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs = resolve_shadows varnames rhs in
      let varnames = add_variable varnames name in
      let body = resolve_shadows varnames body in
      `Declaration (((varname varnames name), nameLoc), rhs, body, loc)
    | `VariableLookup (name, loc) ->
      let name = varname varnames name in
      `VariableLookup (name, loc)
    | `ExternalCall ((name, nameLoc), args, loc) ->
      let args = List.map (resolve_shadows varnames) args in
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
  let ast = resolve_shadows IntMap.empty ast in
  ast