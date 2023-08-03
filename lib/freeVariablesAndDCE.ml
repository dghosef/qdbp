(* FIXME: Make part of the free_variables function for efficiency *)
let rec is_pure ast =
  match ast with
  | `PrototypeCopy (ext, _, _, _, _) -> is_pure ext
  | `TaggedObject (_, value, _) -> is_pure value
  | `MethodInvocation (receiver, _, args, _) ->
      is_pure receiver && List.for_all (fun (_, arg, _) -> is_pure arg) args
  | `PatternMatch (_, receiver, cases, _) ->
      is_pure receiver
      && List.for_all (fun (_, (_, body, _), _) -> is_pure body) cases
  | `Declaration (_, rhs, body, _) -> is_pure rhs && is_pure body
  | `VariableLookup _ -> true
  | `ExternalCall ((name, _), args, _) ->
      List.for_all is_pure args && String.ends_with ~suffix:"__PURE" name
  | `StrProto _ -> true
  | `Abort _ -> true
  | `EmptyPrototype _ -> true
  | `IntProto _ -> true

(* Remove free variables and perform basic dead code elimination*)
let rec free_variables ast =
  match ast with
  | `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc, op)
    ->
      let ext_fvs, ext = free_variables ext in
      let body_fvs, body = free_variables body in
      let meth_fvs =
        List.fold_left
          (fun fvs (name, _) -> AstTypes.StringSet.remove name fvs)
          body_fvs args
      in
      let fvs = AstTypes.StringSet.union ext_fvs meth_fvs in
      ( fvs,
        `PrototypeCopy
          ( ext,
            ((name, labelLoc), (args, body, methLoc), fieldLoc),
            size,
            loc,
            op ) )
  | `TaggedObject ((tag, tagLoc), value, loc) ->
      let value_fvs, value = free_variables value in
      (value_fvs, `TaggedObject ((tag, tagLoc), value, loc))
  | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let arg_fvs, args =
        List.fold_left_map
          (fun fvs arg ->
            let name, expr, loc = arg in
            let arg_fvs, expr = free_variables expr in
            (AstTypes.StringSet.union fvs arg_fvs, (name, expr, loc)))
          AstTypes.StringSet.empty args
      in
      let receiver_fvs, receiver = free_variables receiver in
      let fvs = AstTypes.StringSet.union arg_fvs receiver_fvs in
      (fvs, `MethodInvocation (receiver, (name, labelLoc), args, loc))
  | `PatternMatch (hasDefault, receiver, cases, loc) ->
      let receiver_fvs, receiver = free_variables receiver in
      let cases_fvs, cases =
        List.fold_left_map
          (fun fvs ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let body_fvs, body = free_variables body in
            let body_fvs = AstTypes.StringSet.remove arg body_fvs in
            ( AstTypes.StringSet.union fvs body_fvs,
              ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ))
          AstTypes.StringSet.empty cases
      in
      let fvs = AstTypes.StringSet.union receiver_fvs cases_fvs in
      (fvs, `PatternMatch (hasDefault, receiver, cases, loc))
  | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let body_fvs, body = free_variables body in
      let rhs_is_pure = is_pure rhs in
      if (not (AstTypes.StringSet.mem name body_fvs)) && rhs_is_pure then
        (* If the rhs is pure, we can remove the declaration *)
        (body_fvs, body)
      else
        let rhs_fvs, rhs = free_variables rhs in
        let body_fvs = AstTypes.StringSet.remove name body_fvs in
        let fvs = AstTypes.StringSet.union rhs_fvs body_fvs in
        (fvs, `Declaration ((name, nameLoc), rhs, body, loc))
  | `VariableLookup (name, loc) ->
      let fvs = AstTypes.StringSet.singleton name in
      (fvs, `VariableLookup (name, loc))
  | `ExternalCall ((name, nameLoc), args, loc) ->
      let fvs, args =
        List.fold_left_map
          (fun fvs arg ->
            let arg_fvs, arg = free_variables arg in
            (AstTypes.StringSet.union fvs arg_fvs, arg))
          AstTypes.StringSet.empty args
      in
      (fvs, `ExternalCall ((name, nameLoc), args, loc))
  | `StrProto (s, loc) -> (AstTypes.StringSet.empty, `StrProto (s, loc))
  | `Abort loc -> (AstTypes.StringSet.empty, `Abort loc)
  | `EmptyPrototype loc -> (AstTypes.StringSet.empty, `EmptyPrototype loc)
  | `IntProto _ as i -> (AstTypes.StringSet.empty, i)
