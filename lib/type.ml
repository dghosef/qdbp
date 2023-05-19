module TyVarMap = Map.Make(struct type t = int let compare = compare end)
module TyVarSet = Set.Make(struct type t = int let compare = compare end)
module TyVarPairSet = Set.Make(struct type t = (int * int) let compare = compare end)
let get_tyvar id tyvars =
  let (_, varmap) = tyvars in
  match TyVarMap.find_opt id varmap with
  | Some v -> v
  | None ->
    Error.abort (Error.internal_error ("Failed to find variable " ^ (string_of_int id)))
let make_new_unbound_id tvars level =
  let (var_id, varmap) = tvars in
  let unbound_id = var_id + 1 in
  let new_id = unbound_id + 1 in 
  let tvars' = (new_id, (TyVarMap.add var_id (`Unbound (unbound_id, level)) varmap)) in
  tvars', var_id

let make_new_generic_id tvars id =
  let (var_id, varmap) = tvars in
  let new_id = var_id + 1 in 
  let tvars' = (new_id, (TyVarMap.add var_id (`Generic id) varmap)) in
  tvars', var_id

let make_new_unbound_var tvars level =
  let tvars, var_id = make_new_unbound_id tvars level in
  tvars, `TVar var_id

let update_tvars tvars var_id new_var =
  (fst tvars, TyVarMap.add var_id new_var (snd tvars))

let bool_ty =
  `TVariant (
    `TRowExtend (
      "True", `TRecord (`TRowEmpty),
      `TRowExtend (
        "False", `TRecord (`TRowEmpty),
        `TRowEmpty
      )
    )
  )
let int_proto_type tvars level =
  let add_arith_binop tvars level binop row =
    let tvars, v1 = make_new_unbound_var tvars level in
    let tvars, v2 = make_new_unbound_var tvars level in
    let tvars, v3 = make_new_unbound_var tvars level in
    let that_ty = `TRecord (
      `TRowExtend (
        "Val:this", `TArrow ([v2], `TConst `Int),
        v3
      )
    ) in
    tvars,
    `TRowExtend (
      binop ^ ":this" ^ ":that", 
      `TArrow ([ v1;  that_ty], v1)
      , row
    ) in
  let add_cmp_binop tvars level binop row =
    let tvars, v1 = make_new_unbound_var tvars level in
    let tvars, v2 = make_new_unbound_var tvars level in
    let tvars, v3 = make_new_unbound_var tvars level in
    let that_ty = `TRecord (
      `TRowExtend (
        "Val:this", `TArrow ([v2], `TConst `Int),
        v3
      )
    ) in
    tvars,
    `TRowExtend (
      binop ^ ":this" ^ ":that", 
      `TArrow ([ v1;  that_ty], bool_ty)
      , row
    ) in
  let tvars, v1 = make_new_unbound_var tvars level in
  (* MUST Keep in sync with int_proto.h and namesToInts.ml *)
  let row = 
    `TRowExtend (
      "Print:this", `TArrow ([v1],
                             `TRecord (`TRowEmpty)),
      `TRowEmpty)
  in
  let tvars, v1 = make_new_unbound_var tvars level in
  let row = 
    `TRowExtend (
      "Val:this", `TArrow ([v1],
                           (`TConst `Int)),
      row)
  in
  let tvars, row = add_arith_binop tvars level "+" row in
  let tvars, row = add_arith_binop tvars level "-" row in
  let tvars, row = add_arith_binop tvars level "*" row in
  let tvars, row = add_arith_binop tvars level "/" row in
  let tvars, row = add_arith_binop tvars level "%" row in
  let tvars, row = add_cmp_binop tvars level "=" row in
  let tvars, row = add_cmp_binop tvars level "!=" row in
  let tvars, row = add_cmp_binop tvars level "<" row in
  let tvars, row = add_cmp_binop tvars level ">" row in
  let tvars, row = add_cmp_binop tvars level "<=" row in
  let tvars, row = add_cmp_binop tvars level ">=" row in
  tvars, `TRecord row

let paren s = "(" ^ s ^ ")"
let bracket s = "{" ^ s ^ "}"
let brace s = "[" ^ s ^ "]"
let angle s = "<" ^ s ^ ">"
let pipe s = "|" ^ s ^ "|"
let annotate_rec_tvars tvars ty =
  let rec annotate_rec_tvars seen ty =
    match ty with
    | `TArrow (params, ret) ->
      let rec_tvars, param_tys = List.fold_left_map
          (fun rec_tvars param_ty ->
             let rec_tvars', param_ty = annotate_rec_tvars seen param_ty in
             (TyVarSet.union rec_tvars rec_tvars'), param_ty)
          TyVarSet.empty params
      in
      let rec_tvars', ret_ty = annotate_rec_tvars seen ret in 
      (TyVarSet.union rec_tvars' rec_tvars), `TArrow (param_tys, ret_ty)
    | `TRecord row ->
      let rec_tvars, row = annotate_rec_tvars seen row in
      rec_tvars, `TRecord row
    | `TVariant row ->
      let rec_tvars, row = annotate_rec_tvars seen row in
      rec_tvars, `TVariant row
    | `TRowEmpty ->
      TyVarSet.empty, `TRowEmpty
    | `TConst c ->
      TyVarSet.empty, `TConst c
    | `TRowExtend (label, field_ty, extension) ->
      let rec_tvars, field_ty = annotate_rec_tvars seen field_ty in
      let rec_tvars', extension_ty = annotate_rec_tvars seen extension in
      (TyVarSet.union rec_tvars rec_tvars'), 
      `TRowExtend (label, field_ty, extension_ty)
    | `TVar id ->
      begin
        match get_tyvar id tvars with
        | `Link ty ->
          if TyVarSet.mem id seen then
            (TyVarSet.add id (TyVarSet.empty)), `FixpointReference id
          else
            let rec_tvars, ty =
              annotate_rec_tvars (TyVarSet.add id seen) ty
            in
            if TyVarSet.mem id rec_tvars then
              (TyVarSet.remove id rec_tvars), `Fixpoint (id, ty)
            else
              rec_tvars, ty
        | `Unbound v ->
          TyVarSet.empty, `Unbound v
        | `Generic v ->
          TyVarSet.empty, `Generic v
      end
  in
  annotate_rec_tvars (TyVarSet.empty) ty

let newline indent =
  "\n" ^ (String.make (indent * 2) ' ')

let str_of_const_ty ty =
  match ty with
  | `Int -> "Integer Literal"
  | `Float -> "Float Literal"
  | `String -> "String Literal"

let str_of_ty tvars ty =
  let rec str_of_ty indent fixpoint_ids unbound_ids ty =
    match ty with
    | `TArrow (params, ret) ->
      let unbound_ids, param_ty_strs = List.fold_left_map
          (str_of_ty (indent + 1) fixpoint_ids) unbound_ids params in
      let params_ty_str = String.concat ", " param_ty_strs in
      let unbound_ids, ret_str =
        str_of_ty (indent + 1) fixpoint_ids unbound_ids ret in
      unbound_ids,
      paren (
        params_ty_str ^ " -> " ^ ret_str)
    | `TRecord row ->
      let unbound_ids, row_ty_str =
        str_of_ty (indent + 1) fixpoint_ids unbound_ids row in
      unbound_ids, bracket (
        newline (indent + 1) ^
        (row_ty_str) ^
        newline indent
      )
    | `TVariant row ->
      let unbound_ids, row_ty_str =
        str_of_ty (indent + 1) fixpoint_ids unbound_ids row in
      unbound_ids, brace (
        newline (indent + 1) ^
        (row_ty_str) ^
        newline indent
      )
    | `TRowEmpty -> unbound_ids, "<>"
    | `TConst c -> unbound_ids, str_of_const_ty c
    | `TRowExtend (label, field_ty, extension_ty) ->
      let unbound_ids, field_ty_str =
        str_of_ty (indent + 1) fixpoint_ids unbound_ids field_ty in 
      let unbound_ids, extension_ty_str =
        str_of_ty (indent) fixpoint_ids unbound_ids extension_ty in 
      unbound_ids, angle (label ^ " : " ^ field_ty_str ^ newline indent
                          ^ extension_ty_str)
    | `FixpointReference id ->
      unbound_ids, "'" ^ (string_of_int (TyVarMap.find id fixpoint_ids))
    | `Fixpoint (id, ty) ->
      let unbound_ids, ty_str = str_of_ty
          indent
          (TyVarMap.add id
             (TyVarMap.cardinal fixpoint_ids + TyVarMap.cardinal unbound_ids)
             fixpoint_ids)
          unbound_ids ty
      in
      unbound_ids,
      "'" ^ string_of_int ((TyVarMap.cardinal fixpoint_ids)) ^ "." ^ ty_str
    | `Unbound (id, _) ->
      begin
        match TyVarMap.find_opt id unbound_ids with 
        | Some id -> unbound_ids, "'" ^ (string_of_int id)
        | None -> 
          let next_id = (TyVarMap.cardinal fixpoint_ids + TyVarMap.cardinal unbound_ids) in
          (TyVarMap.add id next_id unbound_ids), "'" ^ (string_of_int next_id)
      end
    | `Generic id -> unbound_ids, "forall" ^ (paren (string_of_int id))
  in
  let rec_types, ty = annotate_rec_tvars tvars ty in
  if (TyVarSet.is_empty rec_types) then
    snd (str_of_ty 0 TyVarMap.empty TyVarMap.empty ty)
  else
    Error.internal_error "Didn't resolve all the recursive types in `str_of_ty`"


let rec kind_of tvars ty =
  match ty with
  | `TArrow _ -> "Method"
  | `TRecord _ -> "Nonempty Prototype"
  | `TVariant _ -> "Tagged Object"
  | `TRowEmpty -> "Empty Row"
  | `TConst c -> str_of_const_ty c
  | `TRowExtend _ -> "Row Extend"
  | `TVar id ->
    begin
      match get_tyvar id tvars with
      | `Unbound (_, _) ->
        "Unbound " ^ (string_of_int id)
      | `Link ty -> "Link " ^ kind_of tvars ty
      | `Generic _ -> "Generic " ^ (string_of_int id)
    end