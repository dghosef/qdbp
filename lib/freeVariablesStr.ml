module FvSet = Set.Make(String)
(* FIXME: Make part of the free_variables function for efficiency *)
let rec is_pure ast = match ast with
  | `PrototypeCopy (ext, _, _, _, _) -> is_pure ext
  | `TaggedObject (_, value, _) -> is_pure value
  | `MethodInvocation (receiver, _, args, _) ->
    is_pure receiver && List.for_all (fun (_, arg, _) -> is_pure arg) args
  | `PatternMatch (receiver, cases, _) ->
    is_pure receiver && List.for_all (fun (_, (_, body, _), _) -> is_pure body) cases
  | `Declaration (_, rhs, body, _) ->
    is_pure rhs && is_pure body
  | `VariableLookup _ -> true
  | `ExternalCall ((name, _), args, _) ->
    List.for_all is_pure args &&
    (String.ends_with ~suffix: "__PURE" name)
  | `IntLiteral _ -> true
  | `FloatLiteral _ -> true
  | `StringLiteral _ -> true
  | `Abort _ -> true
  | `EmptyPrototype _ -> true
  | `IntProto _ -> true

let rec free_variables ast =
  match ast with
  | `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc, op) -> 
    let ext_fvs, ext = free_variables ext in
    let body_fvs, body = free_variables body in
    let meth_fvs = List.fold_left (
        fun fvs (name, _) ->
          FvSet.remove name fvs
      ) body_fvs args in
    let fvs = FvSet.union ext_fvs meth_fvs in 
    fvs, 
    `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), size, loc, op)
  | `TaggedObject ((tag, tagLoc), value, loc) -> 
    let value_fvs, value = free_variables value in
    value_fvs,
    `TaggedObject ((tag, tagLoc), value, loc)
  | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
    let arg_fvs, args = List.fold_left_map (
        fun fvs arg ->
          let (name, expr, loc) = arg in 
          let arg_fvs, expr = free_variables expr in
          FvSet.union fvs arg_fvs, (name, expr, loc)
      ) FvSet.empty args in
    let receiver_fvs, receiver = free_variables receiver in
    let fvs = FvSet.union arg_fvs receiver_fvs in 
    fvs,
    `MethodInvocation (receiver, (name, labelLoc), args, loc)
  | `PatternMatch (receiver, cases, loc) -> 
    let receiver_fvs, receiver = free_variables receiver in
    let cases_fvs, cases = List.fold_left_map (
        fun fvs ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
          let body_fvs, body = free_variables body in
          let body_fvs = FvSet.remove arg body_fvs in
          FvSet.union fvs body_fvs, ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc)
      ) FvSet.empty cases in
    let fvs = FvSet.union receiver_fvs cases_fvs in 
    fvs, `PatternMatch (receiver, cases, loc)
  | `Declaration ((name, nameLoc), rhs, body, loc) ->
    let body_fvs, body = free_variables body in
    let rhs_is_pure = is_pure rhs in
    if (not (FvSet.mem name body_fvs)) && rhs_is_pure  then
      (* If the rhs is pure, we can remove the declaration *)
      (body_fvs, body)
    else
      let rhs_fvs, rhs = free_variables rhs in
      let body_fvs = FvSet.remove name body_fvs in
      let fvs = FvSet.union rhs_fvs body_fvs in 
      fvs, `Declaration ((name, nameLoc), rhs, body, loc)
  | `VariableLookup (name, loc) ->
    let fvs = FvSet.singleton name in 
    fvs,
    `VariableLookup (name, loc)
  | `ExternalCall ((name, nameLoc), args, loc) ->
    let fvs, args = List.fold_left_map (
        fun fvs arg ->
          let arg_fvs, arg = free_variables arg in
          FvSet.union fvs arg_fvs, arg
      ) FvSet.empty args in
    fvs, `ExternalCall ((name, nameLoc), args, loc)
  | `IntLiteral (i, loc) ->
    FvSet.empty, `IntLiteral (i, loc)
  | `FloatLiteral (f, loc) -> 
    FvSet.empty, `FloatLiteral (f, loc)
  | `StringLiteral (s, loc) ->
    FvSet.empty, `StringLiteral (s, loc)
  | `Abort loc ->
    FvSet.empty, `Abort loc
  | `EmptyPrototype loc ->
    FvSet.empty, `EmptyPrototype loc
  | `IntProto _ as i ->
    FvSet.empty, i