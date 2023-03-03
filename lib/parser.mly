(* TODO: Add better comments *)
(* URGENT: I don't like self used for variants. Use some other convention *)
(* FIXME: Allow unnamed first param by having a set name for it *)
(* FIXME: empty method returns {} by default *)
(* URGENT: change precedences for := and messages *)
%token<string> UPPER_ID LOWER_ID ARG
%token PIPE DECLARATION PERIOD TAG QUESTION MONEY
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
| e = record                {e}
| e = variant               {e}
| e = record_message        {e}
| e = variant_message       {e}
| e = declaration            {e}
| e = variable              {e}
| e = ocaml_call            {e}
| e = replace {e}

replace:
| e = expr; UPPER_ID; LBRACKET; meth; RBRACKET 
{e}


ocaml_call:
| MONEY; id = LOWER_ID LPAREN; arg = expr; RPAREN
  {let call: Ast.ocaml_call = {
    call_id = None;
    call_location = $startpos;
    fn_name = id;
    fn_arg = arg;
  } in Ast.OcamlCall call}

(*FIXME: Make sure we don't have duplicate arguments*)
meth:
| LBRACE; a = arg_list; e = expr; RBRACE
  {{
    location = $startpos;
    args = List.append ["self"] a;
    method_body = e;
    meth_id = None;
    arg_ids = None :: (List.map (fun _ -> None) a);
  }}
| LBRACE; e = expr; RBRACE
  {{
    location = $startpos;
    args = ["self"];
    meth_id = None;
    method_body = e;
    arg_ids = [None]
  }}
(* FIXME: Better syntax *)
arg_list:
| params = nonempty_list(LOWER_ID); PIPE {params}


(* FIXME: Make sure method fields have self stuff *)
(* FIXME: Don't allow {...asdf} by itself? *)
record:
| LBRACKET; r = record_body_first; RBRACKET
  {r}
| c = closure
  {c}
record_body_first:
| id = UPPER_ID; m = meth; rest = record_body_rest ;
{
  let (m: Ast.meth) = m in
  (Ast.RecordExtension
  {
    field = {
      field_id = None;
      field_location = $startpos;
      field_name = id ^ ":" ^ (String.concat ":" m.args);
      field_value = m;
    };
    extension = rest;
    extension_id = None;
  })
}
| {(Ast.EmptyRecord None)}

record_body_rest:
| first = record_body_first {first}
| e = expr {e}


(* FIXME: Make first arg not necessarily require a colon *)

record_message:
| r = expr; id = UPPER_ID; a = record_message_arg*; PERIOD;
  { 
    let var = Ast.Declaration {
      decl_id = None;
      decl_location = $startpos;
      (* FIXME: Change this *)
      decl_lhs = "Self";
      decl_rhs = r;
    } in
    let to_concat = (String.concat ":" (List.map (fun (x: Ast.argument) -> x.name) a)) in
    let rm = Ast.Record_Message {
      rm_id = None;
      rm_location = $startpos;
      rm_receiver = Ast.Variable {var_id = None; var_location = $startpos; var_name = "Self"; origin = None};
      rm_message = id ^ ":self" ^ (if to_concat = "" then "" else ":") ^ to_concat;
      rm_arguments = {
        arg_id = None;
        name = "self"; 
        value = Ast.Variable 
          {var_location = $startpos; 
          var_id = None;
          var_name = "Self"; 
          origin = None}} :: a
    } in 
    Ast.Sequence {
      seq_id = None;
      seq_location = $startpos;
      l = var;
      r = rm;
    }
  }
record_message_arg:
(* URGENT: move to lexer *)
| id = ARG; e = expr; 
  {let r: Ast.argument = {
    arg_id = None;
    name = id; 
    value = e
  } in r}

variant:
| TAG; id = UPPER_ID; e = expr;
  {Ast.emit_variant $startpos ("#" ^ id) e}

variant_meth:
| LBRACE; a = LOWER_ID; PIPE; e = expr; RBRACE
  {{
    location = $startpos;
    args = ["Self"];
    method_body = 
    Ast.Sequence {
      seq_location = $startpos;
      seq_id = None;
      l = Ast.Declaration {
        decl_location = $startpos;
        decl_id = None;
        decl_lhs = a;
        decl_rhs = Ast.Variable {
          var_location = $startpos;
          var_id = None;
          var_name = "Self";
          origin = None;
        }
      };
      r = e;
    }
    ;
    meth_id = None;
    arg_ids = [None]
  }}
| LBRACE; e = expr; RBRACE
  {let (meth:Ast.meth) = {
    location = $startpos;
    args = ["Self"];
    meth_id = None;
    method_body = e;
    arg_ids = [None]
  } in meth}
variant_message:
| r = expr; m = tag_message+; PERIOD;
  {Ast.emit_variant_message r $startpos m}
tag_message:
| n = UPPER_ID; QUESTION; e = variant_meth;
  {{
    field_location = $startpos;
    field_name = "#" ^ n;
    field_id = None;
    field_value = e
  }}


decl_in: e = expr {e} %prec decl_in_prec
declaration:
| id = LOWER_ID; DECLARATION; r = expr; e = decl_in
  {
    Ast.Sequence {
      l = Ast.Declaration {
        decl_location = $startpos;
        decl_lhs = id;
        decl_id = None;
        decl_rhs = r;
      };
      r = e;
      seq_location = $startpos;
      seq_id = None;
      }
  }

variable:
| name = LOWER_ID
  {Ast.Variable {
    var_location = $startpos;
    var_id = None;
    var_name = name;
    origin = None;
  }}

closure:
| LBRACKET; a = arg_list; e = expr; RBRACKET
  {Ast.emit_closure $startpos a e}
| LBRACKET; e = expr; RBRACKET
  {Ast.emit_closure $startpos [] e}

// TODO: Strings(including fancy nested strings like r"asdf"r) and lists
