module IntMap = CollectMethods.IntMap
module VarSet = LetBoundVariables.VarSet
module FvSet = Refcount.FvSet
module IntSet = Set.Make(struct type t = int let compare = compare end)
let method_name id = "method_" ^ (string_of_int id)
let rec indent level =
  if level = 0 then ""
  else
    (indent (level - 1)) ^ "  "
let newline level = "\n" ^ (indent level)
let varname id = 
  "var_" ^ (string_of_int (id))

let paren s = "(" ^ s ^ ")"
let brace s = "[" ^ s ^ "]"
let bracket s = "{" ^ s ^ "}"
let quoted s = 
  "\"" ^ (String.escaped s) ^ "\""
let fn_sig id args =
  let args_strs = List.map (
      fun (id, _) -> "qdbp_object_ptr " ^ (varname id)
    ) args in
  "qdbp_object_ptr " ^ (method_name id) ^
  (paren
     (String.concat ", " ("qdbp_object_arr captures" :: args_strs))
  )

let fn_declarations methods =
  let method_strs = List.map (fun (id, (args, _, _, _, _)) -> fn_sig id args)
      (IntMap.bindings methods) in
  let method_strs = List.map (fun s -> s ^ ";") method_strs in
  String.concat "\n" method_strs

let invoke_fn_name cnt = "invoke_" ^ (string_of_int cnt)
let invoke_fns methods =
  let arities =
    IntSet.of_list (
      List.map (fun (_, (args, _, _, _, _)) -> List.length args) (IntMap.bindings methods)) in
  (* Arities 0 and 1 are hardcoded because of int objects *)
  let arities = IntSet.remove 1 arities in
  let arities = IntSet.remove 2 arities in
  let invoke_fn arity =
    (* MUST keep up to date w/ invoke fns in proto.c *)
    let args = List.init arity (fun a -> "qdbp_object_ptr arg" ^ (string_of_int a)) in
    let args = "qdbp_object_ptr receiver" :: "label_t label" :: args in 
    let fn_name = invoke_fn_name arity in
    let fn_args_types = List.init arity (fun _ -> "qdbp_object_ptr") in
    let fn_args_types = "qdbp_object_arr" :: fn_args_types in
    let fn_args_types_str = String.concat ", " fn_args_types in
    let fn_type = "qdbp_object_ptr(*)" ^ (paren (fn_args_types_str)) in
    let fn_args = List.init arity (fun a -> "arg" ^ (string_of_int a)) in
    let fn_args = "captures" :: fn_args in 
    let fn_args_str = String.concat ", " fn_args in
    "qdbp_object_ptr " ^ fn_name ^ (paren (String.concat ", " args)) ^ " {" ^ (newline 1) ^
    "void* code;" ^ (newline 1) ^
    "qdbp_object_arr captures = get_method(receiver, label, &code);" ^ (newline 1) ^
    "return " ^ (paren ((paren fn_type) ^ "code")) ^ (paren fn_args_str) ^ ";\n}"
  in
  let arities_list = IntSet.elements arities in 
  let fns = List.map invoke_fn arities_list in
  String.concat "\n" fns

let c_call fn args =
  fn ^ (paren (String.concat ", " args))

let rec expr_to_c level expr =
  match expr with
  | `Abort _ -> "qdbp_abort()"
  | `Declaration (lhs, rhs, body, _, _) ->
    let body_c = expr_to_c (level + 1) body in
    let rhs_c = expr_to_c (level + 1) rhs in
    "LET" ^ paren(
      (varname (fst lhs)) ^ ", " ^ rhs_c ^ "," ^
      (newline (level + 1)) ^ body_c
    )
  | `Drop (v, e, cnt) -> c_call "DROP" [(varname v); string_of_int cnt; (expr_to_c (level + 1) e);]
  | `Dup (v, e, cnt) ->
    c_call "DUP" [(varname v); string_of_int cnt; (expr_to_c (level + 1) e);]
  | `EmptyPrototype _ -> "empty_prototype()"
  | `IntProto (i, _) -> c_call "make_unboxed_int" [string_of_int i]
  | `ExternalCall (fn, args, _, _) ->
    c_call (fst fn) (List.map (expr_to_c (level + 1)) args)

  | `FloatLiteral (f, _) -> c_call "qdbp_float" [string_of_float f]
  | `IntLiteral (i, _) -> c_call "qdbp_int" [string_of_int i]
  (* FIXME: *)
  | `StringLiteral (s, _) -> c_call "qdbp_string" [quoted s]

  | `MethodInvocation (receiver, label, args, _, _) ->
    let args_c = List.map (fun (_, e, _) -> expr_to_c (level + 1) e) args in
    let label_c = Int64.to_string (fst label) in
    let receiver_c = expr_to_c (level + 1) receiver in
    c_call (invoke_fn_name (List.length args))
      (receiver_c :: label_c :: args_c)
  | `PatternMatch (v, cases, _, _) ->
    let rec cases_to_c level cases =
      match cases with
      | [] -> "match_failed()"
      | ((tag, _), ((arg, _), body, _), _) :: rest ->
        c_call "MATCH" [
          ("tag");
          (string_of_int tag);
          (varname arg);
          expr_to_c (level + 1) body;
          (newline level) ^ (cases_to_c (level + 1) rest)
        ]
    in

    paren ((c_call "decompose_variant" [(varname v); "&tag"; "&payload"]) ^ "," ^ (newline level)^
           cases_to_c level cases)
  | `PrototypeCopy (ext, ((label, _), (meth_id, meth_fvs), _), size, _, op, _) ->
    let ext_c = expr_to_c (level + 1) ext in
    let label_c = Int64.to_string label in
    let fn = match op with
      | `Extend ->
        "extend"
      | `Replace ->
        "replace"
    in
    let code_ptr = "(void*)" ^ (method_name meth_id) in 
    let fv_list = List.map varname (FvSet.elements meth_fvs) in
    let captures_c =
      if (List.length fv_list) > 0 then
        (paren ("struct qdbp_object*" ^ (brace (string_of_int (List.length fv_list))))) ^bracket (String.concat ", " fv_list) 
      else "NULL"
    in
    let args = [
      ext_c;
      label_c;
      code_ptr;
      captures_c;
      string_of_int (List.length fv_list)
    ] in
    let args = match op with
      | `Extend ->
        args @ [string_of_int size]
      | `Replace ->
        args in
    c_call fn args
  | `TaggedObject ((tag, _), value, _, _) ->
    c_call "variant_create" [
      (string_of_int tag);
      expr_to_c (level + 1) value
    ]
  | `VariableLookup (v, _) ->
    varname v

let fn_definitions methods =
  let fn_definition (id, (args, body, _, fvs, bvs)) =
    let declare id =
      (indent 1) ^ "qdbp_object_ptr " ^ (varname id) ^ ";" in
    let init_fv idx id =
      (indent 1) ^ 
      "qdbp_object_ptr " ^ (varname id) ^
      " = (captures" ^ (brace (string_of_int idx)) ^ ");"
    in
    (* Naively, we dup the first arg at callsite then drop it here *)
    (* Instead, omit both operations *)
    let _ = match args with
      | v :: _ -> (newline 1) ^ "drop(" ^ (varname (fst v)) ^ ", 1);"
      | [] -> ""
    in
    let bvs_declarations = List.map declare (VarSet.elements bvs) in
    let fvs_initializations = List.mapi init_fv (FvSet.elements fvs) in
    (fn_sig id args) ^ " " ^ (bracket ( "\n" ^
                                        (String.concat "\n" (List.append bvs_declarations fvs_initializations)) ^ "\n" ^
                                        newline 1 ^ "tag_t tag; qdbp_object_ptr payload;" ^
                                        newline 1 ^ ("return ") ^
                                        (expr_to_c 1 body) ^ ";\n"))
  in
  let method_strs = List.map fn_definition (IntMap.bindings methods) in
  String.concat "\n" method_strs


let codegen_c methods main_method_id =
  let main_method = method_name main_method_id in
  {|
#include "runtime.h"
#include "basic_fns.h"
|} ^
  invoke_fns methods ^ "\n" ^
  fn_declarations methods ^ "\n" ^
  fn_definitions methods ^ "\n" ^
  "int main() {init(); qdbp_object_ptr result = " ^ main_method ^ "(NULL); drop(result, 1); check_mem(); return 0;}"
