let fv expr =
  match expr with
  | `PrototypeCopy (_, _, _, _, _, fvs) -> fvs
  | `TaggedObject (_, _, _, fvs) -> fvs
  | `MethodInvocation (_, _, _, _, fvs) -> fvs
  | `PatternMatch (_, _, _, _, fvs) -> fvs
  | `Declaration (_, _, _, _, fvs) -> fvs
  | `VariableLookup (_, _, fvs) -> fvs
  | `ExternalCall (_, _, _, fvs) -> fvs
  | `EmptyPrototype _ -> AstTypes.IntSet.empty
  | `StrProto _ -> AstTypes.IntSet.empty
  | `Abort _ -> AstTypes.IntSet.empty
  | `Method (_, _, _, fvs) -> fvs
  | `Dispatch -> AstTypes.IntSet.empty
  | `IntProto _ -> AstTypes.IntSet.empty

let rec get_args = function `App (arg, rest) -> arg :: get_args rest | _ -> []

let rec get_body = function
  | `Lambda (_, body) -> get_body body
  | `Drop (v, body) -> `Drop (v, get_body body)
  | `Dup (v, body) -> `Dup (v, get_body body)
  | body -> body

let rec refcount delta gamma ast =
  assert (AstTypes.IntSet.disjoint delta gamma);
  match ast with
  | `Declaration ((x, xLoc), e1, e2, loc, fvs) ->
      (*
    x ∈ fv(e2)
    x̸ \∈ Δ, Γ
    Δ, Γ2 | Γ − Γ2 ⊢s e1 ⇝ e1'
    Δ | Γ2, x ⊢s e2 ⇝ e2'
    Γ2 = Γ ∩ (fv(e2) − x )
    ____________________________________________[sbind]
    Δ | Γ ⊢s val x = e1; e2 ⇝ val x = e1'; e2'   
    *)
      if AstTypes.IntSet.mem x (fv e2) then (
        assert (not (AstTypes.IntSet.mem x delta));
        assert (not (AstTypes.IntSet.mem x gamma));
        let gamma2 =
          AstTypes.IntSet.inter gamma (AstTypes.IntSet.remove x (fv e2))
        in
        let e1' =
          refcount
            (AstTypes.IntSet.union delta gamma2)
            (AstTypes.IntSet.diff gamma gamma2)
            e1
        in
        let e2' = refcount delta (AstTypes.IntSet.add x gamma2) e2 in
        `Declaration ((x, xLoc), e1', e2', loc, fvs)
        (*
    x̸ \∈ fv(e2), Δ, Γ
    Δ, Γ2 | Γ − Γ2 ⊢s e1 ⇝ e1'
    Δ | Γ2 ⊢s e2 ⇝ e2'
    Γ2 = Γ ∩ fv(e2)
    ____________________________________________________[sbind-drop]
    Δ | Γ ⊢s val x = e1; e2 ⇝ val x = e1'; drop x ; e'
    *))
      else if not (AstTypes.IntSet.mem x (fv e2)) then (
        assert (not (AstTypes.IntSet.mem x delta));
        assert (not (AstTypes.IntSet.mem x gamma));
        let gamma2 = AstTypes.IntSet.inter gamma (fv e2) in
        let e1' =
          refcount
            (AstTypes.IntSet.union delta gamma2)
            (AstTypes.IntSet.diff gamma gamma2)
            e1
        in
        let e2' = refcount delta gamma2 e2 in
        `Declaration ((x, xLoc), e1', `Drop (x, e2'), loc, fvs))
      else Error.internal_error "refcount: declaration"
  | `TaggedObject ((tag, tagLoc), v, loc, fvs) ->
      (*
    Δ, Γi+1, ..., Γn | Γi ⊢ s vi ⇝ v'
    i 1 ⩽ i ⩽ n Γi = (Γ − Γi+1 − ... − Γn ) ∩ fv(vi )
    ___________________________________________________[scon]
    Δ | Γ ⊢ s C v1 ... vn ⇝ C v'1 ... v'n   

    n=1
    *)
      let gamma1 = AstTypes.IntSet.inter gamma (fv v) in
      let v' = refcount delta gamma1 v in
      `TaggedObject ((tag, tagLoc), v', loc, fvs)
  | `VariableLookup (x, loc, fvs) ->
      (*
    ______________[svar]
    Δ| x ⊢s x ⇝ x    
    *)
      (*
    _________________________[svar-dup]
    Δ, x | ∅ ⊢s x ⇝ dup x ; x    
    *)
      if AstTypes.IntSet.mem x gamma then `VariableLookup (x, loc, fvs)
      else if AstTypes.IntSet.mem x delta then
        `Dup (x, `VariableLookup (x, loc, fvs))
      else
        Error.internal_error "refcount: variable lookup not in delta or gamma"
  | `StrProto (s, loc) -> `StrProto (s, loc)
  | `Abort loc -> `Abort loc
  | `EmptyPrototype loc -> `EmptyPrototype loc
  | `PatternMatch (hasDefault, x, cases, loc, fvs) ->
      assert (AstTypes.IntSet.mem x gamma);
      let gamma = AstTypes.IntSet.remove x gamma in
      let cases =
        List.map
          (fun ((name, nameLoc), ((bv_pi, bv_pi_loc), ei, methLoc), loc) ->
            (*
          Δ | Γi ⊢s ei ⇝ e'i
          Γi = (Γ, bv(pi)) ∩ fv(ei ) Γ'
          i = (Γ, bv(pi)) − Γi
          ______________________________________________________________________[smatch]
          Δ | Γ, x ⊢s match x { pi ↦ → ei } ⇝ match x { pi ↦ → drop Γ'i ; e'i }   
          *)
            let gamma_i =
              AstTypes.IntSet.inter (AstTypes.IntSet.add bv_pi gamma) (fv ei)
            in
            let gamma_i' =
              AstTypes.IntSet.diff (AstTypes.IntSet.add bv_pi gamma) gamma_i
            in
            let ei' = refcount delta gamma_i ei in
            let body =
              AstTypes.IntSet.fold (fun v e -> `Drop (v, e)) gamma_i' ei'
            in
            (* push down drop/dup *)
            let body = `Drop (x, body) in
            let body = `Dup (bv_pi, body) in
            ((name, nameLoc), ((bv_pi, bv_pi_loc), body, methLoc), loc))
          cases
      in
      `PatternMatch (hasDefault, x, cases, loc, fvs)
  | `ExternalCall ((name, nameLoc), args, loc, fvs) ->
      (* Convert e to lambda app form *)
      (* Refcount app *)
      let app = refcount_app delta gamma `Dispatch args `Dispatch in
      let args = get_args app in
      let args = List.tl args in
      `ExternalCall ((name, nameLoc), args, loc, fvs)
  | `Dispatch -> `Dispatch
  | `MethodInvocation (receiver, (name, nameLoc), args, loc, fvs) ->
      let arg_exprs = List.map (fun (_, e, _) -> e) args in
      let app = refcount_app delta gamma receiver arg_exprs `Dispatch in
      let app_args = get_args app in
      let receiver = List.hd app_args in
      (* Naively, `receiver` is always dupped here and then dropped upon dispatch *)
      (* Instead, we can skip the dup and on dispatch skip the drop *)
      let receiver = match receiver with `Dup (_, e) -> e | e -> e in
      let arg_exprs = List.tl app_args in
      let args =
        List.map2 (fun (name, _, loc) e -> (name, e, loc)) args arg_exprs
      in
      `MethodInvocation (receiver, (name, nameLoc), args, loc, fvs)
  | `Method (args, e, loc, ys) ->
      (* Combine these two rules cuz some parameters are free and some aren't *)
      (*
    x ∈ fv(e)
    ∅ | ys, x ⊢s e ⇝ e 
    ys = fv(𝜆x . e)
    Δ1 = ys − Γ
    ________________________________________[slam]
    Δ, Δ1 | Γ ⊢s 𝜆x . e ⇝ dup Δ1; 𝜆ys x . e 
    *)
      (*
    x̸ \∈ fv(e)
    ∅ | ys ⊢s e ⇝ e'
    ys = fv(𝜆x . e)
    Δ1 = ys − Γ
    ___________________________________________________[slam-drop]
    Δ, Δ1 | Γ ⊢s 𝜆x . e ⇝ dup Δ1; 𝜆ys x . (drop x ; e')    
    *)
      let arg_set = AstTypes.IntSet.of_list (List.map fst args) in
      let delta1 = AstTypes.IntSet.diff ys gamma in
      let e' =
        refcount AstTypes.IntSet.empty (AstTypes.IntSet.union ys arg_set) e
      in
      assert (AstTypes.IntSet.subset delta1 delta);
      let drops = AstTypes.IntSet.diff arg_set (fv e) in
      let e' = AstTypes.IntSet.fold (fun v e -> `Drop (v, e)) drops e' in
      `Method (args, e', loc, ys, delta1)
  | `PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), size, loc, op, fvs)
    -> (
      let app = refcount_app delta gamma ext [ `Method meth ] `Dispatch in
      let args = get_args app in
      let ext = List.hd args in
      let meth = List.hd (List.tl args) in
      match meth with
      | `Method meth ->
          let args, body, methLoc, methFvs, dups = meth in
          let meth = (args, body, methLoc, methFvs) in
          let result =
            `PrototypeCopy
              (ext, ((name, nameLoc), meth, fieldLoc), size, loc, op, fvs)
          in
          AstTypes.IntSet.fold (fun v e -> `Dup (v, e)) dups result
      | _ -> Error.internal_error "refcount: prototype copy")
  | `IntProto (loc, fvs) -> `IntProto (loc, fvs)

and refcount_app delta gamma e1 rest_args body =
  (*
  Δ, Γ2 | Γ − Γ2 ⊢s e1 ⇝ e1'
  Δ | Γ2 ⊢s e2 ⇝ e2'
  Γ2 = Γ ∩ fv(e2)
  __________________________[sapp]
  Δ | Γ ⊢s e1 e2 ⇝ e1' e2'
  *)
  let refcount_e2 delta gamma =
    match rest_args with
    | [] ->
        let e' = refcount delta gamma body in
        e'
    | y :: rest_args -> refcount_app delta gamma y rest_args body
  in
  let fv_e2 =
    List.fold_left
      (fun acc e -> AstTypes.IntSet.union acc (fv e))
      (fv body) rest_args
  in
  let gamma2 = AstTypes.IntSet.inter gamma fv_e2 in
  let e1' =
    refcount
      (AstTypes.IntSet.union delta gamma2)
      (AstTypes.IntSet.diff gamma gamma2)
      e1
  in
  let e2' = refcount_e2 delta gamma2 in
  `App (e1', e2')

let rec fusion expr =
  let rec rc_map expr =
    match expr with
    | `Drop (v, e) ->
        let map, e = rc_map e in
        ( AstTypes.IntMap.update v
            (fun entry ->
              match entry with
              | Some cnt -> Some (cnt - 1)
              | None -> Some (0 - 1))
            map,
          e )
    | `Dup (v, e) ->
        let map, e = rc_map e in
        ( AstTypes.IntMap.update v
            (fun entry ->
              match entry with
              | Some cnt -> Some (cnt + 1)
              | None -> Some (0 + 1))
            map,
          e )
    | e -> (AstTypes.IntMap.empty, fusion e)
  in
  match expr with
  | `PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), size, loc, op) ->
      let args, body, methLoc, methFvs = meth in
      `PrototypeCopy
        ( fusion ext,
          ((name, nameLoc), (args, fusion body, methLoc, methFvs), fieldLoc),
          size,
          loc,
          op )
  | `TaggedObject (tag, payload, loc) -> `TaggedObject (tag, fusion payload, loc)
  | `MethodInvocation (receiver, (name, nameLoc), args, loc) ->
      let args =
        List.map (fun (name, value, loc) -> (name, fusion value, loc)) args
      in
      `MethodInvocation (fusion receiver, (name, nameLoc), args, loc)
  | `PatternMatch (hasDefault, x, cases, loc) ->
      let cases =
        List.map
          (fun (name, (v, body, methLoc), loc) ->
            (name, (v, fusion body, methLoc), loc))
          cases
      in
      `PatternMatch (hasDefault, x, cases, loc)
  | `Declaration (l, r, body, loc) ->
      `Declaration (l, fusion r, fusion body, loc)
  | `VariableLookup _ as v -> v
  | `ExternalCall ((name, nameLoc), args, loc) ->
      let args = List.map fusion args in
      `ExternalCall ((name, nameLoc), args, loc)
  | `EmptyPrototype _ as e -> e
  | `StrProto _ as s -> s
  | `Abort _ as a -> a
  | `IntProto _ as i -> i
  (*
    The one rule is that you cannot move any drops before dups
    However, you are free to rearrange dup and drops amongst themselves
    And you can move dups before drops
  *)
  | (`Drop _ | `Dup _) as d ->
      let map, e = rc_map d in
      let e =
        AstTypes.IntMap.fold
          (fun v cnt e -> if cnt < 0 then `Drop (v, e, -1 * cnt) else e)
          map e
      in
      let e =
        AstTypes.IntMap.fold
          (fun v cnt e -> if cnt > 0 then `Dup (v, e, cnt) else e)
          map e
      in
      e
  | `App _ -> Error.internal_error "Should not have `App here"
  | `Lambda _ -> Error.internal_error "Should not have `Lambda here"
  | `Method _ -> Error.internal_error "Should not have `Method here"
  | `Dispatch -> Error.internal_error "Should not have `Dispatch here"
