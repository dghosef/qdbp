(* FIXME: Rename files/rearrange stuff? *)
(* FIXME:Rename everything with "field"? *)
(* FIXME: Rename everythign? *)
(* FIXME: loc should be an option and have a start and end *)
type loc = Lexing.position
type id = int option
type argument = {
  arg_id: id;
  name: string; 
  value: expr;
}
and record_field = {
  field_id: id;
  field_name: string; 
  field_value: meth;
  field_location: loc;
}
and meth = {
  meth_id: id;
  location: loc; 
  args: string list; 
  arg_ids: id list;
  method_body: expr
}
and record_extension = {
  field: record_field;
  (* FIXME: Add a position to the extension too*)
  extension: expr;
  extension_id: id;
}
(* TODO: Refactor so that location is independent of the rest*)
and record_message_expr = {
  rm_id: id;
  rm_location: loc; 
  rm_message: string; 
  rm_arguments: argument list;
  rm_receiver: expr
}
and sequence_expr = {
  seq_id: id;
  seq_location: loc;
  l: expr;
  r: expr;
}
and declaration_expr = {
  decl_id: id;
  decl_location: loc; 
  decl_lhs: string; 
  decl_rhs: expr;
}
and var_expr = {
  var_id: id;
  var_location: loc; 
  var_name: string;
  origin: id option;
}
and ocaml_call = {
  call_id: id;
  call_location: loc;
  fn_name: string;
  fn_arg: expr
}
and expr = 
  | RecordExtension of record_extension
  (* FIXME: Make this a struct *)
  | EmptyRecord of id
  (* Are they really messages? If not rename *)
  | Record_Message  of record_message_expr
  | Sequence        of sequence_expr
  | Declaration      of declaration_expr
  (* FIXME: Change to variable read or something?*)
  | Variable        of var_expr
  | OcamlCall         of ocaml_call

let emit_closure loc args body = 
  let args = "self" :: args in
  let fld = {
    field_location = loc;
    field_id = None;
    field_name = "!:" ^ (String.concat ":" args); 
    field_value = {
      meth_id = None;
      location = loc;
      args = args;
      method_body = body;
      arg_ids = List.map (fun _ -> None) args}} in
  (RecordExtension {field= fld; extension = ((EmptyRecord None)); extension_id = None})
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
let emit_variant_message receiver location messages =
  (*FIXME: Rename*)
  let selector = 
    List.fold_left (fun acc elem -> (RecordExtension 
    {field = elem; extension = acc; extension_id = None })) ((EmptyRecord None)) messages
  in
Record_Message
  {
    rm_id = None;
    rm_receiver = receiver;
    rm_arguments = [{name = "Self"; value = receiver; arg_id = None}; {name = "Receiver"; value = selector; arg_id = None}];
    (* FIXME: Use something other than !*)
    rm_message = "!";
    rm_location = location;
  }
let emit_variant loc tag expr =
  let var = Declaration
    {
      decl_id = None;
      decl_location = loc;
      (* FIXME: Change this name? *)
      decl_lhs = "Expr";
      decl_rhs = expr
    }
  in
  (* FIXME: rename message everywhere *)
  let message =
    Record_Message {
      rm_id = None;
      rm_location = loc;
      rm_receiver = Variable 
        {
          var_location = loc;
          var_id = None;
          (* FIXME: Rename Receiver to something else? *)
          var_name = "Receiver";
          origin = None
        };
      rm_arguments = [{
        name = "Self";
        arg_id = None;
        value = Variable {
          var_location = loc;
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
    seq_location = loc;
    seq_id = None;
    l = var;
    r = message
  }
  in
  (* FIXME: We dont' need to do a whole closure since it's guaranteed to not use self *)
  (* FIXME: Rename Receiver to something else? *)
  let args = ["Self"; "Receiver"] in
  let fld = {
    field_location = loc;
    field_id = None;
    field_name = "!"; 
    field_value = {
      meth_id = None;
      location = loc;
      args = args;
      method_body = body;
      arg_ids = List.map (fun _ -> None) args}} in
  (RecordExtension {field= fld; extension = ((EmptyRecord None)); extension_id = None})
