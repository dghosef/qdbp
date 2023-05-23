let dummy_pos = Lexing.dummy_pos
let newvar _ = 
  (Oo.id (object end))
let decl (l: int) (r: 'a) (body: 'a) (loc: Lexing.position * Lexing.position) =
  `Declaration ((l, loc), r, body, loc)




let id x = x

(* e is the expr to be ANF'd *)
(* k is a function that takes the result of e and does something with it *)
(* the arg to k must be atomic *)
let rec anf e k  =
  let anf_simple e k loc =
    let var = newvar () in
    decl
      var
      e
      (k (`VariableLookup (var, loc)))
      loc
  in
  (* The return should be semantically equivalent to k e *)
  match e with 
  | `Abort _ as a ->
    k a
  | `EmptyPrototype _ as e ->
    k e
  | `IntProto _ as i ->
    k i
  | `FloatLiteral _ as f ->
    k f
  | `IntLiteral _ as i ->
    k i
  | `StringLiteral _ as s ->
    k s
  | `VariableLookup (name, loc, _)  ->
    k (`VariableLookup (name, loc))

  | `Declaration ((x, xLoc), r, body, loc, _) ->
    anf r (fun r ->
        `Declaration ((x, xLoc), r, (anf body k), loc)
      )
  | `Drop (v, e) ->
    (*
      let _ = drop(v) in e
      anf (drop v) (fun r -> let _ = r in (anf e k))
      let _ = (drop v) in (anf e k)
      Drop 
    *)
    `Drop(v, (anf e k))
  | `Dup (v, e) ->
    `Dup(v, (anf e k))

  | `PatternMatch (x, cases, loc, _) ->
    let cases = List.map (
        fun (n, (arg, body, patternLoc), loc) ->
          let body = anf body id in
          (n, (arg, body, patternLoc), loc))
        cases
    in
    k (`PatternMatch (x, cases, loc))
  | `PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), size, loc, op, _) ->
    let (args, body, methLoc, methFvs) = meth in
    anf ext (fun ext ->
        let body = anf body id in
        let meth = (args, body, methLoc, methFvs) in
        anf_simple
          (`PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), size, loc, op))
          k loc
      )
  | `TaggedObject ((tag, tagLoc), payload, loc, _) ->
    anf payload (fun payload ->
        anf_simple (`TaggedObject ((tag, tagLoc), payload, loc)) k loc
      )
  | `MethodInvocation (receiver, (name, nameLoc), args, loc, _) ->
    anf receiver (fun receiver ->
        (* https://compiler.club/anf-conversion/ *)
        (*
        anf arg0 -> (fun arg0 ->
          anf arg1 (fun arg1 ->
              ...
              anf argn (fun argn ->
                anf_simple `MethodInvocation (receiver, (name, nameLoc), [arg0...argn], loc)
                  k loc)))
        *)
        let accumulate arg ctx vs = 
          let (name, e, argLoc) = arg in 
          anf e (fun v -> ctx ((name, v, argLoc) :: vs))
        in
        let base vs =
          anf_simple (`MethodInvocation (receiver, (name, nameLoc), (List.rev vs), loc)) k loc
        in
        List.fold_right accumulate args base [] 
      )
  | `ExternalCall (name, args, loc, _) ->
    (* https://compiler.club/anf-conversion/ *)
    let accumulate arg ctx vs = 
      anf arg (fun v -> ctx (v :: vs))
    in
    let base vs =
      anf_simple (`ExternalCall (name, (List.rev vs), loc)) k loc
    in
    List.fold_right accumulate args base [] 


  | `App _ ->
    Error.internal_error "Should not have `App here"
  | `Lambda _ ->
    Error.internal_error "Should not have `Lambda here"
  | `Method _ ->
    Error.internal_error "Should not have `Method here"
  | `Dispatch ->
    Error.internal_error "Should not have `Dispatch here"