(* Largely ripped off from https://github.com/tomprimozic/type-systems/blob/master/extensible_rows/infer.ml *)
(* But with fixpoint types and limited extension *)
(* FIXME: Better error propogation *)
(* URGENT: Include TVariant and TRecord *)
open Type
open Ast
let current_id = ref 0

let next_id () =
  let id = !current_id in
  current_id := id + 1 ;
  id

let new_var level = 
  let id = next_id () in
  TVar (ref (Unbound(id, level)))

exception Error of string
let error msg = raise (Error msg)

module StringSet = Set.Make (String)
module TVarSet = Set.Make( 
  struct
    let compare = compare
    type t = tvar ref
  end )
module Env = struct
  module StringMap = Map.Make (String)
  type env = ty StringMap.t
  let empty : env = StringMap.empty
  let extend env name ty = StringMap.add name ty env
  let lookup env name = StringMap.find name env
end

let occurs_check_adjust_levels _ tvar_level ty =
  let already_adjusted = TVarHashtbl.create 10 in
  let rec f ty= 
    match ty with
    | TVar ({contents = Link ty} as tvar) ->
      if TVarHashtbl.mem already_adjusted (tvar) then () else begin
        TVarHashtbl.add already_adjusted (tvar) () ;
        f ty
      end;
    | TVar {contents = Generic _} -> assert false
    | TVar ({contents = Unbound(other_id, other_level)} as other_tvar) ->
      if other_level > tvar_level then
        other_tvar := Unbound(other_id, tvar_level)
      else
        ()
    | TArrow(param_ty_list, return_ty) ->
      List.iter (fun param -> f (snd param)) param_ty_list ;
      f return_ty
    | TRecord row ->
      f row
    | TVariant row ->
      f row
    | TRowExtend(_, field_ty, row) -> f field_ty ;
      f row
    | TRowEmpty -> ()
  in
  f ty

let namecheck param1 param2 =
  let name1, _ = param1 in
  let name2, _ = param2 in
  if name1 <> name2 then error "parameter names must match"
  else ()
let already_unified = TVarPairHashtbl.create 10
(* FIXME: Get rid of labels_found *)
let rec unify ty1 ty2 labels_found =
  if (string_of_ty 0 ty1 = string_of_ty 0 ty2) then () else
  if (match ty1, ty2 with 
      | TVar tvar1, TVar tvar2 -> TVarPairHashtbl.mem already_unified (tvar1, tvar2)
      | _ -> false) then () else
    begin
      begin
        match ty1, ty2 with
        | TVar tvar1, TVar tvar2 -> 
          TVarPairHashtbl.add already_unified (tvar1, tvar2) ();
          TVarPairHashtbl.add already_unified (tvar2, tvar1) ()
        | _ -> ()
      end;
      if ty1 == ty2 then () else
        let tystr2 = string_of_ty 0 ty2 in
        let tystr1 = string_of_ty 0 ty1 in
        begin 
          match (ty1, ty2) with
          | TArrow(param_ty_list1, return_ty1), TArrow(param_ty_list2, return_ty2) ->
            List.iter2 (fun param1 param2 -> unify (snd param1) (snd param2) StringSet.empty) param_ty_list1 param_ty_list2 ;
            unify return_ty1 return_ty2 StringSet.empty;
            if List.length param_ty_list1 <> List.length param_ty_list2 then error "parameter lists must be the same length" ;
            List.iter2 namecheck param_ty_list1 param_ty_list2 ;
          | TVar ({contents = Link ty1}), ty2 | ty1, TVar {contents = Link ty2} ->
            unify ty1 ty2 labels_found;
            (* For RecLink, do unification and make sure haven't been unified yet *)
          | TVar {contents = Unbound(id1, _)}, TVar {contents = Unbound(id2, _)} when id1 = id2 ->
            assert false (* There is only a single instance of a particular type variable. *)
          | TVar ({contents = Unbound(id, level)} as tvar), ty
          | ty, TVar ({contents = Unbound(id, level)} as tvar) ->
            occurs_check_adjust_levels id level ty ;
            tvar := Link ty;
            (* Make a recursive link if adjust levels indicates so *)
          | TRecord row1, TRecord row2 -> 
            unify row1 row2 labels_found
          | TVariant row1, TVariant row2 -> 
            unify row1 row2 labels_found
          | TRowEmpty, TRowEmpty ->
            ()
          | TRowExtend(label1, field_ty1, rest_row1), (TRowExtend _ as row2) -> begin
              let rest_row2 = rewrite_row row2 label1 field_ty1 labels_found in
              let labels_found = StringSet.add label1 labels_found in
              unify rest_row1 rest_row2 labels_found
            end
          | _, _ -> error ("cannot unify types." ^ tystr1 ^ " and " ^ tystr2)
        end
    end;

and rewrite_row row2 label1 field_ty1 labels_found = 
  match row2 with
  | TRowEmpty -> error ("row does not contain label " ^ label1)
  | TRowExtend(label2, field_ty2, rest_row2) when label2 = label1 ->
    unify field_ty1 field_ty2 StringSet.empty;
    rest_row2
  | TRowExtend(label2, field_ty2, rest_row2) ->
    TRowExtend(label2, field_ty2, rewrite_row rest_row2 label1 field_ty1 labels_found)
  | TVar {contents = Link row2} -> rewrite_row row2 label1 field_ty1 labels_found
  (* RecLink -> only do once. Should this even be possible?*)
  | TVar ({contents = Unbound(_, level)} as tvar) ->
    let rest_row2 = new_var level in
    let ty2 = TRowExtend(label1, field_ty1, rest_row2) in
    tvar := Link ty2 ;
    rest_row2
  | _ -> error "row type expected"

let generalize level = 
  let already_generalized = TVarHashtbl.create 10 in
  let rec generalize level ty =
    match ty with
    | TVar {contents = Unbound(id, other_level)} when other_level > level ->
      TVar (ref (Generic id))
    | TArrow(param_ty_list, return_ty) ->
      TArrow(List.map (fun param -> fst param, (generalize level) (snd param)) param_ty_list, generalize level  return_ty)
    | TVar ({contents = Link ty} as ty2) ->
      begin
        match TVarHashtbl.find_opt already_generalized ty2 with
        | None -> 
          let newvar = ref (Unbound(next_id (), level)) in
          TVarHashtbl.add already_generalized ty2 newvar;
          let generalized_ty = generalize level ty in 
          newvar := Link generalized_ty;
          TVar newvar
        | Some newvar -> TVar newvar
      end
    (* RecLink - same but make sure we don't loop too many times *)
    | TRecord row -> TRecord (generalize level row)
    | TVariant row -> TVariant (generalize level row)
    | TRowExtend(label, field_ty, row) ->
      TRowExtend(label, generalize level field_ty , generalize level row)
    | TVar {contents = Generic _} | TVar {contents = Unbound _} | TRowEmpty as ty -> ty
  in generalize level

let instantiate level ty =
  let id_var_map = Hashtbl.create 10 in
  let already_instantiated = TVarHashtbl.create 10 in
  let rec f ty = 
    match ty with
    | (TVar ({contents = Link ty} as tvar)) ->
      begin 
        match TVarHashtbl.find_opt already_instantiated tvar with
        | None -> 
          let newvar = ref (Unbound(next_id (), level)) in
          TVarHashtbl.add already_instantiated tvar newvar;
          let ty' = f ty in
          newvar := Link ty' ;
          TVar newvar
        | Some ty' ->
          TVar ty'
      end
    | TVar {contents = Generic id} -> begin
        try
          Hashtbl.find id_var_map id
        with Not_found ->
          let var = new_var level in
          Hashtbl.add id_var_map id var ;
          var
      end
    | TVar {contents = Unbound _} -> 
      ty
    | TArrow(param_ty_list, return_ty) ->
      TArrow(List.map (fun param -> fst param, f (snd param)) param_ty_list, f return_ty)
    | TRecord row -> TRecord (f row)
    | TVariant row -> TVariant (f row)
    | TRowEmpty ->
      ty
    | TRowExtend(label, field_ty, row) ->
      TRowExtend(label, f field_ty, f row)
  in
  f ty

let rec match_fun_ty param_names = function
  | TArrow(param_ty_list, return_ty) ->
    if List.length param_ty_list <> List.length param_names then
      error "unexpected number of arguments"
    else
      let namecheck name param = if name <> fst param then error "parameter names must match" in
      List.iter2 namecheck param_names param_ty_list ;
      param_ty_list, return_ty
  | TVar {contents = Link ty} -> match_fun_ty param_names ty
  (* I think Recursive Link is impossible since functions aren't first class *)
  | TVar ({contents = Unbound(_, level)} as tvar) ->
    let param_ty_list = List.map (fun name -> name, new_var level) param_names in
    let return_ty = new_var level in
    tvar := Link (TArrow(param_ty_list, return_ty)) ;
    param_ty_list, return_ty
  | _ -> error "expected a function"

let rec get_row row label =
  match row with 
  | TRowExtend(l, field_ty, rest_row) ->
    if l = label then Some field_ty
    else get_row rest_row label
  | TVar {contents = Link row} -> get_row row label
  | TRecord row -> get_row row label
  | TVariant row -> get_row row label
  | _ -> None
let rec row_is_complete row =
  match row with
  | TRowEmpty -> true
  | TRowExtend(_, _, rest_row) -> row_is_complete rest_row
  | TVar {contents = Link row} -> row_is_complete row
  | TRecord row -> row_is_complete row
  | TVariant row -> row_is_complete row
  | _ -> false

let rec infer_method env level meth =
  let param_ty_list = List.map (fun arg -> arg, new_var level) meth.args in
  let fn_env = List.fold_left2
      (fun env param_name param_ty -> Env.extend env param_name (snd param_ty))
      env meth.args param_ty_list
  in
  let return_ty, _ = infer fn_env level meth.method_body in
  TArrow(param_ty_list, return_ty), env
and infer_record_select env level rm =
  let rest_row_ty = new_var level in
  let field_ty = new_var level in
  let param_ty = TRecord (TRowExtend(rm.rm_message, field_ty, rest_row_ty)) in
  let return_ty = field_ty in
  unify param_ty (fst (infer env level rm.rm_receiver)) StringSet.empty ;
  return_ty, env
and infer env level = function
  | RecordExtension re ->
    begin
      match re.variant_expr with 
      | Some (label, expr) -> 
        begin
          let rest_row_ty = new_var level in
          let variant_ty = new_var level in
          let param_ty = variant_ty in
          let return_ty = TVariant (TRowExtend(label, variant_ty, rest_row_ty)) in
          let e_ty, _ = (infer env level expr) in
          unify param_ty e_ty StringSet.empty ; 
          return_ty, env
        end
      | None -> begin
          let rest_row_ty = new_var level in
          let field_ty = new_var level in
          let param1_ty = field_ty in
          let param2_ty = TRecord rest_row_ty in
          let return_ty = TRecord (TRowExtend(re.field.field_name, field_ty, rest_row_ty)) in
          let extension_ty = fst (infer env level re.extension) in 
          unify param1_ty (fst (infer_method env level re.field.field_value)) StringSet.empty ;
          unify ( param2_ty) (extension_ty) StringSet.empty ;
          (* If we know for sure that the label doesn't exist then extend*)
          if row_is_complete param2_ty && get_row param2_ty re.field.field_name = None then
            return_ty, env
            (* otherwise, replace *)
          else
            begin
              let field_ty1 = param1_ty in
              let field_ty2 = new_var level in
              let rest_row_ty2 = new_var level in
              unify extension_ty (TRecord (TRowExtend(re.field.field_name, field_ty2, rest_row_ty2))) StringSet.empty ;
              ignore field_ty1;
              unify field_ty1 field_ty2 StringSet.empty ;
              extension_ty, env
            end
        end
    end
  | EmptyRecord | OcamlCall _ -> TRecord TRowEmpty, env
  | Record_Message rm ->
    begin
      match rm.cases with 
      | Some (expr, cases) -> 
        begin
          let return_ty = new_var level in
          let expr_ty, _ = infer env level expr in
          let cases_row = infer_cases env level return_ty TRowEmpty cases in
          prerr_endline (Ast_utils.string_of_ast expr 0);
          unify expr_ty (TVariant cases_row) StringSet.empty;
          return_ty, env
        end
      | None -> 
        begin
          let fn_type = fst (infer_record_select env level rm) in
          let param_ty_list, return_ty =
            match_fun_ty (List.map (fun param -> param.name) rm.rm_arguments) fn_type
          in
          List.iter2
            (fun param_ty arg -> unify (snd param_ty) (fst (infer env level arg.value)) StringSet.empty)
            param_ty_list rm.rm_arguments
          ;
          return_ty, env
        end
    end
  | Sequence s ->
    let _, env' = infer env level s.l in
    let ty2, _ = infer env' level s.r in 
    ty2, env
  | Declaration d ->
    let var_ty, env = infer env (level + 1) d.decl_rhs in
    let generalized_ty = generalize level var_ty in
    let env = Env.extend env d.decl_lhs generalized_ty in
    (TRowEmpty), env
  | Variable v ->
    begin
      try
        let a = instantiate level (Env.lookup env v.var_name), env in
        a
      with Not_found -> error ("variable " ^ v.var_name ^ " not found")
    end
and infer_cases env level return_ty rest_row_ty cases = match cases with
  | [] -> rest_row_ty
  | (label, var_name, expr) :: other_cases ->
    let variant_ty = new_var level in
    unify return_ty (fst (infer (Env.extend env var_name variant_ty) level expr)) StringSet.empty;
    let other_cases_row = infer_cases env level return_ty rest_row_ty other_cases in
    TRowExtend(label, variant_ty, other_cases_row)