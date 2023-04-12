(* FIXME: Money, Tag should include the name *)
{
  open Parser
  open Lexing

  let next_line lexbuf =
    let pos = lexbuf.lex_curr_p in
    lexbuf.lex_curr_p <-
      { pos with pos_bol = lexbuf.lex_curr_pos;
          pos_lnum = pos.pos_lnum + 1
      }
  exception LexerError of (string * position)
}
let digit = ['0' - '9']
let lower = ['a' - 'z']
let upper = ['A' - 'Z']
(* ASCII symbols and all non-ascii characters *)
let symbol = ['!' '%' '&' '*' '~' '-' '+' '=' '\\' '/' '>' '<' '^'] | [^'\x00'-'\x7F']
let filename = ('_' | upper | lower | digit | '.')+
let string_delimiter = (upper | lower | digit)*
let upper_id = (symbol | upper) (symbol | upper | lower | digit | '_')*
let lower_id = (lower | '_') (upper | lower | digit | '_' | '\'')*
(* Other forms of whitespace are disallowed *)
let spaces = [' ' '\t']+
let newline = '\r' | '\n' | "\r\n"

rule token = parse
| spaces { token lexbuf }
| "@" (filename as name) { IMPORT(name) }
| string_delimiter as delimiter '"' { STRING(get_string delimiter lexbuf) }
| '-'? digit+ as s { 
  try (INT (int_of_string (s))) with 
    | Failure _ -> raise (LexerError (("Integer too big: " ^ s), lexbuf.lex_curr_p))
}
| '-'? digit+ '.' digit+ as s { 
  try (FLOAT (float_of_string (s))) with 
    | Failure _ -> raise (LexerError ("Float too big: " ^ s, lexbuf.lex_curr_p))
}
| ';' [ ^'\n']* { token lexbuf }
| "ABORT" { ABORT }
| upper_id as s { UPPER_ID (s) }
| lower_id as s ':' { ARG (s) }
| lower_id as s { LOWER_ID (s) }
| newline { next_line lexbuf; token lexbuf}
| "(*" { comment 0 lexbuf }
| '.' { PERIOD }
| ":=" { DECLARATION }
| '|' { PIPE }
| '[' { LBRACE }
| ']' { RBRACE }
| '(' { LPAREN }
| ')' { RPAREN }
| '{' { LBRACKET }
| '}' { RBRACKET }
| '#' { TAG }
| '?' { QUESTION }
| '$' { MONEY }
| eof { EOF }
| _ as c { raise
  (LexerError ("Unexpected character: " ^ (String.make 1 c),
    lexbuf.lex_curr_p ))}
and comment level = parse
  | "*)" {
    if level = 0 then token lexbuf
    else comment (level - 1) lexbuf
  }
  | "(*" { comment (level + 1) lexbuf }
  | newline { next_line lexbuf; comment level lexbuf}
  | eof { raise (LexerError ("Never finished comment", lexbuf.lex_curr_p)) }
  | _ { comment level lexbuf }

and get_string delimiter = parse
  | '"' (string_delimiter as delimiter') {
    if delimiter = delimiter' then ""
    else ("\"" ^ delimiter) ^ (get_string delimiter lexbuf)
  }
  | newline { next_line lexbuf; "\n" ^ (get_string delimiter lexbuf)}
  | eof { raise (LexerError ("Never finished string literal", lexbuf.lex_curr_p))}
  | _ as c { (String.make 1 c) ^ (get_string delimiter lexbuf) }
