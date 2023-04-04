module ImportMap = Map.Make(String)
module ImportSet = Set.Make(String)
let resolve_imports ast =
  let rec resolve_imports already_seen state (ast: Types.ast) =
    match ast with 

    | `ExternalCall (_, args, _) ->
      List.fold_left (resolve_imports already_seen) state args
    | `EmptyPrototype _ -> 
      state
    | `PrototypeExtension (extension, (_, (_, body, _), _), _) ->
      let imports = resolve_imports already_seen state body in
      resolve_imports already_seen imports extension
    | `TaggedObject (_, obj, _) ->
      resolve_imports already_seen state obj
    | `MethodInvocation (obj, _, args, _) ->
      let arg_exprs = List.map (fun (_, expr, _) -> expr) args in
      let imports =
        List.fold_left (resolve_imports already_seen) state arg_exprs in
      resolve_imports already_seen imports obj
    | `PatternMatch (obj, cases, _) ->
      let case_body_exprs = List.map (fun (_, (_, body, _), _) -> body) cases in
      let imports =
        List.fold_left (resolve_imports already_seen) state case_body_exprs in
      resolve_imports already_seen imports obj
    | `Declaration (_, rhs, body, _) -> 
      let imports = resolve_imports already_seen state rhs in
      resolve_imports already_seen imports body
    | `VariableLookup _ ->
      state
    | `IntLiteral _ ->
      state
    | `FloatLiteral _ ->
      state
    | `StringLiteral _ ->
      state
    | `Abort _ ->
      state
    | `Import (filename, _) -> 
      if ImportSet.mem filename already_seen then
        failwith "cycle detected in imports"
      else
        let (imports, files) = state in
        if not (ImportMap.mem filename imports) then
          let import, files = ParserDriver.parse_file files filename in
          let already_seen = ImportSet.add filename already_seen  in
          let imports = ImportMap.add filename import imports in
          resolve_imports already_seen (imports, files) import
        else
          state
  in
  resolve_imports ImportSet.empty (ImportMap.empty, ParserDriver.FileMap.empty) ast