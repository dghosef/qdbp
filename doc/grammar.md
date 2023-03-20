```ocaml
(* LOWEST PRECEDNCE *)
%nonassoc decl_in_prec
%right UPPER_ID
(* HIGHEST PRECEDNCE *)
program:
  | expr; EOF
expr:
  | LPAREN; expr; RPAREN;
  | record
  | variant
  | record_message
  | variant_message
  | declaration
  | variable
  | ocaml_call
  | import_expr
  | int_literal
  | empty_list
  | abort
  | string
meth:
  | LBRACE; arg_list; expr; RBRACE
  | LBRACE; expr; RBRACE
arg_list:
  | nonempty_list(LOWER_ID); PIPE
record:
  | LBRACKET; record_body; RBRACKET
  | LBRACKET; RBRACKET
  | closure
record_field:
  | UPPER_ID; meth; 
record_body:
  | expr; record_field+; 
  | record_field+; 
record_message:
  | expr; UPPER_ID; expr?; record_message_arg*; PERIOD;
  | LPAREN; expr; UPPER_ID; expr?; record_message_arg*; RPAREN;
record_message_arg:
  | ARG; expr; 
variant:
  | TAG; UPPER_ID; expr;

variant_meth:
  | LBRACE; LOWER_ID; PIPE; expr; RBRACE
  | LBRACE; expr; RBRACE
variant_message:
  | expr; tag_message+; PERIOD;
  | LPAREN; expr; tag_message+; RPAREN
tag_message:
  | UPPER_ID; QUESTION; variant_meth;
decl_in:
  | expr %prec decl_in_prec
declaration:
  | LOWER_ID; DECLARATION; expr; decl_in
variable:
  | LOWER_ID
closure:
  | LBRACKET; arg_list; expr; RBRACKET
  | LBRACKET; expr; RBRACKET
import_expr:
  | IMPORT
int_literal:
  | INT
empty_list:
  | LBRACE; RBRACE
abort:
  | ABORT; PERIOD
string:
  | STRING
ocaml_call:
  | MONEY; LOWER_ID LPAREN; expr*; RPAREN
```