module IntMap = Map.Make(struct type t = int let compare = compare end)
let collect_methods ast =
  let rec collect_methods methods ast =
    match ast with
    | `App _ -> Error.internal_error "`App should be eliminated"
    | `Lambda _ -> Error.internal_error "`Lambda should be eliminated"
    | `Dispatch -> Error.internal_error "`Dispatch should be eliminated"
    | `Method _ -> Error.internal_error "`Method should be eliminated"

    | `Abort a -> methods, `Abort a
    | `VariableLookup v -> methods,  `VariableLookup v
    | `EmptyPrototype e -> methods, `EmptyPrototype e
    | `StringLiteral s -> methods, `StringLiteral s
    | `FloatLiteral f -> methods, `FloatLiteral f
    | `IntLiteral i -> methods, `IntLiteral i
    | `Declaration (lhs, rhs, body, loc, fvs) ->
      let methods, rhs = collect_methods methods rhs in 
      let methods, body = collect_methods methods body in
      methods, `Declaration (lhs, rhs, body, loc, fvs)
    | `Drop (v, body) -> 
      let methods, body = collect_methods methods body in
      methods, `Drop (v, body)
    | `Dup (v, body) ->
      let methods, body = collect_methods methods body in
      methods, `Dup (v, body)
    | `ExternalCall (fn, args, loc, fvs) ->
      let methods, args = List.fold_left_map collect_methods methods args in
      methods, `ExternalCall (fn, args, loc, fvs)
    | `MethodInvocation (receiver, label, args, loc, fvs) ->
      let methods, receiver = collect_methods methods receiver in
      let methods, args = List.fold_left_map (
          fun methods (name, expr, loc) ->
            let methods, expr = collect_methods methods expr in
            methods, (name, expr, loc)
        ) methods args in
      methods, `MethodInvocation (receiver, label, args, loc, fvs)
    | `PatternMatch (v, cases, loc, fvs) ->
      let methods, cases = List.fold_left_map (
          fun methods (tag, (arg, body, patternLoc), loc) ->
            let methods, body = collect_methods methods body in
            methods, (tag, (arg, body, patternLoc), loc)
        ) methods cases in
      methods, `PatternMatch (v, cases, loc, fvs)
    | `TaggedObject (tag, value, loc, fvs) ->
      let methods, value = collect_methods methods value in 
      methods, `TaggedObject (tag, value, loc, fvs)
    | `PrototypeCopy (ext, (tag, meth, fieldLoc), loc, op, fvs) ->
      let methods, ext = collect_methods methods ext in 
      let (args, body, methLoc, meth_fvs) = meth in 
      let methods, body = collect_methods methods body in
      let method_id = Oo.id (object end) in
      let methods = IntMap.add method_id (args, body, methLoc, meth_fvs) methods in
      methods, `PrototypeCopy (ext, (tag, (method_id, meth_fvs), fieldLoc), loc, op, fvs)
  in collect_methods IntMap.empty ast