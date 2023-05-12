module FvSet = FreeVariables.FvSet

let fv expr =
  match expr with
  | `PrototypeCopy (_, _, _, _, fvs) -> fvs
  | `TaggedObject (_, _, _, fvs) -> fvs
  | `MethodInvocation (_, _, _, _, fvs) -> fvs
  | `PatternMatch (_, _, _, fvs) -> fvs
  | `Declaration (_, _, _, _, fvs) -> fvs
  | `VariableLookup (_, _, fvs) -> fvs
  | `ExternalCall (_, _, _, fvs) -> fvs
  | `IntLiteral _ -> FvSet.empty
  | `FloatLiteral _ -> FvSet.empty
  | `EmptyPrototype _ -> FvSet.empty
  | `StringLiteral _ -> FvSet.empty
  | `Abort _ -> FvSet.empty
  | `Method (_, _, _, fvs) -> fvs
  | `Dispatch -> FvSet.empty


let rec get_args = function
  | `App (arg, rest) -> arg :: (get_args rest)
  | _ -> []
let rec get_body = function
  | `Lambda (_,  body) -> get_body body
  | `Drop (v, body) -> `Drop(v, get_body body)
  | `Dup (v, body) -> `Dup(v, get_body body)
  | body -> body

let rec refcount delta gamma ast =
  assert (FvSet.disjoint delta gamma);
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
    if FvSet.mem x (fv e2) then
      begin
        assert (not (FvSet.mem x delta));
        assert (not (FvSet.mem x gamma));
        let gamma2 = (FvSet.inter gamma (FvSet.remove x (fv e2))) in
        let e1' = refcount (FvSet.union delta gamma2) (FvSet.diff gamma gamma2) e1 in
        let e2' = refcount delta (FvSet.add x gamma2) e2 in
        `Declaration ((x, xLoc), e1', e2', loc, fvs)
      end
    (*
    x̸ \∈ fv(e2), Δ, Γ
    Δ, Γ2 | Γ − Γ2 ⊢s e1 ⇝ e1'
    Δ | Γ2 ⊢s e2 ⇝ e2'
    Γ2 = Γ ∩ fv(e2)
    ____________________________________________________[sbind-drop]
    Δ | Γ ⊢s val x = e1; e2 ⇝ val x = e1'; drop x ; e'
    *)
    else if not (FvSet.mem x (fv e2)) then 
      begin
        assert (not (FvSet.mem x delta));
        assert (not (FvSet.mem x gamma));
        let gamma2 = (FvSet.inter gamma (fv e2)) in
        let e1' = refcount (FvSet.union delta gamma2) (FvSet.diff gamma gamma2) e1 in
        let e2' = refcount delta gamma2 e2 in
        `Declaration ((x, xLoc), e1', (`Drop (x, e2')), loc, fvs)
      end
    else 
      Error.internal_error "refcount: declaration"

  | `TaggedObject ((tag, tagLoc), v, loc, fvs) -> 
    (*
    Δ, Γi+1, ..., Γn | Γi ⊢ s vi ⇝ v'
    i 1 ⩽ i ⩽ n Γi = (Γ − Γi+1 − ... − Γn ) ∩ fv(vi )
    ___________________________________________________[scon]
    Δ | Γ ⊢ s C v1 ... vn ⇝ C v'1 ... v'n   

    n=1
    *)
    let gamma1 = FvSet.inter gamma (fv v) in
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
    if FvSet.mem x gamma then
      `VariableLookup (x, loc, fvs)
    else if FvSet.mem x delta then
      `Dup (x,
            `VariableLookup (x, loc, fvs))
    else
      Error.internal_error "refcount: variable lookup not in delta or gamma"
  | `IntLiteral (i, loc) ->
    `IntLiteral (i, loc)
  | `FloatLiteral (f, loc) -> 
    `FloatLiteral (f, loc)
  | `StringLiteral (s, loc) ->
    `StringLiteral (s, loc)
  | `Abort loc ->
    `Abort loc
  | `EmptyPrototype loc ->
    `EmptyPrototype loc
  | `PatternMatch (x, cases, loc, fvs) ->
    assert (FvSet.mem x gamma);
    let gamma = FvSet.remove x gamma in
    let cases = List.map (
        fun ((name, nameLoc), ((bv_pi, bv_pi_loc), ei, methLoc), loc) ->
          (*
          Δ | Γi ⊢s ei ⇝ e'i
          Γi = (Γ, bv(pi)) ∩ fv(ei ) Γ'
          i = (Γ, bv(pi)) − Γi
          ______________________________________________________________________[smatch]
          Δ | Γ, x ⊢s match x { pi ↦ → ei } ⇝ match x { pi ↦ → drop Γ'i ; e'i }   
          *)
          let gamma_i = FvSet.inter (FvSet.add bv_pi gamma) (fv ei) in
          let gamma_i' = FvSet.diff (FvSet.add bv_pi gamma) gamma_i in
          let ei' = refcount delta gamma_i ei in
          let body = FvSet.fold (fun v e -> `Drop (v, e)) gamma_i' ei' in
          ((name, nameLoc), ((bv_pi, bv_pi_loc), body, methLoc), loc) )
        cases in
    `PatternMatch (x, cases, loc, fvs)
  | `ExternalCall ((name, nameLoc), args, loc, fvs) ->
    (* Convert e to lambda app form *)
    (* Refcount app *)
    let app = refcount_app delta gamma (`Dispatch) args `Dispatch in
    let args = get_args app in
    let args = List.tl args in
    `ExternalCall ((name, nameLoc), args, loc, fvs)

  | `Dispatch -> `Dispatch
  | `MethodInvocation (receiver, (name, nameLoc), args, loc, fvs) ->
    let arg_exprs = List.map (fun (_, e, _) -> e) args in
    let app = refcount_app delta gamma receiver arg_exprs `Dispatch in
    let app_args = get_args app in
    let receiver = List.hd app_args in
    let arg_exprs = List.tl app_args in
    let args = List.map2 (fun (name, _, loc) e -> (name, e, loc)) args arg_exprs in
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
    let arg_set = FvSet.of_list (List.map fst args) in 
    let delta1 = FvSet.diff ys gamma in
    let e' = refcount FvSet.empty (FvSet.union ys arg_set) e in
    assert (FvSet.subset delta1 delta);
    let drops = FvSet.diff arg_set (fv e) in
    let e' = FvSet.fold (fun v e -> `Drop (v, e)) drops e' in
    (`Method (args, e', loc, ys, delta1))

  | `PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), loc, op, fvs) ->
    let app = refcount_app delta gamma ext [`Method meth] `Dispatch in
    let args = get_args app in
    let ext = List.hd args in
    let meth = List.hd (List.tl args) in
    begin
      match meth with
      | `Method meth ->
        let (args, body, methLoc, methFvs, dups) = meth in
        let meth = (args, body, methLoc, methFvs) in
        let result = `PrototypeCopy (ext, ((name, nameLoc), meth, fieldLoc), loc, op, fvs) in
        FvSet.fold (fun v e -> `Dup(v, e)) dups result

      | _ -> Error.internal_error "refcount: prototype copy"
    end



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
    | y :: rest_args ->
      refcount_app delta gamma y rest_args body in
  let fv_e2 = List.fold_left
      (fun acc (e) -> FvSet.union acc (fv e)) (fv body) rest_args in
  let gamma2 = FvSet.inter gamma (fv_e2) in 
  let e1' = refcount (FvSet.union delta gamma2) (FvSet.diff gamma gamma2) e1 in 
  let e2' = refcount_e2 delta gamma2 in 
  `App (e1', e2')