let rec free_variables ast =
  match ast with
  | `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc, op)
    ->
      let ext_fvs, ext = free_variables ext in
      let body_fvs, body = free_variables body in
      let meth_fvs =
        List.fold_left
          (fun fvs (name, _) -> AstTypes.IntSet.remove name fvs)
          body_fvs args
      in
      let fvs = AstTypes.IntSet.union ext_fvs meth_fvs in
      ( fvs,
        `PrototypeCopy
          ( ext,
            ((name, labelLoc), (args, body, methLoc, meth_fvs), fieldLoc),
            size,
            loc,
            op,
            fvs ) )
  | `TaggedObject ((tag, tagLoc), value, loc) ->
      let value_fvs, value = free_variables value in
      (value_fvs, `TaggedObject ((tag, tagLoc), value, loc, value_fvs))
  | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let arg_fvs, args =
        List.fold_left_map
          (fun fvs arg ->
            let name, expr, loc = arg in
            let arg_fvs, expr = free_variables expr in
            (AstTypes.IntSet.union fvs arg_fvs, (name, expr, loc)))
          AstTypes.IntSet.empty args
      in
      let receiver_fvs, receiver = free_variables receiver in
      let fvs = AstTypes.IntSet.union arg_fvs receiver_fvs in
      (fvs, `MethodInvocation (receiver, (name, labelLoc), args, loc, fvs))
  | `PatternMatch (receiver_id, cases, loc) ->
      let receiver_fvs = AstTypes.IntSet.singleton receiver_id in
      let cases_fvs, cases =
        List.fold_left_map
          (fun fvs ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let body_fvs, body = free_variables body in
            let body_fvs = AstTypes.IntSet.remove arg body_fvs in
            ( AstTypes.IntSet.union fvs body_fvs,
              ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ))
          AstTypes.IntSet.empty cases
      in
      let fvs = AstTypes.IntSet.union receiver_fvs cases_fvs in
      (fvs, `PatternMatch (receiver_id, cases, loc, fvs))
  | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs_fvs, rhs = free_variables rhs in
      let body_fvs, body = free_variables body in
      let body_fvs = AstTypes.IntSet.remove name body_fvs in
      let fvs = AstTypes.IntSet.union rhs_fvs body_fvs in
      (fvs, `Declaration ((name, nameLoc), rhs, body, loc, fvs))
  | `VariableLookup (name, loc) ->
      let fvs = AstTypes.IntSet.singleton name in
      (fvs, `VariableLookup (name, loc, fvs))
  | `ExternalCall ((name, nameLoc), args, loc) ->
      let fvs, args =
        List.fold_left_map
          (fun fvs arg ->
            let arg_fvs, arg = free_variables arg in
            (AstTypes.IntSet.union fvs arg_fvs, arg))
          AstTypes.IntSet.empty args
      in
      (fvs, `ExternalCall ((name, nameLoc), args, loc, fvs))
  | `IntProto _ as i -> (AstTypes.IntSet.empty, i)
  | `StrProto (s, loc) -> (AstTypes.IntSet.empty, `StrProto (s, loc))
  | `Abort loc -> (AstTypes.IntSet.empty, `Abort loc)
  | `EmptyPrototype loc -> (AstTypes.IntSet.empty, `EmptyPrototype loc)
