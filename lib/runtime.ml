let prelude =
{qdbp_prelude|
module StringMap = Map.Make(String)
type __qdbp_obj =
  | TaggedObject of string * __qdbp_obj
  | Prototype of (__qdbp_obj list -> __qdbp_obj) StringMap.t
  | Int of int
  | String of string
  | Float of float

let __qdbp_first lst = 
  match lst with
  | [] -> failwith "INTERNAL ERROR: __qdbp_first called on empty list"
  | x :: _ -> x

let __qdbp_rest lst =
  match lst with
  | [] -> failwith "INTERNAL ERROR: __qdbp_rest called on empty list"
  | _ :: xs -> xs

let __qdbp_empty_prototype = 
  Prototype StringMap.empty

let __qdbp_extend prototype name value =
  match prototype with
  | Prototype map -> Prototype (StringMap.add name value map)
  | _ -> failwith "INTERNAL ERROR: __qdbp_extend called on non-prototype"

let __qdbp_tagged_object tag value =
  TaggedObject (tag, value)

let __qdbp_method_invocation receiver name arg =
  match receiver with 
  | Prototype map ->
    (StringMap.find name map) arg
  | _ -> failwith "INTERNAL ERROR: __qdbp_method_invocation called on non-prototype"

let __qdbp_pattern_match value cases =
  match value with
  | TaggedObject (tag, value) ->
    let meth = snd (List.find (fun (tag', _) -> tag = tag') cases) in
    meth value

  | _ -> failwith "INTERNAL ERROR: __qdbp_pattern_match called on non-tagged object"
|qdbp_prelude}