type name = string
let prerr_endline = print_endline
let prerr_int = print_int
type id = int
type level = int
type ty =
  | TArrow of ((name * ty) list) * ty             (* function type: `(int, int) -> int` *)
  | TVar of tvar ref                  (* type variable *)
  | TRowEmpty                         (* empty row: `<>` *)
  | TRowExtend of name * ty * row     (* row extension: `<a : _ | ...>` *)
  | TRecord of row                    (* record type: `{<...>}` *)
and row = ty    (* the kind of rows - empty row, row variable, or row extension *)
(* URGENT: Recursive link *)
and tvar =
  | Unbound of id * level
  | Link of ty
  | Generic of id
  (* FIXME: Show fixpoint stuff *)
module TVarMap = Map.Make( 
  struct
    type t = tvar ref
    let compare = compare
  end )
module TVarHashtbl = Hashtbl.Make(struct
    type t = (tvar ref)
    let equal = (==)
    let hash = Hashtbl.hash
  end)
module TVarPairHashtbl = Hashtbl.Make(struct
    type t = ((tvar ref) * (tvar ref))
    let equal l r = (fst l) == (fst r) && (snd l) == (snd r)
    let hash = Hashtbl.hash
  end)
let contains s1 s2 =
  try
    let len = String.length s2 in
    for i = 0 to String.length s1 - len do
      if String.sub s1 i len = s2 then raise Exit
    done;
    false
  with Exit -> true
let rec do_indent indent =
  if indent = 0 then "" else "  " ^ do_indent (indent - 1)
let nl indent =
  "\n" ^ (do_indent indent)

let seen = TVarHashtbl.create 10
let rec eager_print_ty ty =
  match ty with
  | TArrow (args, ret) ->
    let print_arg arg = 
      let name, ty = arg in
      prerr_string name;
      prerr_string " : ";
      eager_print_ty ty in
    List.iter print_arg args;
    eager_print_ty ret
  | TVar ({contents = Link ty} as tvar) ->
    (match TVarHashtbl.find_opt seen tvar with
     | Some id -> prerr_string ("fix" ^ string_of_int id)
     | None ->
       let id = TVarHashtbl.length seen in
       TVarHashtbl.add seen tvar id;
       prerr_string ("fix" ^ string_of_int id ^ ".");
       eager_print_ty ty)
  | TVar {contents = Unbound (id, _)} -> prerr_string ("t" ^ string_of_int id)
  | TVar {contents = Generic id} -> prerr_string ("forall t" ^ string_of_int id)
  | TRowEmpty -> prerr_string "<>"
  | TRowExtend (name, ty, row) ->
    prerr_string ("<" ^ name ^ " = ");
    eager_print_ty ty;
    prerr_string (" | ");
    eager_print_ty row;
    prerr_string ">"
  | TRecord row -> prerr_string "{"; eager_print_ty row; prerr_string "}"

let string_of_ty indent ty: string =
  let curr_id = ref 0 in
  let next_id () =
    let id = !curr_id in
    curr_id := id + 1;
    id in
  let already_printed = TVarHashtbl.create 10 in
  (* FIXME: Could be more efficient *)
  let rec string_of_ty indent ty : string =
    let ty_str =
      match ty with
      | TArrow (args, ret) ->
        let args = List.map (fun (name, ty) -> name ^ " : " ^ string_of_ty indent ty) args in
        let args = String.concat ", " args in
        "[(" ^ args ^ ") -> " ^ string_of_ty indent ret ^ "]"
      | TVar ({contents = Link ty} as tvar) ->
        ignore (TVarHashtbl.find_opt already_printed tvar);
        (match TVarHashtbl.find_opt already_printed tvar with
         | Some id -> 
           "fix" ^ string_of_int id
         | None ->
           let id = (next_id ()) in
           TVarHashtbl.add already_printed tvar id;
           let ty_of_link = string_of_ty indent ty in
           let fix = "fix" ^ string_of_int id in
           if contains ty_of_link fix then
             fix ^ "." ^ ty_of_link
           else 
             ty_of_link)
      (* For RecLink, just print id *)
      | TVar {contents = Unbound (id, _)} -> 
        "t" ^ string_of_int id
      | TVar {contents = Generic id} ->
        "forall t" ^ string_of_int id
      | TRowEmpty -> 
        "<>"
      | TRowExtend (name, ty, row) ->
        "<" ^ name ^ " = " ^ string_of_ty indent ty ^ " | "  ^
        (nl (indent + 1)) ^
        string_of_ty (indent + 1) row ^ ">"
      | TRecord row ->
        "{" ^ string_of_ty indent row ^ "}" in 
    ty_str 
  in string_of_ty indent ty
let fields_of ty =
  let rec fields_of ty =
    match ty with
    | TRowEmpty -> ""
    | TRowExtend (name, _, row) -> name ^ fields_of row
    | TVar {contents = Link ty} -> fields_of ty
    | _ -> ""
  in
  fields_of ty


let labels_of indent ty: string =
  let already_printed = TVarHashtbl.create 10 in
  (* FIXME: Could be more efficient *)
  let rec labels_of indent ty : string =
    let ty_str =
      match ty with
      | TArrow _ ->
        "->"
      | TVar ({contents = Link ty} as tvar) ->
        ignore (TVarHashtbl.find_opt already_printed tvar);
        (match TVarHashtbl.find_opt already_printed tvar with
         | Some _ -> 
           "l"
         | None ->
           TVarHashtbl.add already_printed tvar ();
           labels_of indent ty
        )
      (* For RecLink, just print id *)
      | TVar {contents = Unbound (id, _)} -> 
        " " ^ string_of_int id
      | TVar {contents = Generic id} ->
        "generic "^ string_of_int id
      | TRowEmpty -> 
        "<>"
      | TRowExtend (name, _, row) ->
        name ^ ", " ^ labels_of indent row
      | TRecord row ->
        "{" ^ labels_of indent row ^ "}" in 
    ty_str 
  in labels_of indent ty