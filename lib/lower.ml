module StringMap = Map.Make(String)
(*
  Takes an AST and lowers it to a (very inefficient for now) C-like
  imperative IR
*)
let lower imports ast =
  let rec lower variable_ids ast =
    match ast with
    | `ExternalCall (name, args, _) ->
      let args = List.map (lower variable_ids) args in 
      `Call (name, args)
    | `EmptyPrototype _ -> 
      `EmptyPrototype
    | `PrototypeExtension (extension, ((name, _), (_, meth, _), _), _) ->
      `Extend (
        (lower variable_ids extension),
        name,
        lower variable_ids (`Method meth)
      )
    | `TaggedObject (tag, obj, _) ->
      `TaggedObject (tag, (lower variable_ids obj))
    | `MethodInvocation (obj, (name, _), args, _) ->
      let arg_exprs = List.map 
          (fun (_, ast, _) -> lower variable_ids ast)
          args
      in
      let arg = `List arg_exprs in
      `Call (`Lookup ((lower variable_ids obj), name), [arg])
    | `PatternMatch (obj, cases, _) ->
      let obj_id = StringMap.cardinal variable_ids in
      let variable_ids =
        StringMap.add "InternalVariantObject" obj_id variable_ids in
      let cases = List.fold_left
          (fun acc ((tag, _),((var, _), body, _) , _) ->
             `If (
               `TagIs (`Variable obj_id, `StringLiteral tag),
               (
                 `Declaration (
                   tag,
                   `GetTaggedValue (`Variable obj_id),
                   lower (StringMap.add var tag variable_ids) body
                 )
               ),
               acc
             )
          )
          (`PatternMatchError)
          cases
      in
      `Declaration (
        obj_id,
        lower variable_ids obj,
        cases
      )
    | `Declaration ((name, _), rhs, body, _) -> 
      let id = StringMap.cardinal variable_ids in
      `Declaration (
        id,
        lower variable_ids rhs,
        lower (StringMap.add name id variable_ids) body
      )
    | `VariableLookup (name, _) ->
      let name = (StringMap.find name variable_ids) in
      `Variable name
    | `IntLiteral (i, _) ->
      `IntLiteral i
    | `FloatLiteral (f, _) ->
      `FloatLiteral f
    | `StringLiteral (s, _) ->
      `StringLiteral s
    | `Abort _ ->
      `Abort
    | `Import (filename, _) -> 
      (* FIXME: Cache somehow *)
      lower variable_ids (ResolveImports.ImportMap.find filename imports)
    | `Method (arg_list, body, _) ->
      let arg_names = List.map fst arg_list in
      `Function (arg_names, lower variable_ids body)
  in
  lower StringMap.empty ast