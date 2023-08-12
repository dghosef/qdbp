let get_tyvar id tyvars =
  let _, varmap = tyvars in
  match AstTypes.IntMap.find_opt id varmap with
  | Some v -> v
  | None ->
      Error.abort
        (Error.internal_error ("Failed to find variable " ^ string_of_int id))

let make_new_unbound_id tvars level =
  let var_id, varmap = tvars in
  let unbound_id = var_id + 1 in
  let new_id = unbound_id + 1 in
  let tvars' =
    (new_id, AstTypes.IntMap.add var_id (`Unbound (unbound_id, level)) varmap)
  in
  (tvars', var_id)

let make_new_generic_id tvars id =
  let var_id, varmap = tvars in
  let new_id = var_id + 1 in
  let tvars' = (new_id, AstTypes.IntMap.add var_id (`Generic id) varmap) in
  (tvars', var_id)

let make_new_unbound_var tvars level =
  let tvars, var_id = make_new_unbound_id tvars level in
  (tvars, `TVar var_id)

let update_tvars tvars var_id new_var =
  (fst tvars, AstTypes.IntMap.add var_id new_var (snd tvars))

let bool_ty level tvars =
  let tvars, var = make_new_unbound_var tvars level in
  let ty =
    `TVariant
      (`TRowExtend
        ( "True",
          `TRecord `TRowEmpty,
          `TRowExtend ("False", `TRecord `TRowEmpty, var) ))
  in
  (tvars, ty)

let strconst_ty =
  `TVariant (`TRowExtend ("isString", `TRecord `TRowEmpty, `TRowEmpty))

let str_proto_type tvars level =
  let tvars, v1 = make_new_unbound_var tvars level in
  (* MUST Keep in sync with int_proto.h and namesToInts.ml *)
  let row =
    `TRowExtend
      ("Print:SelfArg", `TArrow ([ v1 ], `TRecord `TRowEmpty), `TRowEmpty)
  in
  let tvars, v1 = make_new_unbound_var tvars level in
  let row =
    `TRowExtend ("isAStr:SelfArg", `TArrow ([ v1 ], `TRecord `TRowEmpty), row)
  in
  (tvars, `TRecord row)

let channel_proto_type tvars level =
  let tvars, v1 = make_new_unbound_var tvars (level) in
  let tvars, v2 = make_new_unbound_var tvars (-1) in
  let tvars, v3 = make_new_unbound_var tvars (level) in
  (* MUST Keep in sync with int_proto.h and namesToInts.ml *)
  let row =
    `TRowExtend
      ("Receive:SelfArg", `TArrow ([ v1 ], v2), `TRowEmpty) in
  let row =
    `TRowExtend ("Send:SelfArg:Arg0", `TArrow ([ v3; v2 ], `TRecord `TRowEmpty), row)
  in
  (tvars, `TRecord row)

let int_proto_type tvars level =
  let add_arith_binop tvars level binop row =
    let tvars, v1 = make_new_unbound_var tvars level in
    let tvars, v2 = make_new_unbound_var tvars level in
    let tvars, v3 = make_new_unbound_var tvars level in
    let arg0_ty =
      `TRecord
        (`TRowExtend
          ("isAnInt:SelfArg", `TArrow ([ v2 ], `TRecord `TRowEmpty), v3))
    in
    ( tvars,
      `TRowExtend
        (binop ^ ":SelfArg" ^ ":Arg0", `TArrow ([ v1; arg0_ty ], v1), row) )
  in
  let add_cmp_binop tvars level binop row =
    let tvars, v1 = make_new_unbound_var tvars level in
    let tvars, v2 = make_new_unbound_var tvars level in
    let tvars, v3 = make_new_unbound_var tvars level in
    let arg0_ty =
      `TRecord
        (`TRowExtend
          ("isAnInt:SelfArg", `TArrow ([ v2 ], `TRecord `TRowEmpty), v3))
    in
    let tvars, ret_ty = bool_ty level tvars in
    ( tvars,
      `TRowExtend
        (binop ^ ":SelfArg" ^ ":Arg0", `TArrow ([ v1; arg0_ty ], ret_ty), row)
    )
  in
  let tvars, v1 = make_new_unbound_var tvars level in
  (* MUST Keep in sync with int_proto.h and namesToInts.ml *)
  let row =
    `TRowExtend
      ("Print:SelfArg", `TArrow ([ v1 ], `TRecord `TRowEmpty), `TRowEmpty)
  in
  let tvars, v1 = make_new_unbound_var tvars level in
  let row =
    `TRowExtend ("isAnInt:SelfArg", `TArrow ([ v1 ], `TRecord `TRowEmpty), row)
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
  (tvars, `TRecord row)

let paren s = "(" ^ s ^ ")"
let bracket s = "{" ^ s ^ "}"
let brace s = "[" ^ s ^ "]"
let angle s = "<" ^ s ^ ">"
let pipe s = "|" ^ s ^ "|"

let annotate_rec_tvars tvars ty =
  let rec annotate_rec_tvars seen ty =
    match ty with
    | `TArrow (params, ret) ->
        let rec_tvars, param_tys =
          List.fold_left_map
            (fun rec_tvars param_ty ->
              let rec_tvars', param_ty = annotate_rec_tvars seen param_ty in
              (AstTypes.IntSet.union rec_tvars rec_tvars', param_ty))
            AstTypes.IntSet.empty params
        in
        let rec_tvars', ret_ty = annotate_rec_tvars seen ret in
        (AstTypes.IntSet.union rec_tvars' rec_tvars, `TArrow (param_tys, ret_ty))
    | `TRecord row ->
        let rec_tvars, row = annotate_rec_tvars seen row in
        (rec_tvars, `TRecord row)
    | `TVariant row ->
        let rec_tvars, row = annotate_rec_tvars seen row in
        (rec_tvars, `TVariant row)
    | `TRowEmpty -> (AstTypes.IntSet.empty, `TRowEmpty)
    | `TRowExtend (label, field_ty, extension) ->
        let rec_tvars, field_ty = annotate_rec_tvars seen field_ty in
        let rec_tvars', extension_ty = annotate_rec_tvars seen extension in
        ( AstTypes.IntSet.union rec_tvars rec_tvars',
          `TRowExtend (label, field_ty, extension_ty) )
    | `TVar id -> (
        match get_tyvar id tvars with
        | `Link ty ->
            if AstTypes.IntSet.mem id seen then
              ( AstTypes.IntSet.add id AstTypes.IntSet.empty,
                `FixpointReference id )
            else
              let rec_tvars, ty =
                annotate_rec_tvars (AstTypes.IntSet.add id seen) ty
              in
              if AstTypes.IntSet.mem id rec_tvars then
                (AstTypes.IntSet.remove id rec_tvars, `Fixpoint (id, ty))
              else (rec_tvars, ty)
        | `Unbound v -> (AstTypes.IntSet.empty, `Unbound v)
        | `Generic v -> (AstTypes.IntSet.empty, `Generic v))
  in
  annotate_rec_tvars AstTypes.IntSet.empty ty

let newline indent = "\n" ^ String.make (indent * 2) ' '

let str_of_ty tvars ty =
  let rec str_of_ty indent fixpoint_ids unbound_ids ty =
    match ty with
    | `TArrow (params, ret) ->
        let unbound_ids, param_ty_strs =
          List.fold_left_map
            (str_of_ty (indent + 1) fixpoint_ids)
            unbound_ids params
        in
        let params_ty_str = String.concat ", " param_ty_strs in
        let unbound_ids, ret_str =
          str_of_ty (indent + 1) fixpoint_ids unbound_ids ret
        in
        (unbound_ids, paren (params_ty_str ^ " -> " ^ ret_str))
    | `TRecord row ->
        let unbound_ids, row_ty_str =
          str_of_ty (indent + 1) fixpoint_ids unbound_ids row
        in
        ( unbound_ids,
          bracket (newline (indent + 1) ^ row_ty_str ^ newline indent) )
    | `TVariant row ->
        let unbound_ids, row_ty_str =
          str_of_ty (indent + 1) fixpoint_ids unbound_ids row
        in
        (unbound_ids, brace (newline (indent + 1) ^ row_ty_str ^ newline indent))
    | `TRowEmpty -> (unbound_ids, "<>")
    | `TRowExtend (label, field_ty, extension_ty) ->
        let unbound_ids, field_ty_str =
          str_of_ty (indent + 1) fixpoint_ids unbound_ids field_ty
        in
        let unbound_ids, extension_ty_str =
          str_of_ty indent fixpoint_ids unbound_ids extension_ty
        in
        ( unbound_ids,
          angle
            (label ^ " : " ^ field_ty_str ^ newline indent ^ extension_ty_str)
        )
    | `FixpointReference id ->
        (unbound_ids, "'" ^ string_of_int (AstTypes.IntMap.find id fixpoint_ids))
    | `Fixpoint (id, ty) ->
        let unbound_ids, ty_str =
          str_of_ty indent
            (AstTypes.IntMap.add id
               (AstTypes.IntMap.cardinal fixpoint_ids
               + AstTypes.IntMap.cardinal unbound_ids)
               fixpoint_ids)
            unbound_ids ty
        in
        ( unbound_ids,
          "'"
          ^ string_of_int (AstTypes.IntMap.cardinal fixpoint_ids)
          ^ "." ^ ty_str )
    | `Unbound (id, _) -> (
        match AstTypes.IntMap.find_opt id unbound_ids with
        | Some id -> (unbound_ids, "'" ^ string_of_int id)
        | None ->
            let next_id =
              AstTypes.IntMap.cardinal fixpoint_ids
              + AstTypes.IntMap.cardinal unbound_ids
            in
            ( AstTypes.IntMap.add id next_id unbound_ids,
              "'" ^ string_of_int next_id ))
    | `Generic id -> (unbound_ids, "forall" ^ paren (string_of_int id))
  in
  let rec_types, ty = annotate_rec_tvars tvars ty in
  if AstTypes.IntSet.is_empty rec_types then
    snd (str_of_ty 0 AstTypes.IntMap.empty AstTypes.IntMap.empty ty)
  else
    Error.internal_error "Didn't resolve all the recursive types in `str_of_ty`"

let rec kind_of tvars ty =
  match ty with
  | `TArrow _ -> "Method"
  | `TRecord _ -> "Nonempty Prototype"
  | `TVariant _ -> "Tagged Object"
  | `TRowEmpty -> "Empty Row"
  | `TRowExtend _ -> "Row Extend"
  | `TVar id -> (
      match get_tyvar id tvars with
      | `Unbound (_, _) -> "Unbound " ^ string_of_int id
      | `Link ty -> "Link " ^ kind_of tvars ty
      | `Generic _ -> "Generic " ^ string_of_int id)
