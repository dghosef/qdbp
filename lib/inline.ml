module StringMap = Map.Make(String)
module FvSet = FreeVariablesStr.FvSet
let rename_args arg_ids fvs body =
  let argmap = List.fold_left (fun argmap (id, _) ->
      StringMap.add id ("V" ^ (string_of_int (Oo.id (object end)))) argmap) StringMap.empty arg_ids in
  let argmap = FvSet.fold (fun v argmap -> StringMap.add v v argmap) fvs argmap
  in
  let varname varmap name =
    StringMap.find name varmap
  in
  let rec rename varmap body =
    match body with 
    | `PatternMatch (receiver, cases, loc) ->
      let receiver = rename varmap receiver in 
      let cases = List.map
          (fun ((tag, tagLoc), ((arg, argLoc), body, patternLoc), caseLoc) ->
             let varmap = StringMap.add arg arg varmap in
             let body = rename varmap body in
             ((tag, tagLoc), ((arg, argLoc), body, patternLoc), caseLoc)
          ) cases in
      `PatternMatch (receiver, cases, loc)
    | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let receiver = rename varmap receiver in
      let args = List.map(fun (name, a, loc) -> let a = rename varmap a in
                           (name, a, loc)) args in
      `MethodInvocation (receiver, (name, labelLoc), args, loc)
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op) ->
      let ext = rename varmap ext in
      let varmap = List.fold_left (fun varmap arg -> StringMap.add (fst arg) (fst arg) varmap)
          varmap args in
      let body = rename varmap body in
      `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op)
    | `TaggedObject (tag, value, loc) ->
      `TaggedObject (tag, rename varmap value, loc)
    | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs = rename varmap rhs in 
      let varmap = StringMap.add name name varmap in
      let body = rename varmap body in
      `Declaration ((name, nameLoc), rhs, body, loc)
    | `VariableLookup (v, loc) ->
      `VariableLookup(varname varmap v, loc)
    | `ExternalCall (name, args, loc) ->
      let args = List.map (rename varmap) args in
      `ExternalCall (name, args, loc)
    | `Abort _ as a -> a
    | `StringLiteral _ as s -> s
    | `FloatLiteral _ as f -> f
    | `IntLiteral _ as i -> i
    | `EmptyPrototype _ as e -> e
  in
  let arg_ids = List.map (fun (id, loc) -> (varname argmap id, loc)) arg_ids in
  arg_ids, rename argmap body

let inline expr =
  let can_inline proto label env = 
    match StringMap.find_opt label proto with
    | Some (_, _, _, fvs) ->
      FvSet.for_all (fun fv -> StringMap.mem fv env) fvs
    | None -> false
  in
  let rec inline env expr =
    match expr with
    | `PatternMatch (receiver, cases, loc, _) ->
      let peval_receiver, receiver = inline env receiver in
      begin
        match (peval_receiver) with
        | `Variant (label, payload) ->
          let case = List.find (
              fun ((tag, _), _, _) ->
                label = tag) cases in
          let (tag, ((argname, argloc), body, patternLoc), caseLoc) = case in
          let env = StringMap.add argname payload
              env in 
          let pevalbody, body = inline env body in
          pevalbody, `PatternMatch (receiver, [(tag, ((argname, argloc), body, patternLoc), caseLoc)], loc) 
        | `Unit -> 
          let cases = List.map (fun ((tag, tagLoc), ((arg, argLoc), body, patternLoc), caseLoc) ->
              let env = StringMap.add arg `Unit env in
              let _, body = inline env body in
              ((tag, tagLoc), ((arg, argLoc), body, patternLoc), caseLoc)
            ) cases in
          `Unit, `PatternMatch (receiver, cases, loc)
        | `Proto _ -> Error.internal_error "Shouldn't have proto here"
      end
    | `MethodInvocation (receiver, (name, labelLoc), args, loc, _) ->
      let peval_receiver, receiver = inline env receiver in
      begin
        match peval_receiver with
        | `Variant _ -> Error.internal_error "Expected proto or unit"
        | `Proto p when
            (can_inline p name env) ->
          let (arg_ids, body, _, methFvs) = StringMap.find name p in
          let arg_ids, body = rename_args arg_ids methFvs body in
          let arg_exprs = List.map(fun (_, e, _) -> (let _, e = inline env e
                                                     in e)) args in
          let args = List.combine arg_exprs arg_ids in
          `Unit, List.fold_right (fun (arg, arg_id) e->
              `Declaration(arg_id, arg, e, loc)
            ) args body
        | `Unit | `Proto _ ->
          let args = List.map (fun (name, arg, loc) ->
              let _, arg = inline env arg in
              (name, arg, loc)) args in
          `Unit, `MethodInvocation (receiver, (name, labelLoc), args, loc)
      end
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc, meth_fvs), fieldLoc), loc, op, _) ->
      let peval_ext, ext = inline env ext in
      (* Add each arg to env *)
      let env = List.fold_left (fun env (name, _) ->
          StringMap.add name `Unit env) env args in
      let _, body = inline env body in
      let peval =
        match peval_ext with
        | `Proto proto ->
          `Proto (StringMap.add name (args, body, methLoc, meth_fvs) proto)
        | `Unit -> `Unit
        | `Variant _  -> Error.internal_error "Expected proto or unit"
      in
      peval, `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op)

    | `TaggedObject ((tag, tagLoc), value, loc, _) -> 
      let peval_value, value = inline env value in
      `Variant (tag, peval_value), `TaggedObject ((tag, tagLoc), value, loc) 
    | `Declaration ((name, nameLoc), rhs, body, loc, _) ->
      let peval_rhs, rhs = inline env rhs in
      let env = StringMap.add name peval_rhs env in
      let peval_body, body = inline env body in
      peval_body, `Declaration ((name, nameLoc), rhs, body, loc)
    | `VariableLookup (v, loc, _) ->
      StringMap.find v env, `VariableLookup (v, loc)
    | `ExternalCall (name, args, loc, _) ->
      let args = List.map (fun arg -> 
          let _, e = inline env arg in e) args in
      `Unit, `ExternalCall (name, args, loc)

    | `EmptyPrototype _ as e ->
      `Proto StringMap.empty, e
    | `IntLiteral _ as i ->
      `Unit, i
    | `FloatLiteral _ as f -> 
      `Unit, f
    | `StringLiteral _ as s ->
      `Unit, s
    | `Abort _ as a ->
      `Unit, a
  in
  let _, expr = inline StringMap.empty expr in
  expr
