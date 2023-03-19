(* TODO: Add better comments *)
(* FIXME: I don't like self used for variants. Use some other convention *)
(* FIXME: Allow unnamed first param by having a set name for it *)
(* FIXME: empty method returns {} by default *)
%token<string> UPPER_ID LOWER_ID ARG IMPORT STRING
%token <int> INT
%token PIPE DECLARATION PERIOD TAG QUESTION MONEY ABORT
%token LPAREN RPAREN
%token LBRACE RBRACE
%token LBRACKET RBRACKET
%token EOF

(* LOWEST PRECEDNCE *)
(*===============================================================*)
%nonassoc decl_in_prec
%right UPPER_ID
(*===============================================================*)
(* HIGHEST PRECEDNCE *)
%start <Ast.expr> program
%%
(* TODO: More specific parse messages *)
(* TODO: Module system *)
(* TODO: Add start and end positions to tokens and productions. Figure out in general how to do start and end locations *)

program:
| e = expr; EOF {e}

expr:
| LPAREN; e = expr; RPAREN; {e}
| e = record                {e} (* URGENT: Make it a top to bottom order *)
| e = variant               {e}
| e = record_message        {e}
| e = variant_message       {e}
| e = declaration            {e}
| e = variable              {e}
| e = ocaml_call            {e}
| e = import_expr {e}
| e = int_literal {e}
| e = empty_list {e}
| e = abort {e}
| e = string{e}



(*FIXME: Make sure we don't have duplicate arguments*)
meth:
| LBRACE; a = arg_list; e = expr; RBRACE
  {{
    args = List.append ["self"] a;
    method_body = e;
    case = None;
    arg_ids = None :: (List.map (fun _ -> None) a);
  }}
| LBRACE; e = expr; RBRACE
  {{
    args = ["self"];
    method_body = e;
    case = None;
    arg_ids = [None]
  }}
(* FIXME: Better syntax *)
arg_list:
| params = nonempty_list(LOWER_ID); PIPE {params}

(* FIXME: Make sure method fields have self stuff *)
(* FIXME: Don't allow {...asdf} by itself? *)
record:
| LBRACKET; r = record_body; RBRACKET
  {r}
| LBRACKET; RBRACKET
  {Ast.EmptyRecord}
| c = closure
  {c}

record_field:
| id = UPPER_ID; m = meth; 
  {let (m: Ast.meth) = m in
  let (field: Ast.record_field) = {
    field_name = id ^ ":" ^ (String.concat ":" m.args);
    field_value = m;
  } in field
  }

record_body:
| extension = expr; fields = record_field+; 
  {
    List.fold_left (fun (acc: Ast.expr) (field: Ast.record_field) -> 
      Ast.RecordExtension {
        field = field;
        extension = acc;
        variant_expr = None;
      }) extension fields
  }
| fields = record_field+; 
  {
    List.fold_left (fun (acc: Ast.expr) (field: Ast.record_field) -> 
      Ast.RecordExtension {
        field = field;
        extension = acc;
        variant_expr = None;
      }) (Ast.EmptyRecord) fields
  }

(* FIXME: Make first arg not necessarily require a colon *)
record_message:
| r = expr; id = UPPER_ID; arg1 = expr?; a = record_message_arg*; PERIOD;
| LPAREN; r = expr; id = UPPER_ID; arg1 = expr?; a = record_message_arg*; RPAREN;
  { 
    let a: Ast.argument list = match arg1 with
      | None -> a
      | Some x -> ({name = "val"; value = x}) :: a in
    let var = Ast.Declaration {
      decl_id = None;
      (* FIXME: Change this *)
      decl_lhs = "Self";
      decl_rhs = r;
    } in
    let to_concat = (String.concat ":" (List.map (fun (x: Ast.argument) -> x.name) a)) in
    let rm = Ast.Record_Message {
      cases = None;
      rm_receiver = Ast.Variable {var_name = "Self"; origin = None};
      rm_message = id ^ ":self" ^ (if to_concat = "" then "" else ":") ^ to_concat;
      rm_arguments = {
        name = "self"; 
        value = Ast.Variable 
          {
          var_name = "Self"; 
          origin = None}} :: a
    } in 
    Ast.Sequence {
      l = var;
      r = rm;
    }
  }
record_message_arg:
| id = ARG; e = expr; 
  {let r: Ast.argument = {
    name = id; 
    value = e
  } in r}

variant:
| TAG; id = UPPER_ID; e = expr;
  {Ast.emit_variant ("#" ^ id) e}

variant_meth:
| LBRACE; a = LOWER_ID; PIPE; e = expr; RBRACE
  {{
    args = ["Self"];
    method_body = 
    Ast.Sequence {
      l = Ast.Declaration {
        decl_id = None;
        decl_lhs = a;
        decl_rhs = Ast.Variable {
          var_name = "Self";
          origin = None;
        }
      };
      r = e;
    }
    ;
    case = Some (a, e);
    arg_ids = [None]
  }}
| LBRACE; e = expr; RBRACE
  {let (meth:Ast.meth) = {
    args = ["Self"];
    method_body = e;
    case = Some ("Self", e);
    arg_ids = [None]
  } in meth}
variant_message:
| r = expr; m = tag_message+; PERIOD;
| LPAREN; r = expr; m = tag_message+; RPAREN
  {Ast.emit_variant_message r m}
tag_message:
| n = UPPER_ID; QUESTION; e = variant_meth;
  {{
    field_name = "#" ^ n;
    field_value = e
  }}

decl_in: e = expr {e} %prec decl_in_prec
declaration:
| id = LOWER_ID; DECLARATION; r = expr; e = decl_in
  {
    Ast.Sequence {
      l = Ast.Declaration {
        decl_lhs = id;
        decl_id = None;
        decl_rhs = r;
      };
      r = e;
      }
  }

variable:
| name = LOWER_ID
  {Ast.Variable {
    var_name = name;
    origin = None;
  }}

closure:
| LBRACKET; a = arg_list; e = expr; RBRACKET
  {Ast.emit_closure a e}
| LBRACKET; e = expr; RBRACKET
  {Ast.emit_closure [] e}

// TODO: Strings(including fancy nested strings like r"asdf"r) and lists
(* FIXME: Add cycle detection *)

import_expr:
| filename = IMPORT
  {
    Ast.Import { filename = filename ^ ".qdbp"; import_expr = EmptyRecord; }
  }

int_literal:
| i = INT
  {
    let literal = Ast.IntLiteral i in
    let int_fn = (* FIXME: change filename? *)
      Ast.Import { filename = "int.qdbp"; import_expr = EmptyRecord; } in
    let variable_decl = Ast.Declaration {
      decl_id = None;
      decl_lhs = "self";
      decl_rhs = int_fn;
    } in
    let self = Ast.Variable {
      var_name = "self";
      origin = None;
    } in
    let message = Ast.Record_Message {
      rm_message = "!:self:val";
      rm_arguments = [{name = "self"; value = self}; {name = "val"; value = literal}];
      rm_receiver = self;
      cases = None
    } in
    Ast.Sequence {
      l = variable_decl;
      r = message;
    }
  }
empty_list:
| LBRACE; RBRACE
  {Ast.Import { filename = "list.qdbp"; import_expr = EmptyRecord; }}
abort:
| ABORT; PERIOD {Ast.Abort}
string:
| s = STRING
  {
    let literal = Ast.StringLiteral s in
    let str_fn = (* FIXME: change filename? *)
      Ast.Import { filename = "string.qdbp"; import_expr = EmptyRecord; } in
    let variable_decl = Ast.Declaration {
      decl_id = None;
      decl_lhs = "self";
      decl_rhs = str_fn;
    } in
    let self = Ast.Variable {
      var_name = "self";
      origin = None;
    } in
    let message = Ast.Record_Message {
      rm_message = "!:self:val";
      rm_arguments = [{name = "self"; value = self}; {name = "val"; value = literal}];
      rm_receiver = self;
      cases = None
    } in
    Ast.Sequence {
      l = variable_decl;
      r = message;
    }
  }
ocaml_call:
| MONEY; id = LOWER_ID LPAREN; args = expr*; RPAREN
  {let call: Ast.ocaml_call = {
    call_id = None;
    fn_name = id;
    fn_args = args;
  } in Ast.OcamlCall call}