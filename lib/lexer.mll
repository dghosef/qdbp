{
  open Parser
  open Lexing

  let abort msg =
    prerr_endline msg;
    exit (-1)
  let next_line lexbuf =
    let pos = lexbuf.lex_curr_p in
    lexbuf.lex_curr_p <-
      { pos with pos_bol = lexbuf.lex_curr_pos;
          pos_lnum = pos.pos_lnum + 1
      }
  let str_of_loc (loc: Lexing.position) =
    let file = loc.pos_fname in 
    let line = loc.pos_lnum in
    let col = loc.pos_cnum - loc.pos_bol in
    file ^ ":" ^ string_of_int line ^ ":" ^ string_of_int col
  let lex_error msg loc =
    abort 
      (msg ^ "\n" ^
      "--> " ^ (str_of_loc loc))
}
let digit = ['0' - '9']
let lower = ['a' - 'z']
let upper = ['A' - 'Z']
(* ASCII symbols and all non-ascii characters *)
let symbol = ['!' '%' '&' '*' '~' '-' '+' '=' '\\' '/' '>' '<' '^' '|' '_'] | [^'\x00'-'\x7F']
let filename = (symbol | upper | lower | digit | '.')+
let string_delimiter = (symbol | upper | lower | digit | '.')*
let upper_id = (symbol | upper) (symbol | upper | lower | digit)*
let lower_id = (lower) (symbol | upper | lower | digit)*
(* Other forms of whitespace are disallowed *)
let spaces = [' ' '\t']+
let newline = '\r' | '\n' | "\r\n"

rule token = parse
| spaces { token lexbuf }
| "@" (filename as name) { IMPORT(name) }
| string_delimiter as delimiter '"' { STRING(get_string delimiter lexbuf) }
| '-'? digit+ as s { 
  try (INT (int_of_string (s))) with 
    | Failure _ -> lex_error ("Integer too big: " ^ s) lexbuf.lex_curr_p
}
| '-'? digit+ '.' digit+ as s { 
  try (FLOAT (float_of_string (s))) with 
    | Failure _ -> lex_error ("Float too big: " ^ s) lexbuf.lex_curr_p
}
| ';' [ ^'\n']* { token lexbuf }
| upper_id as s { UPPER_ID (s) }
| lower_id as s ':' { ARG (s) }
| lower_id as s { LOWER_ID (s) }
| newline { next_line lexbuf; token lexbuf}
| "ABORT" { ABORT }
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
| _ as c { lex_error ("Unexpected character: " ^ (String.make 1 c)) lexbuf.lex_curr_p }
and comment level = parse
  | "*)" {
    if level = 0 then token lexbuf
    else comment (level - 1) lexbuf
  }
  | "(*" { comment (level + 1) lexbuf }
  | newline { next_line lexbuf; comment level lexbuf}
  | eof { lex_error "Never finished comment" lexbuf.lex_curr_p }
  | _ { comment level lexbuf }

and get_string delimiter = parse
  | '"' (string_delimiter as delimiter') {
    if delimiter = delimiter' then ""
    else ("\"" ^ delimiter) ^ (get_string delimiter lexbuf)
  }
  | newline { next_line lexbuf; "\n" ^ (get_string delimiter lexbuf)}
  | eof { lex_error "Never finished string literal" lexbuf.lex_curr_p }
  | _ as c { (String.make 1 c) ^ (get_string delimiter lexbuf) }
