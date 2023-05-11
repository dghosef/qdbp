let prelude = {|
module IntMap = Map.Make(struct type t = int let compare = compare end)
type __qdbp_obj =
  | TaggedObject of int * __qdbp_obj
  | Prototype of (__qdbp_obj list -> __qdbp_obj) IntMap.t
  | Int of int
  | String of string
  | Float of float

let __qdbp_int_literal x = Int x
let __qdbp_string_literal x = String x
let __qdbp_float_literal x = Float x
let __qdbp_abort () = failwith "ABORT"

let __qdbp_first lst = 
  match lst with
  | [] -> failwith "INTERNAL ERROR: __qdbp_first called on empty list"
  | x :: _ -> x

let __qdbp_rest lst =
  match lst with
  | [] -> failwith "INTERNAL ERROR: __qdbp_rest called on empty list"
  | _ :: xs -> xs

let __qdbp_empty_prototype = 
  Prototype IntMap.empty

let __qdbp_extend prototype name value =
  match prototype with
  | Prototype map -> Prototype (IntMap.add name value map)
  | _ -> failwith "INTERNAL ERROR: __qdbp_extend called on non-prototype"

let __qdbp_tagged_object tag value =
  TaggedObject (tag, value)

let __qdbp_method_invocation receiver name arg =
  match receiver with 
  | Prototype map ->
    begin
    try
      (IntMap.find name map) arg
    with Not_found ->
      failwith 
      ("INTERNAL ERROR: method with label " ^ (string_of_int name) ^ " not found. The available methods are: " ^
      (String.concat ", " (IntMap.fold (fun k _ acc -> (string_of_int k) :: acc) map [])))
    end
  | _ -> failwith "INTERNAL ERROR: __qdbp_method_invocation called on non-prototype"

let __qdbp_pattern_match value cases =
  match value with
  | TaggedObject (tag, value) ->
    let meth = snd (List.find (fun (tag', _) -> tag = tag') cases) in
    meth [value]
  | _ -> failwith "INTERNAL ERROR: __qdbp_pattern_match called on non-tagged object"

let __qdbp_true = TaggedObject (1, __qdbp_empty_prototype)
let __qdbp_false = TaggedObject (0, __qdbp_empty_prototype)

let __qdbp_make_binary_int_op_int op =
  fun x y ->
  match x, y with
  | Int x, Int y -> Int (op x y)
  | _ -> failwith "INTERNAL ERROR: __qdbp_make_binary_int_op called on non-int"

let __qdbp_make_binary_int_op_bool op =
  fun x y ->
  match x, y with
  | Int x, Int y -> if op x y then __qdbp_true else __qdbp_false
  | _ -> failwith "INTERNAL ERROR: __qdbp_make_binary_int_op called on non-int"

let __qdbp_make_binary_float_op_float op =
  fun x y ->
  match x, y with
  | Int x, Int y -> Int (op x y)
  | _ -> failwith "INTERNAL ERROR: __qdbp_make_binary_int_op called on non-int"

let __qdbp_make_binary_float_op_bool op =
  fun x y ->
  match x, y with
  | Int x, Int y -> if op x y then __qdbp_true else __qdbp_false
  | _ -> failwith "INTERNAL ERROR: __qdbp_make_binary_int_op called on non-int"

let qdbp_zero_int = Int 0
let qdbp_int_add_int = __qdbp_make_binary_int_op_int (+)
let qdbp_int_sub_int = __qdbp_make_binary_int_op_int (-)
let qdbp_int_mul_int = __qdbp_make_binary_int_op_int ( * )
let qdbp_int_div_int = __qdbp_make_binary_int_op_int (/)
let qdbp_int_mod_int = __qdbp_make_binary_int_op_int (mod)
let qdbp_int_lt_bool = __qdbp_make_binary_int_op_bool (<)
let qdbp_int_le_bool = __qdbp_make_binary_int_op_bool (<=)
let qdbp_int_gt_bool = __qdbp_make_binary_int_op_bool (>)
let qdbp_int_ge_bool = __qdbp_make_binary_int_op_bool (>=)
let qdbp_int_eq_bool = __qdbp_make_binary_int_op_bool (=)
let qdbp_int_ne_bool = __qdbp_make_binary_int_op_bool (<>)
let qdbp_int_to_string =
  fun x -> match x with
    | Int x -> String (string_of_int x)
    | _ -> failwith "INTERNAL ERROR: qdbp_int_to_string called on non-int"

let qdbp_zero_float = Float 0.0
let qdbp_float_add_float = __qdbp_make_binary_float_op_float (+)
let qdbp_float_sub_float = __qdbp_make_binary_float_op_float (-)
let qdbp_float_mul_float = __qdbp_make_binary_float_op_float ( * )
let qdbp_float_div_float = __qdbp_make_binary_float_op_float (/)
let qdbp_float_mod_float = __qdbp_make_binary_float_op_float (mod)
let qdbp_float_lt_bool = __qdbp_make_binary_float_op_bool (<)
let qdbp_float_le_bool = __qdbp_make_binary_float_op_bool (<=)
let qdbp_float_gt_bool = __qdbp_make_binary_float_op_bool (>)
let qdbp_float_ge_bool = __qdbp_make_binary_float_op_bool (>=)
let qdbp_float_eq_bool = __qdbp_make_binary_float_op_bool (=)
let qdbp_float_ne_bool = __qdbp_make_binary_float_op_bool (<>)

let qdbp_empty_string = String ""
let qdbp_float_to_string = 
  fun x -> match x with
    | Float x -> String (string_of_float x)
    | _ -> failwith "INTERNAL ERROR: qdbp_float_to_string called on non-float"

let qdbp_print_string_int =
  fun x -> match x with
    | String x ->
      print_string x;
      Int 0
    | _ -> failwith "INTERNAL ERROR: qdbp_print_string_int called on non-string"
let concat_string x y =
  match x, y with
  | String x, String y -> String (x ^ y)
  | _ -> failwith "INTERNAL ERROR: concat_string called on non-string"

(* Sorta useless *)
let foreign_fn_bool _ _ = __qdbp_true
|}