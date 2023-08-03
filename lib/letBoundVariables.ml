let rec let_bound_variables ast =
  match ast with
  | `Abort a -> (AstTypes.IntSet.empty, `Abort a)
  | `Declaration (lhs, rhs, body, loc) ->
      let bvs, rhs = let_bound_variables rhs in
      let bvs', body = let_bound_variables body in
      let bvs'' = AstTypes.IntSet.union bvs bvs' in
      let bvs'' = AstTypes.IntSet.add (fst lhs) bvs'' in
      (bvs'', `Declaration (lhs, rhs, body, loc, bvs''))
  | `VariableLookup v -> (AstTypes.IntSet.empty, `VariableLookup v)
  | `IntProto i -> (AstTypes.IntSet.empty, `IntProto i)
  | `EmptyPrototype e -> (AstTypes.IntSet.empty, `EmptyPrototype e)
  | `StrProto s -> (AstTypes.IntSet.empty, `StrProto s)
  | `MethodInvocation (receiver, label, args, loc) ->
      let bvs, receiver = let_bound_variables receiver in
      let bvs', args =
        List.fold_left_map
          (fun bvs (name, expr, loc) ->
            let bvs', expr = let_bound_variables expr in
            (AstTypes.IntSet.union bvs bvs', (name, expr, loc)))
          bvs args
      in
      let bvs'' = AstTypes.IntSet.union bvs bvs' in
      (bvs'', `MethodInvocation (receiver, label, args, loc, bvs''))
  | `Drop (v, value, cnt) ->
      let bvs, value = let_bound_variables value in
      (bvs, `Drop (v, value, cnt))
  | `Dup (v, value, cnt) ->
      let bvs, value = let_bound_variables value in
      (bvs, `Dup (v, value, cnt))
  | `PatternMatch (hasDefault, v, cases, loc) ->
      let bvs, cases =
        List.fold_left_map
          (fun bvs (tag, (arg, body, patternLoc), loc) ->
            let bvs', body = let_bound_variables body in
            let bvs'' = AstTypes.IntSet.union bvs bvs' in
            let bvs'' = AstTypes.IntSet.add (fst arg) bvs'' in
            (bvs'', (tag, (arg, body, patternLoc), loc)))
          AstTypes.IntSet.empty cases
      in
      (bvs, `PatternMatch (hasDefault, v, cases, loc, bvs))
  | `PrototypeCopy (ext, (tag, meth, fieldLoc), size, loc, op) ->
      let bvs, ext = let_bound_variables ext in
      (bvs, `PrototypeCopy (ext, (tag, meth, fieldLoc), size, loc, op, bvs))
  | `TaggedObject (tag, value, loc) ->
      let bvs, value = let_bound_variables value in
      (bvs, `TaggedObject (tag, value, loc, bvs))
  | `ExternalCall (fn, args, loc) ->
      let bvs, args =
        List.fold_left_map
          (fun bvs arg ->
            let bvs', arg = let_bound_variables arg in
            (AstTypes.IntSet.union bvs bvs', arg))
          AstTypes.IntSet.empty args
      in
      (bvs, `ExternalCall (fn, args, loc, bvs))
