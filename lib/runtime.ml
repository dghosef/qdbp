type base_object =
  | Int of int
  | EmptyRecord
  
type record =
  | Extension of record_extension
  | Empty of base_object
and record_extension =
  {field_name: string;
   field_method: meth;
   extension: record}
(* Change from record list to array or something*)
and meth = (record list) -> record
let emptyrecord = Empty EmptyRecord
let select field_name record =
  let rec select field_name record =
    match record with
    | Empty _ -> raise Not_found
    | Extension {field_name = field_name'; field_method = field_method;
                 extension = extension} ->
      if field_name = field_name' then
        field_method
      else
        select field_name extension
  in
  select field_name record