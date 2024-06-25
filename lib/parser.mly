%token<string> UPPER_ID LOWER_ID IMPORT STRING INT
%token PIPE PERIOD TAG QUESTION MONEY EOF COLON DOUBLE_COLON
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET

(* LOWEST PRECEDNCE *)
%right UPPER_ID COLON
(* HIGHEST PRECEDNCE *)

%start<AstTypes.ast> program
%%
program:
| e = expr; EOF {
  e
}

upper_id:
| s = UPPER_ID {(s, $loc)}
lower_id:
| s = LOWER_ID {(s, $loc)}

expr:
(* parenthesized expression *)
| LPAREN; e = expr; RPAREN; {e}
(* prototype replacement *)
| LBRACKET; original = expr; fields = prototype_field+; RBRACKET 
  {AstCreate.make_prototype (Some original) fields AstTypes.Replace $loc}
(* prototype extension *)
| LBRACKET; original = expr; DOUBLE_COLON fields = prototype_field+; RBRACKET 
  {AstCreate.make_prototype (Some original) fields AstTypes.Extend $loc}
(* prototype extension(of empty prototype) *)
| LBRACKET; fields = prototype_field+; RBRACKET
  {AstCreate.make_prototype None fields AstTypes.Extend $loc}
(* empty prototype literal *)
| LBRACKET; RBRACKET {AstCreate.make_empty_prototype $loc}
(* closure w/ args *)
| LBRACKET; a = arg_list; e = expr; RBRACKET
  {AstCreate.make_closure a (AstCreate.make_meth_body e $loc) $loc}
(* closure w/out args *)
| LBRACKET; e = expr; RBRACKET 
  {AstCreate.make_closure [] (AstCreate.make_meth_body e $loc) $loc}
(* tagged object *)
| TAG; id = UPPER_ID; e = expr; {AstCreate.make_tagged_object (id, $loc(id)) e $loc}
(* prototype method invoke w/ arg(s) *)
| r = expr; id = upper_id; arg1 = expr; a = prototype_invoke_arg*; PERIOD;
| LPAREN; r = expr; id = upper_id; arg1 = expr; a = prototype_invoke_arg*; RPAREN;
  {AstCreate.make_method_invocation r id (Some arg1) a $loc}
(* prototype method invoke no args *)
| r = expr; id = upper_id; PERIOD;
| LPAREN; r = expr; id = upper_id; RPAREN;
  {AstCreate.make_method_invocation r id None [] $loc}
(* pattern match of tagged object *)
| r = expr; m = pattern_match_atoms; PERIOD;
| LPAREN; r = expr; m = pattern_match_atoms; RPAREN;
  {AstCreate.make_pattern_match r m None $loc}
| r = expr; m = pattern_match_atoms; LBRACE; default = expr; RBRACE PERIOD;
| LPAREN; r = expr; m = pattern_match_atoms; LBRACE; default = expr; RBRACE; RPAREN;
  {AstCreate.make_pattern_match r m (Some (default, $loc(default))) $loc}
(* variable declaration *)
| id = lower_id; COLON; rhs = expr; e = expr
  {AstCreate.make_declaration id rhs e $loc}
(* variable lookup *)
| name = LOWER_ID {AstCreate.make_variable_lookup name $loc}
(* import *)
| filename = IMPORT {AstCreate.make_import filename $loc}
(* int, float, string literals *)
| i = INT {AstCreate.make_int_literal i $loc}
| s = STRING {AstCreate.make_string_literal s $loc}
(* external call *)
| MONEY; id = lower_id; LPAREN; args = expr*; RPAREN
  {AstCreate.make_external_call id args $loc}

meth:
| LBRACE; a = arg_list; e = expr; RBRACE
  {AstCreate.make_meth a (AstCreate.make_meth_body e $loc) $loc}
| LBRACE; e = expr; RBRACE
  {AstCreate.make_meth [] (AstCreate.make_meth_body e $loc) $loc}

pattern_match_meth:
| LBRACE; a = lower_id; PIPE; e = expr; RBRACE
  { AstCreate.make_pattern_match_meth a e $loc }
| LBRACE; e = expr; RBRACE {AstCreate.make_pattern_match_meth ("", $loc) e $loc}

arg_list:
| params = nonempty_list(param); PIPE {params}
param:
| name = LOWER_ID {AstCreate.make_param name $loc}

prototype_field:
| id = upper_id; m = meth;
  { AstTypes.Method (AstCreate.make_prototype_method_field id m $loc) }
| id = upper_id; LBRACE; e = expr; RBRACE; PERIOD;
  { AstTypes.Data (AstCreate.make_prototype_value_field id e $loc) }

prototype_invoke_arg:
| id = LOWER_ID; COLON; e = expr; {AstCreate.make_prototype_invoke_arg id e $loc}

question: n = upper_id; QUESTION; {n}

pattern_match_atom:
| questions = nonempty_list(question); e = pattern_match_meth;
  {AstCreate.make_pattern_match_atom questions e $loc}

pattern_match_atoms:
| atoms = pattern_match_atom+
{ List.flatten atoms }