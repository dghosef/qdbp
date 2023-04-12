(* Largely ripped off from 
   https://github.com/tomprimozic/type-systems/blob/master/extensible_rows/infer.ml
   But fully functional style, better error reporting, extension semantics modified to qbdp,
   and recursive types
*)
open Type
exception UnifyError of string

module Env = struct
  module StringMap = Map.Make (String)
  let empty = StringMap.empty
  let extend env name ty = StringMap.add name ty env
  let lookup env name = StringMap.find_opt name env
end

let bool_type level tvars = 
  let tvars, var = make_new_unbound_var tvars level in
  let ty = `TVariant 
      (`TRowExtend
         ("True", (`TRecord `TRowEmpty),
          (`TRowExtend 
             ("False", (`TRecord `TRowEmpty),
              (var)))))
  in
  tvars, ty

let generalize tvars level ty =
  let rec generalize state level ty =
    match ty with
    | `TVar id ->
      let tvars, already_generalized = state in
      begin
        match get_tyvar id tvars with
        | `Unbound (id', level') ->
          if level' > level then 
            let tvars, new_varid = make_new_generic_id tvars id' in
            (tvars, already_generalized), `TVar new_varid
          else
            (tvars, already_generalized), `TVar id
        | `Link ty' -> 
          begin
            match TyVarMap.find_opt id already_generalized with
            | None ->
              let tvars, newvarid = make_new_unbound_id tvars level in 
              let already_generalized =
                TyVarMap.add id (`TVar newvarid) already_generalized 
              in
              let (tvars, already_generalized), generalized_ty' =
                generalize (tvars, already_generalized) level ty'
              in
              let tvars = update_tvars tvars newvarid (`Link generalized_ty') in
              (tvars, already_generalized), `TVar newvarid
            | Some var -> (tvars, already_generalized), var
          end
        | `Generic _ -> (tvars, already_generalized), `TVar id
      end
    | `TArrow (param_tys, ret_ty) ->
      let state, param_tys = List.fold_left_map 
          (fun state param_ty -> generalize state level param_ty)
          state param_tys in
      let state, ret_ty = generalize state level ret_ty in
      state, `TArrow (param_tys, ret_ty)
    | `TRecord row ->
      let state, row = generalize state level row in
      state, `TRecord row
    | `TVariant row ->
      let state, row = generalize state level row in
      state, `TVariant row
    | `TRowExtend (label, ty, row) ->
      let state, ty = generalize state level ty in
      let state, row = generalize state level row in
      state, `TRowExtend (label, ty, row)
    | `TConst c -> state, `TConst c
    | `TRowEmpty -> state, `TRowEmpty
  in
  let ((tvars, _), ty) = generalize (tvars, TyVarMap.empty) level ty in
  tvars, ty

let adjust_levels tvars level ty =
  let rec adjust_levels state level ty =
    match ty with
    | `TConst _ -> state
    | `TRowEmpty -> state
    | `TRowExtend (_, ty, row) ->
      let state = adjust_levels state level ty in
      let state = adjust_levels state level row in
      state
    | `TRecord row ->
      let state = adjust_levels state level row in
      state
    | `TVariant row ->
      let state = adjust_levels state level row in
      state
    | `TArrow (param_tys, ret_ty) ->
      let state = List.fold_left
          (fun state param_ty -> adjust_levels state level param_ty)
          state param_tys in
      adjust_levels state level ret_ty
    | `TVar id ->
      let tvars, already_adjusted = state in
      begin
        match get_tyvar id tvars with
        | `Unbound (id', level') ->
          if level' > level then
            let tvars = update_tvars tvars id (`Unbound (id', level)) in
            (tvars, already_adjusted)
          else
            (tvars, already_adjusted)
        | `Link ty' ->
          begin
            if TyVarSet.mem id already_adjusted then
              (tvars, already_adjusted)
            else
              let already_adjusted = TyVarSet.add id already_adjusted in
              let (tvars, already_adjusted) =
                adjust_levels (tvars, already_adjusted) level ty' in
              (tvars, already_adjusted)
          end

        | `Generic _ ->
          (Error.internal_error "should not be adjusting generic")
      end
  in
  let (tvars, _) = adjust_levels (tvars, TyVarSet.empty) level ty
  in tvars

let splat tvars ty =
  match ty with
  | `TArrow t -> `TArrow t
  | `TRecord t -> `TRecord t
  | `TVariant t -> `TVariant t
  | `TConst t -> `TConst t
  | `TRowEmpty -> `TRowEmpty
  | `TRowExtend (l, t, r) -> `TRowExtend (l, t, r)
  | `TVar id ->
    begin
      match get_tyvar id tvars with
      | `Link ty -> `Link (id, ty)
      | `Unbound v -> `Unbound (id, v)
      | `Generic v -> `Generic (id, v)
    end

let unsplat ty =
  match ty with
  | `TArrow t -> `TArrow t
  | `TRecord t -> `TRecord t
  | `TVariant t -> `TVariant t
  | `TConst t -> `TConst t
  | `TRowEmpty -> `TRowEmpty
  | `TRowExtend (l, t, r) -> `TRowExtend (l, t, r)
  | `Link (id, _) -> `TVar id
  | `Unbound (id, _) -> `TVar id
  | `Generic (id, _) -> `TVar id

let unify tvars already_unified ty1 ty2 =
  let rec unify state ty1 ty2 =
    match state with
    | `Error e -> `Error e
    | `Ok s ->
      begin
        match ty1, ty2 with
        | `TVar id1, `TVar id2
          when TyVarPairSet.mem (id1, id2) (snd s) ||
               TyVarPairSet.mem (id2, id1) (snd s) ->
          `Ok s
        | _ ->
          let s = (
            fst s,
            match ty1, ty2 with
            | `TVar id1, `TVar id2 ->
              TyVarPairSet.add (id1, id2) (snd s)
            | _ -> snd s
          ) in
          if ty1 = ty2 then (`Ok s)
          else
            match splat (fst s) ty1, splat (fst s) ty2 with
            | `TConst ty1 , `TConst ty2 ->
              if ty1 = ty2 then (`Ok s)
              else `Error (`ConstTypeMismatch (ty1, ty2), s)
            | `TArrow (param_tys1, ret_ty1), `TArrow (param_tys2, ret_ty2) ->
              if List.length param_tys1 != List.length param_tys2 then
                `Error (`MethodArity (param_tys1, param_tys2), s)
              else
                begin
                  let state = List.fold_left2
                      (fun state param_ty1 param_ty2 -> unify state param_ty1 param_ty2)
                      (`Ok s) param_tys1 param_tys2 in
                  let state = unify state ret_ty1 ret_ty2 in 
                  state
                end
            | `Link (_, ty1), ty2
            | ty2, `Link (_, ty1) ->
              unify (`Ok s) ty1 (unsplat ty2)
            | `Unbound (_, (ub_id, _)), `Unbound(_, (ub_id', _)) when ub_id = ub_id' ->
              Error.internal_error("unifying identical unbounds")
            | `Unbound (id, (_, ub_level)), ty
            | ty, `Unbound (id, (_, ub_level)) ->
              let tvars = adjust_levels (fst s) ub_level (unsplat ty) in 
              let tvars = update_tvars tvars id (`Link (unsplat ty)) in
              let res = 
                `Ok (tvars, snd s) in 
              res
            | `TRecord row1, `TRecord row2 ->
              unify (`Ok s) row1 row2
            | `TVariant row1, `TVariant row2 ->
              unify (`Ok s) row1 row2
            | `TRowEmpty, `TRowEmpty ->
              (`Ok s)
            | `TRowExtend(label1, field_ty1, rest_row1), (`TRowExtend _ as row2) ->
              let result = begin
                match rewrite_row (`Ok s) row2 label1 field_ty1 with
                |`Error e -> `Error e
                | `Ok (state, rest_row2) ->
                  unify state rest_row1 rest_row2
              end in
              result
            | _ -> `Error (`CannotUnify (ty1, ty2), s)
      end

  and rewrite_row state row2 label1 field_ty1 = 
    match state with 
    | `Error e -> `Error e
    | `Ok s -> 
      match row2 with
      | `TRowEmpty -> 
        `Error ((`MissingLabel label1), s)
      | `TRowExtend (label2, field_ty2, rest_row2) when label2 = label1 ->
        let state = unify (`Ok s) field_ty1 field_ty2 in
        `Ok (state, rest_row2)
      | `TRowExtend(label2, field_ty2, rest_row2) ->
        begin
          match rewrite_row (`Ok s) rest_row2 label1 field_ty1 with
          | `Error e -> `Error e
          | `Ok(state, rest_row2) ->
            `Ok (state, `TRowExtend(label2, field_ty2, rest_row2))
        end
      | `TVar id ->
        let (tvars, already_unified) = s in
        begin
          match get_tyvar id tvars with
          | `Unbound(_, level) ->
            let tvars, rest_row2_id = make_new_unbound_id tvars level in 
            let rest_row2 = `TVar rest_row2_id in
            let ty2 = `TRowExtend(label1, field_ty1, rest_row2) in
            let tvars = update_tvars tvars id (`Link ty2) in 
            `Ok ((`Ok (tvars, already_unified)), rest_row2)
          | `Link row2 ->
            rewrite_row (`Ok (tvars, already_unified)) row2 label1 field_ty1
          | `Generic _ -> 
            `Error ((`RewriteRowWithGeneric), s)
        end
      | _ -> 
        `Error ((`RewriteRowOnNonRow row2), s)
  in
  unify (`Ok (tvars, already_unified)) ty1 ty2

let instantiate tvars level ty =
  let rec instantiate state ty =
    match ty with
    | `TConst _ -> state, ty
    | `TRowEmpty -> state, ty
    | `TRowExtend (label, ty, row) ->
      let state, ty = instantiate state ty in
      let state, row = instantiate state row in
      state, `TRowExtend (label, ty, row)
    | `TRecord row ->
      let state, row = instantiate state row in
      state, `TRecord row
    | `TVariant row ->
      let state, row = instantiate state row in
      state, `TVariant row
    | `TArrow (param_tys, ret_ty) ->
      let state, param_tys = List.fold_left_map
          (fun state param_ty -> instantiate state param_ty)
          state param_tys in
      let state, ret_ty = instantiate state ret_ty in
      state, `TArrow (param_tys, ret_ty)
    | `TVar id ->
      let tvars, already_instantiated, id_var_map = state in
      begin
        match get_tyvar id tvars with
        | `Unbound _ ->
          state, ty
        | `Link ty' ->
          begin
            match TyVarMap.find_opt id already_instantiated with
            | None ->
              let tvars, var_id = make_new_unbound_id tvars level in
              let already_instantiated =
                TyVarMap.add id (`TVar var_id) already_instantiated in
              let (tvars, already_instantiated, id_var_map), var =
                instantiate (tvars, already_instantiated, id_var_map) ty' in
              let tvars = update_tvars tvars var_id (`Link var) in
              (tvars, already_instantiated, id_var_map), `TVar var_id
            | Some var -> (tvars, already_instantiated, id_var_map), var
          end
        | `Generic id ->
          match TyVarMap.find_opt id id_var_map with
          | None ->
            let tvars, var_id = make_new_unbound_id tvars level in
            let id_var_map = TyVarMap.add id (`TVar var_id) id_var_map in
            (tvars, already_instantiated, id_var_map), `TVar var_id
          | Some var -> (tvars, already_instantiated, id_var_map), var

      end
  in
  let (tvars, _, _), ty = instantiate (tvars, TyVarMap.empty, TyVarMap.empty) ty
  in tvars, ty

let rec row_is_complete tvars row =
  match row with 
  | `TRowEmpty -> true
  | `TRowExtend(_, _, rest_row) -> row_is_complete tvars rest_row
  | `TVar id ->
    begin
      match get_tyvar id tvars with
      | `Link ty ->
        row_is_complete tvars ty
      | `Unbound _ -> false
      | `Generic _ -> false
    end
  | `TRecord row -> row_is_complete tvars row
  | `TVariant row -> row_is_complete tvars row
  | _ -> false

let rec get_row tvars row label =
  match row with 
  | `TRowExtend(l, field_ty, rest_row) ->
    if l = label then Some field_ty
    else get_row tvars rest_row label
  | `TVar id ->
    begin
      match get_tyvar id tvars with
      | `Link ty -> get_row tvars ty label
      | _ -> None
    end
  | `TRecord row -> get_row tvars row label
  | `TVariant row -> get_row tvars row label
  | _ -> None

let unify_error_msg (err, (tvars, _)) = 
  match err with
  | `CannotUnify (ty1, ty2) ->
    "tried to unify a "
    ^ kind_of tvars ty1
    ^ " with a "
    ^ kind_of tvars ty2
  | `ConstTypeMismatch (ty1, ty2) ->
    "tried to unify a "
    ^ str_of_const_ty ty1
    ^ "with a"
    ^ str_of_const_ty ty2
  | `MethodArity (param_list1, param_list2) ->
    "expected " ^ (string_of_int (List.length param_list1))
    ^ "params but got " 
    ^ (string_of_int (List.length param_list2))
  | `MissingLabel label ->
    "expected expression to have '" ^ label ^ "' field"
  | `RewriteRowWithGeneric ->
    "INTERNAL ERROR: Tried calling rewrite_row on generic"
  | `RewriteRowOnNonRow row ->
    "INTERNAL ERROR: Tried calling rewrite_row on " ^ (kind_of tvars row)
  | `CantUnifyGeneric _ -> 
    "INTERNAL ERROR: All generics should be instantiated in unify"
  | `UnifyingSameVariable -> 
    "INTERNAL ERROR: Should not be unifying the same variable"

let loc expr =
  match expr with
  | `EmptyPrototype loc -> loc
  | `PrototypeExtension (_, _, loc) -> loc
  | `TaggedObject (_, _, loc) -> loc
  | `MethodInvocation (_, _, _, loc) -> loc
  | `PatternMatch (_, _, loc) -> loc
  | `Declaration (_, _, _, loc) -> loc
  | `VariableLookup (_, loc) -> loc
  | `Import (_, loc) -> loc
  | `ExternalCall (_, _, loc) -> loc
  | `IntLiteral (_, loc) -> loc
  | `FloatLiteral (_, loc) -> loc
  | `StringLiteral (_, loc) -> loc
  | `Abort loc -> loc
  | `Method (_, _, loc) -> loc

let infer imports files expr =

  let infer_error msg loc =
    Error.compile_error msg loc files in

  let rec match_fn_ty tvars loc num_params ty =
    match ty with 
    | `TArrow (param_tys, ret_ty) ->
      if List.length param_tys = num_params then
        tvars, param_tys, ret_ty
      else
        infer_error ("expected " ^ (string_of_int num_params)
                     ^ " arguments, but instead got " ^
                     (string_of_int (List.length param_tys))) loc
    | `TVar id ->
      begin
        match get_tyvar id tvars with
        | `Unbound (_, level) ->  
          let tvars, param_ty_list = List.fold_left_map
              (fun tvars _ -> make_new_unbound_var tvars level)
              tvars (List.init num_params (fun _ -> ())) in
          let tvars, ret_ty = make_new_unbound_var tvars level in
          let tvars = update_tvars tvars id (`Link (`TArrow (param_ty_list, ret_ty))) in
          tvars, param_ty_list, ret_ty
        | `Generic _ -> 
          infer_error
            ("expected a method but instead got a generic type variable")
            loc
        | `Link ty -> match_fn_ty tvars loc num_params ty
      end
    | ty -> 
      infer_error ("expected a method but instead got a " ^ (kind_of tvars ty)) loc
  in
  let try_unify tvars already_unified locs ty1 ty2 =
    match (unify tvars already_unified ty1 ty2) with
    | `Ok state -> state
    | `Error e -> Error.compile_error (unify_error_msg e) locs files
  in
  let rec infer_method state env level meth =
    let (tvars, already_unififed) = state in
    let (args, body, _) = meth in
    let tvars, param_ty_list = List.fold_left_map
        (fun tvars _ -> make_new_unbound_var tvars level) tvars args in
    let fn_env = List.fold_left2
        (fun env param_name param_ty -> Env.extend env (fst param_name) param_ty)
        env args param_ty_list in
    let state, return_ty = infer (tvars, already_unififed) fn_env level body in
    state, `TArrow (param_ty_list, return_ty)

  and infer_record_select state env level expr =
    let (receiver, (name, _), _, _) = expr in
    let (tvars, already_unified) = state in
    let (tvars, rest_row_ty) = make_new_unbound_var tvars level in
    let (tvars, field_ty) = make_new_unbound_var tvars level in
    let receiver_ty = `TRecord (`TRowExtend(name, field_ty, rest_row_ty)) in
    let (tvars, already_unified), receiver_ty' =
      infer (tvars, already_unified) env level receiver in
    let (tvars, already_unified) =
      try_unify tvars already_unified (loc receiver) receiver_ty receiver_ty' in
    (tvars, already_unified), field_ty

  and infer state env level expr =
    match expr with
    | `EmptyPrototype _ -> state, `TRecord `TRowEmpty
    | `Abort _ -> 
      let tvars, already_unified = state in 
      let tvars, tvar = make_new_unbound_var tvars level in
      (tvars, already_unified), tvar
    | `IntLiteral _ -> state, (`TConst `Int)
    | `FloatLiteral _ -> state, (`TConst `Float)
    | `StringLiteral _ -> state, (`TConst `String)
    | `VariableLookup (name, loc) ->
      let tvars, already_unified = state in
      begin
        match Env.lookup env name with
        | Some ty ->
          let tvars, ty = instantiate tvars level ty in
          (tvars, already_unified), ty
        | None ->
          infer_error
            ("variable " ^ name ^ " was not previously defined")
            loc
      end
    | `Declaration ((name, _), rhs, post_decl_expr, _) ->
      let state, var_ty = infer state env (level + 1) rhs in
      let tvars, already_unified = state in
      let tvars, generalized_ty = generalize tvars level var_ty in
      let env' = Env.extend env name generalized_ty in
      let state', post_decl_ty = 
        infer (tvars, already_unified) env' level post_decl_expr in
      state', post_decl_ty
    | `ExternalCall ((name, name_loc), args, _) ->
      (* FIXME: Add error for name that has no _ or starts with `qdbp` *)
      let state = List.fold_left
          (fun state arg -> fst (infer state env level arg)) state args in
      let ty_str = String.sub 
          name
          (String.rindex name '_')
          (String.length name - String.rindex name '_')
      in
      let tvars, already_unified = state in
      let tvars, ty = match ty_str with
        | "_int" -> tvars, `TConst `Int
        | "_float" -> tvars, `TConst `Float
        | "_string" -> tvars, `TConst `String
        | "_bool" -> bool_type level tvars
        | _ -> 
          infer_error
            ("external call should end with '_int', '_float', '_string', or '_bool'"
             ^ " to signify its return type")
            name_loc
      in
      (tvars, already_unified), ty
    | `PrototypeExtension (extension, ((name, _), meth, _), _) ->
      let tvars, already_unified = state in
      let tvars, rest_row_ty = make_new_unbound_var tvars level in
      let tvars, field_ty = make_new_unbound_var tvars level in
      let extension_ty = `TRecord rest_row_ty in
      let state, extension_ty' = infer (tvars, already_unified) env level extension in
      let state, meth_ty = infer_method state env level meth in
      let tvars, already_unified = state in
      let tvars, already_unified =
        try_unify tvars already_unified (loc (`Method meth)) field_ty meth_ty in
      let tvars, already_unified =
        try_unify tvars already_unified (loc extension) extension_ty extension_ty' in
      if row_is_complete tvars rest_row_ty && get_row tvars rest_row_ty name = None then
        (tvars, already_unified), `TRecord (`TRowExtend(name, field_ty, rest_row_ty))
      else
        let tvars, field_ty' = make_new_unbound_var tvars level in 
        let tvars, rest_row_ty' = make_new_unbound_var tvars level in
        let (tvars, already_unified) =
          try_unify tvars already_unified (loc extension) extension_ty'
            (`TRecord (`TRowExtend(name, field_ty', rest_row_ty'))) in
        let (tvars, already_unified) =
          try_unify tvars already_unified (loc (`Method meth)) field_ty field_ty' in
        (tvars, already_unified), extension_ty

    | `TaggedObject ((tag, _), value, _) ->
      let tvars, already_unified = state in
      let tvars, rest_row_ty = make_new_unbound_var tvars level in
      let tvars, value_ty = make_new_unbound_var tvars level in
      let (tvars, already_unified), value_ty' =
        infer (tvars, already_unified) env level value in
      let tvars, already_unified =
        try_unify tvars already_unified (loc value) value_ty value_ty' in
      (tvars, already_unified), `TVariant (`TRowExtend(tag, value_ty, rest_row_ty))

    | `MethodInvocation invok ->
      let state, fn_ty = infer_record_select state env level invok in
      let (_, _, args, invok_loc) = invok in 
      let (tvars, already_unified) = state in 
      let tvars, param_tys, return_ty = 
        match_fn_ty tvars invok_loc (List.length args) fn_ty in 
      let state = List.fold_left2
          (fun state arg_ty arg -> 
             let (tvars, already_unified) = state in
             let (_, arg, _) = arg in
             let state, arg_ty' = infer (tvars, already_unified) env level arg in
             let (tvars, already_unified) = state in
             let (tvars, already_unified) =
               try_unify tvars already_unified (loc arg) arg_ty arg_ty' in
             (tvars, already_unified))
          (tvars, already_unified) param_tys args in
      let (tvars, already_unified) = state in
      (tvars, already_unified), return_ty
    | `PatternMatch (expr, cases, _) ->
      let (tvars, already_unified) = state in
      let tvars, return_ty = make_new_unbound_var tvars level in
      let (tvars, already_unified), expr_ty = infer (tvars, already_unified) env level expr in
      let tvars, cases_row =
        infer_cases tvars already_unified env level return_ty `TRowEmpty cases in
      let tvars, already_unified =
        try_unify tvars already_unified (loc expr) expr_ty (`TVariant cases_row) in
      (tvars, already_unified), return_ty
    | `Import (filename, _) ->
      let expr = ResolveImports.ImportMap.find filename imports in
      infer state Env.empty level expr

  and infer_cases tvars already_unified env level return_ty rest_row_ty cases =
    match cases with
    | [] -> tvars, rest_row_ty
    | ((label, _), ((var_name, _), expr, _), _) :: other_cases ->
      let tvars, variant_ty = make_new_unbound_var tvars level in
      let (tvars, already_unified), expr_ty =
        infer (tvars, already_unified) (Env.extend env var_name variant_ty) level expr
      in
      let tvars, already_unified =
        try_unify tvars already_unified (loc expr) return_ty expr_ty in
      let tvars, other_cases_row =
        infer_cases tvars already_unified env level return_ty rest_row_ty other_cases in
      let row_ty =`TRowExtend(label, variant_ty, other_cases_row) in
      tvars, row_ty
  in
  let (tvars, _), return_ty =
    infer ((0, TyVarMap.empty), TyVarPairSet.empty) Env.empty 0 expr
  in tvars, return_ty