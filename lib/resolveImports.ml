module ImportMap = Map.Make(String)
module ImportSet = Set.Make(String)
let build_import_map files ast =
  let rec build_import_map already_seen state ast =
    match ast with 

    | `ExternalCall (_, args, _) ->
      List.fold_left (build_import_map already_seen) state args
    | `EmptyPrototype _ -> 
      state
    | `PrototypeCopy (extension, (_, (_, body, _), _), _, _) ->
      let imports = build_import_map already_seen state body in
      build_import_map already_seen imports extension
    | `TaggedObject (_, obj, _) ->
      build_import_map already_seen state obj
    | `MethodInvocation (obj, _, args, _) ->
      let arg_exprs = List.map (fun (_, expr, _) -> expr) args in
      let imports =
        List.fold_left (build_import_map already_seen) state arg_exprs in
      build_import_map already_seen imports obj
    | `PatternMatch (obj, cases, _) ->
      let case_body_exprs = List.map (fun (_, (_, body, _), _) -> body) cases in
      let imports =
        List.fold_left (build_import_map already_seen) state case_body_exprs in
      build_import_map already_seen imports obj
    | `Declaration (_, rhs, body, _) -> 
      let imports = build_import_map already_seen state rhs in
      build_import_map already_seen imports body
    | `VariableLookup _ ->
      state
    | `StringLiteral _ ->
      state
    | `IntProto _ ->
      state
    | `Abort _ ->
      state
    | `Import (filename, loc) -> 
      let (imports, files) = state in
      if ImportSet.mem filename already_seen then
        Error.compile_error
          "cycle detected in imports"  loc files
      else
      if not (ImportMap.mem filename imports) then
        let files, import = ParserDriver.parse_file files filename in
        let already_seen = ImportSet.add filename already_seen  in
        let imports = ImportMap.add filename import imports in
        build_import_map already_seen (imports, files) import
      else
        state
  in
  build_import_map ImportSet.empty (ImportMap.empty, files) ast

let resolve_imports imports ast =
  let rec resolve_imports ast =
    match ast with
    | `EmptyPrototype loc ->
      `EmptyPrototype loc
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc) -> 
      let ext = resolve_imports ext in
      let body = resolve_imports body in
      `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc)
    | `TaggedObject ((tag, tagLoc), value, loc) -> 
      let value = resolve_imports value in
      `TaggedObject ((tag, tagLoc), value, loc)
    | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let args = List.map (fun (name, arg, loc) -> name, (resolve_imports arg), loc) args in
      let receiver = resolve_imports receiver in
      `MethodInvocation (receiver, (name, labelLoc), args, loc)
    | `PatternMatch (receiver, cases, loc) -> 
      let receiver = resolve_imports receiver in
      let cases = List.map (
          fun ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let body = resolve_imports body in
            ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc)
        ) cases in
      `PatternMatch (receiver, cases, loc)
    | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs = resolve_imports rhs in
      let body = resolve_imports body in
      `Declaration ((name, nameLoc), rhs, body, loc)
    | `VariableLookup (name, loc) ->
      `VariableLookup (name, loc)
    | `Import (filename, _) ->
      let import = ImportMap.find filename imports in
      resolve_imports import
    | `ExternalCall ((name, nameLoc), args, loc) ->
      let args = List.map (resolve_imports) args in
      `ExternalCall ((name, nameLoc), args, loc)
    | `StringLiteral (s, loc) ->
      `StringLiteral (s, loc)
    | `Abort loc ->
      `Abort loc
    | `IntProto i ->
      `IntProto i
  in 
  resolve_imports ast