%token<int> INT
%token<float> FLOAT
%token<string> UPPER_ID LOWER_ID ARG IMPORT STRING
(* End FIXME: *)
%token PIPE DECLARATION PERIOD TAG QUESTION MONEY ABORT EOF
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET

(* LOWEST PRECEDNCE *)
%nonassoc post_decl_expr_prec (* Associativity never matters *)
%right UPPER_ID
(* HIGHEST PRECEDNCE *)

%start<Types.ast> program
%%
program:
| e = expr; EOF {e}

upper_id:
| s = UPPER_ID {(s, $loc)}
lower_id:
| s = LOWER_ID {(s, $loc)}

expr:
(* parenthesized expression *)
| LPAREN; e = expr; RPAREN; {e}
(* prototype extension(of arbitrary prototype) *)
| LBRACKET; extension = expr; fields = prototype_field+; RBRACKET 
  {AstCreate.make_prototype (Some extension) fields $loc}
(* prototype extension(of empty prototype) *)
| LBRACKET; fields = prototype_field+; RBRACKET
  {AstCreate.make_prototype None fields $loc}
(* empty prototype literal *)
| LBRACKET; RBRACKET {AstCreate.make_empty_prototype $loc}
(* closure w/ args *)
| LBRACKET; a = arg_list; e = expr; RBRACKET {AstCreate.make_closure a e $loc}
(* closure w/out args *)
| LBRACKET; e = expr; RBRACKET {AstCreate.make_closure [] e $loc}
(* tagged object *)
| TAG; id = UPPER_ID; e = expr; {AstCreate.make_tagged_object (id, $loc(id)) e $loc}
(* prototype method invoke *)
| r = expr; id = upper_id; arg1 = expr?; a = prototype_invoke_arg*; PERIOD;
| LPAREN; r = expr; id = upper_id; arg1 = expr?; a = prototype_invoke_arg*; RPAREN;
  {AstCreate.make_method_invocation r id arg1 a $loc}
(* pattern match of tagged object *)
| r = expr; m = pattern_match_atom+; PERIOD;
| LPAREN; r = expr; m = pattern_match_atom+; RPAREN
  {AstCreate.make_pattern_match r m $loc}
(* variable declaration *)
| id = lower_id; DECLARATION; rhs = expr; e = post_decl_expr
  {AstCreate.make_declaration id rhs e $loc}
(* variable lookup *)
| name = LOWER_ID {AstCreate.make_variable_lookup name $loc}
(* import *)
| filename = IMPORT {AstCreate.make_import filename $loc}
(* int, float, string literals *)
| i = INT {AstCreate.make_int_literal i $loc}
| f = FLOAT {AstCreate.make_float_literal f $loc}
| s = STRING {AstCreate.make_string_literal s $loc}
(* external call *)
| MONEY; id = lower_id; LPAREN; args = expr*; RPAREN {AstCreate.make_external_call id args $loc}
(* abort *)
| ABORT; PERIOD; {AstCreate.make_abort $loc}

meth:
| LBRACE; a = arg_list; e = expr; RBRACE {AstCreate.make_meth a e $loc}
| LBRACE; e = expr; RBRACE {AstCreate.make_meth [] e $loc}
pattern_match_meth:
| LBRACE; a = lower_id; PIPE; e = expr; RBRACE {AstCreate.make_pattern_match_meth a e $loc}
| LBRACE; e = expr; RBRACE {AstCreate.make_pattern_match_meth ("", $loc) e $loc}

arg_list:
| params = nonempty_list(param); PIPE {params}
param:
| name = LOWER_ID {AstCreate.make_param name $loc}

prototype_field:
| id = upper_id; m = meth; {AstCreate.make_prototype_field id m $loc}
prototype_invoke_arg:
| id = ARG; e = expr; {AstCreate.make_prototype_invoke_arg id e $loc}

pattern_match_atom:
| n = upper_id; QUESTION; e = pattern_match_meth;
  {AstCreate.make_pattern_match_atom n e $loc}

post_decl_expr: e = expr {e} %prec post_decl_expr_prec