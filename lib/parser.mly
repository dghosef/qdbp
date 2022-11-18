%token <int> INT
%token <string> ID MESSAGE_ID MESSAGE_ARG TAG TAG_MESSAGE
%token PIPE ASSIGNMENT PERIOD SEMICOLON COMMA
%token LPAREN RPAREN
%token LBRACE RBRACE
%token LBRACKET RBRACKET
%token EOF

%nonassoc low
%nonassoc medium
%left SEMICOLON TAG_MESSAGE MESSAGE_ID


%start <Ast.expr> program

%%
(* TODO: More specific parse messages *)
(* TODO: Add start and end positions to tokens and productions *)

program:
| e = expr; EOF {e}

expr:
| LPAREN; e = expr; RPAREN; {e}
| e = record                {e}
| e = variant               {e}
| e = closure               {e}
| e = record_message        {e}
| e = simple_record_message {e}
| e = variant_message       {e}
| e = sequence              {e}
| e = assignment            {e}
| e = variable              {e}
| e = int                   {e}

record:
| LBRACKET; l = record_field*; RBRACKET
  {Ast.Record {location = $startpos; fields = l}}
record_field:
| LPAREN; e1 = expr; RPAREN;
  {Ast.Destructure {e = e1}}
| id1 = ID; id2 = MESSAGE_ID; ASSIGNMENT; e = expr;
  {Ast.Field {self = id1; field_name = id2; field_value = e}}

variant:
| id = TAG; e = expr;
  {Ast.Variant {location = $startpos; tag = id; value = e}} %prec medium

closure:
| LBRACE; a = closure_arg_list; e = expr; RBRACE
  {Ast.Closure {location = $startpos; args = a; body = e}}
closure_arg_list:
| params = ID*; PIPE {params}

record_message:
| r = expr; id = MESSAGE_ID; a = record_message_arg+; PERIOD;
  {Ast.Record_Message {location = $startpos; receiver = r; message = id; arguments = a}}
record_message_arg:
| id = MESSAGE_ARG; e = expr; {Ast.emit_record_message_arg id e}

simple_record_message:
| r = expr; id = MESSAGE_ID; PERIOD
  {Ast.Record_Message {location = $startpos; receiver = r; message = id; arguments = []}}

variant_message:
| r = expr; m = separated_nonempty_list(COMMA, tag_message); PERIOD;
  {Ast.Variant_Message {location = $startpos; receiver = r; message = m}}
tag_message:
| t = TAG_MESSAGE; e = expr;
  {Ast.emit_variant_message t e}

sequence:
| e1 = expr; SEMICOLON; e2 = expr 
  {Ast.Sequence{location = $startpos; l = e1; r = e2}} %prec medium

assignment:
| id = ID; ASSIGNMENT; e = expr;
  {Ast.Assignment {location = $startpos; lhs = id; rhs = e}} %prec low

variable:
| id = ID
  {Ast.Variable {location = $startpos; name=id}}

int:
| i = INT
  {Ast.Int {location = $startpos; value = i}}
