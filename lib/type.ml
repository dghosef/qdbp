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
  | TVariant of row                    (* record type: `{<...>}` *)
and row = ty    (* the kind of rows - empty row, row variable, or row extension *)
and tvar =
  | Unbound of id * level
  | Link of ty
  | Generic of id
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
        "{" ^ string_of_ty indent row ^ "}" 
      | TVariant row ->
        "<|" ^ string_of_ty indent row ^ "|>" in 
    ty_str 
  in string_of_ty indent ty