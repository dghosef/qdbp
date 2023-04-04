module ImportMap = Map.Make(String)
module ImportSet = Set.Make(String)
let resolve_imports (ast: Types.ast) =
  let rec resolve_imports already_seen imports (ast: Types.ast) =
    (* Recursively descend down AST and
       Check for cycles
       Keep a map of already imported files. Update it at imports
    *)
    match ast with 

    | `ExternalCall (_, args, _) ->
      List.fold_left (resolve_imports already_seen) imports args
    | `EmptyPrototype _ -> 
      imports
    | `PrototypeExtension (extension, (_, (_, body, _), _), _) ->
      let imports = resolve_imports already_seen imports body in
      resolve_imports already_seen imports extension
    | `TaggedObject (_, obj, _) ->
      resolve_imports already_seen imports obj
    | `MethodInvocation (obj, _, args, _) ->
      let arg_exprs = List.map (fun (_, expr, _) -> expr) args in
      let imports =
        List.fold_left (resolve_imports already_seen) imports arg_exprs in
      resolve_imports already_seen imports obj
    | `PatternMatch (obj, cases, _) ->
      let case_body_exprs = List.map (fun (_, (_, body, _), _) -> body) cases in
      let imports =
        List.fold_left (resolve_imports already_seen) imports case_body_exprs in
      resolve_imports already_seen imports obj
    | `Declaration (_, rhs, body, _) -> 
      let imports = resolve_imports already_seen imports rhs in
      resolve_imports already_seen imports body
    | `VariableLookup _ ->
      imports
    | `IntLiteral _ ->
      imports
    | `FloatLiteral _ ->
      imports
    | `StringLiteral _ ->
      imports
    | `Abort _ ->
      imports
    | `Import (filename, _) -> 
      if ImportSet.mem filename already_seen then
        failwith "cycle detected in imports"
      else
        if not (ImportMap.mem filename imports) then
          let import = ParserDriver.parse_file filename in
          let already_seen = ImportSet.add filename already_seen  in
          let imports = ImportMap.add filename import imports in
          resolve_imports already_seen imports import
        else
          imports
  in
  resolve_imports ImportSet.empty ImportMap.empty ast