{
  open Parser
  open Lexing

  let next_line lexbuf =
    let pos = lexbuf.lex_curr_p in
    lexbuf.lex_curr_p <-
      { pos with pos_bol = lexbuf.lex_curr_pos;
          pos_lnum = pos.pos_lnum + 1
      }
  let prepend s maybe_s =
    match maybe_s with 
    | Ok s' -> Ok (s ^ s')
    | Error e -> Error e
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
| string_delimiter as delimiter '"' {
  match get_string delimiter lexbuf with
    | Ok s -> STRING s
    | Error e -> LexError (`EofInString e)
}
| '-'? digit+ as s { 
  try (INT (int_of_string (s))) with 
    | Failure _ -> LexError (`BadInteger (s, lexbuf.lex_curr_p))
}
| '-'? digit+ '.' digit+ as s { 
  try (FLOAT (float_of_string (s))) with 
    | Failure _ -> LexError (`BadFloat (s, lexbuf.lex_curr_p))
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
| _ as c { LexError (`BadCharacter (c, lexbuf.lex_curr_p)) }
and comment level = parse
  | "*)" {
    if level = 0 then token lexbuf
    else comment (level - 1) lexbuf
  }
  | "(*" { comment (level + 1) lexbuf }
  | newline { next_line lexbuf; comment level lexbuf}
  | eof { LexError (`EofInComment lexbuf.lex_curr_p) }
  | _ { comment level lexbuf }

and get_string delimiter = parse
  | '"' (string_delimiter as delimiter') {
    if delimiter = delimiter' then Ok ""
    else prepend ("\"" ^ delimiter) (get_string delimiter lexbuf)
  }
  | newline { next_line lexbuf; prepend "\n" (get_string delimiter lexbuf)}
  | eof { Error (lexbuf.lex_curr_p) }
  | _ as c { prepend (String.make 1 c) (get_string delimiter lexbuf) }
