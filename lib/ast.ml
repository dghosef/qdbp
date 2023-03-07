(* FIXME: Rename files/rearrange stuff? *)
(* FIXME:Rename everything with "field"? *)
(* FIXME: Rename everythign? *)
(* FIXME: loc should be an option and have a start and end *)
(* FIXME: Make separate cases and variant ast nodes *)
(* FIXME: Add wildcard variatn? *)
type id = int option
type argument = {
  name: string; 
  value: expr;
}
and record_field = {
  field_name: string; 
  field_value: meth;
}
and meth = {
  meth_id: id;
  args: string list; 
  arg_ids: id list;
  method_body: expr;
  case: (string * expr) option (* variable, expr *);
}
and record_extension = {
  variant_expr: (string * expr) option;
  field: record_field;
  (* FIXME: Add a position to the extension too*)
  extension: expr;
  extension_id: id;
}
(* TODO: Refactor so that location is independent of the rest*)
and record_message_expr = {
  rm_id: id;
  rm_message: string; 
  rm_arguments: argument list;
  rm_receiver: expr;
  cases: (expr * ((string * string * expr) list)) option
}
and sequence_expr = {
  seq_id: id;
  l: expr;
  r: expr;
}
and declaration_expr = {
  decl_id: id;
  decl_lhs: string; 
  decl_rhs: expr;
}
and var_expr = {
  var_id: id;
  var_name: string;
  origin: id option;
}
and ocaml_call = {
  call_id: id;
  fn_name: string;
  fn_args: expr list
}
and expr = 
  | RecordExtension of record_extension
  (* FIXME: Make this a struct *)
  | EmptyRecord
  (* Are they really messages? If not rename *)
  | Record_Message  of record_message_expr
  | Sequence        of sequence_expr
  | Declaration      of declaration_expr
  (* FIXME: Change to variable read or something?*)
  | Variable        of var_expr
  | OcamlCall         of ocaml_call

let emit_closure args body = 
  let args = "self" :: args in
  let fld = {
    field_name = "!:" ^ (String.concat ":" args); 
    field_value = {
      meth_id = None;
      case = None;
      args = args;
      method_body = body;
      arg_ids = List.map (fun _ -> None) args}} in
  (RecordExtension {field= fld; extension = ((EmptyRecord)); extension_id = None;
  variant_expr = None})
(* Need to make sure arg names make sense *)
(*
  x = #Tag expr
  ^x
    A? [self| A body]
    B? [self| B body]
    C? [self| C body]

  becomes:
  x = 
    Expr = expr 
    ^{Receiver| Receiver #Tag self: Expr}
  ^x! Receiver: {
    #A [self| #A body]
    #B [self| #B body]
    #C [self| #C body]
  }
*)
let emit_variant_message receiver messages =
  (*FIXME: Rename*)
  let selector = 
    List.fold_left (fun acc elem -> (RecordExtension 
    {field = elem; extension = acc; extension_id = None; variant_expr = None })) ((EmptyRecord)) messages
  in
  let tag_names = List.map (fun elem -> elem.field_name) messages in
  let cases = List.map (fun elem -> elem.field_value.case) messages in
  let cases = List.map(fun elem -> match elem with
    | Some (name, expr) -> (name, expr)
    | None -> failwith "No case for variant message") cases in
  let cases = List.map2 (fun tagname (varname, e) -> (tagname, varname, e)) tag_names cases in
  let cases = receiver, cases in
Record_Message
  {
    rm_id = None;
    cases = Some cases;
    rm_receiver = receiver;
    rm_arguments = [{name = "Self"; value = receiver;}; {name = "Receiver"; value = selector; }];
    (* FIXME: Use something other than !*)
    rm_message = "!";
  }
let emit_variant tag expr =
  let var = Declaration
    {
      decl_id = None;
      (* FIXME: Change this name? *)
      decl_lhs = "Expr";
      decl_rhs = expr
    }
  in
  (* FIXME: rename message everywhere *)
  let message =
    Record_Message {
      rm_id = None;
      cases = None;
      rm_receiver = Variable 
        {
          var_id = None;
          (* FIXME: Rename Receiver to something else? *)
          var_name = "Receiver";
          origin = None
        };
      rm_arguments = [{
        name = "Self";
        value = Variable {
          var_id = None;
          (* FIXME: Change this name? *)
          (* FIXME: Put all change this name? fixme into variables and change to TODOs *)
          var_name = "Expr";
          origin = None
        }
      }];
      (* FIXME: Put hashtag before? *)
      rm_message = tag;
    }
  in
  let body = Sequence {
    seq_id = None;
    l = var;
    r = message
  }
  in
  (* FIXME: We dont' need to do a whole closure since it's guaranteed to not use self *)
  (* FIXME: Rename Receiver to something else? *)
  let args = ["Self"; "Receiver"] in
  let fld = {
    field_name = "!"; 
    field_value = {
      meth_id = None;
      case = None;
      args = args;
      method_body = body;
      arg_ids = List.map (fun _ -> None) args}} in
  (RecordExtension {field= fld; extension = ((EmptyRecord)); extension_id = None;
    variant_expr = Some (tag, expr)})
