module FvSet = Set.Make(String)
let rec free_variables ast =
  match ast with
  | `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op) -> 
    let ext_fvs, ext = free_variables ext in
    let body_fvs, body = free_variables body in
    let meth_fvs = List.fold_left (
        fun fvs (name, _) ->
          FvSet.remove name fvs
      ) body_fvs args in
    let fvs = FvSet.union ext_fvs meth_fvs in 
    fvs, 
    `PrototypeCopy
      (ext, ((name, labelLoc), (args, body, methLoc, meth_fvs), fieldLoc), loc, op, fvs)
  | `TaggedObject ((tag, tagLoc), value, loc) -> 
    let value_fvs, value = free_variables value in
    value_fvs,
    `TaggedObject ((tag, tagLoc), value, loc, value_fvs)
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
    `MethodInvocation (receiver, (name, labelLoc), args, loc, fvs)
  | `PatternMatch (receiver, cases, loc) -> 
    let receiver_fvs, receiver = free_variables receiver in
    let cases_fvs, cases = List.fold_left_map (
        fun fvs ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
          let body_fvs, body = free_variables body in
          let body_fvs = FvSet.remove arg body_fvs in
          FvSet.union fvs body_fvs, ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc)
      ) FvSet.empty cases in
    let fvs = FvSet.union receiver_fvs cases_fvs in 
    fvs, `PatternMatch (receiver, cases, loc, fvs)
  | `Declaration ((name, nameLoc), rhs, body, loc) ->
    let rhs_fvs, rhs = free_variables rhs in
    let body_fvs, body = free_variables body in
    let body_fvs = FvSet.remove name body_fvs in
    let fvs = FvSet.union rhs_fvs body_fvs in 
    fvs, `Declaration ((name, nameLoc), rhs, body, loc, fvs)
  | `VariableLookup (name, loc) ->
    let fvs = FvSet.singleton name in 
    fvs,
    `VariableLookup (name, loc, fvs)
  | `ExternalCall ((name, nameLoc), args, loc) ->
    let fvs, args = List.fold_left_map (
        fun fvs arg ->
          let arg_fvs, arg = free_variables arg in
          FvSet.union fvs arg_fvs, arg
      ) FvSet.empty args in
    fvs, `ExternalCall ((name, nameLoc), args, loc, fvs)
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