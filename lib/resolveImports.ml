module ImportMap = Map.Make(String)
let merge_maps maps =
  List.fold_left (fun acc m -> ImportMap.merge (fun _ a b -> match a, b with
    | Some x, Some _ -> Some x
    | Some x, None -> Some x
    | None, Some y -> Some y
    | None, None -> None) acc m) ImportMap.empty maps
let rec resolve_imports (ast: Types.ast) =
  (* Recursively descend down AST and
     Check for cycles
     Keep a map of already imported files. Update it at imports
  *)
  match ast with 

  | `ExternalCall (_, args, _) ->
    let resolved = List.map resolve_imports args in
    merge_maps resolved
  | `EmptyPrototype _ -> ImportMap.empty
  | `PrototypeExtension (extension, (_, (_, body, _), _), _) ->
    merge_maps [resolve_imports extension; resolve_imports body]
  | `TaggedObject (_, obj, _) -> resolve_imports obj
  | `MethodInvocation (obj, _, args, _) ->
    let args = List.map (fun (_, arg, _) -> resolve_imports arg) args in
    merge_maps (resolve_imports obj :: args)
  | `PatternMatch (obj, cases, _) ->
    let cases = List.map (fun (_, (_, body, _), _) -> resolve_imports body) cases in
    merge_maps (resolve_imports obj :: cases)
  | `Declaration (_, rhs, body, _) -> 
    merge_maps [resolve_imports rhs; resolve_imports body]
  | `VariableLookup _ -> ImportMap.empty
  | `IntLiteral _ -> ImportMap.empty
  | `FloatLiteral _ -> ImportMap.empty
  | `StringLiteral _ -> ImportMap.empty
  | `Abort _ -> ImportMap.empty
  | `Import (_, _) -> 
    raise (Failure "Come back and figure this out later")