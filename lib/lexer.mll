{
  open Parser
  open Lexing
  exception Error of string
  let next_line lexbuf =
    let pos = lexbuf.lex_curr_p in
    lexbuf.lex_curr_p <-
      { pos with pos_bol = lexbuf.lex_curr_pos;
                pos_lnum = pos.pos_lnum + 1
      }
}

let digit = ['0' - '9']
let id = ['a'-'z'] ['A'-'Z' 'a'-'z' '0'-'9' '_']*
let message_name = ['_' 'A'-'Z'] ['A'-'Z' 'a'-'z' '0'-'9' '_']*
let variant_message_name = '#' ['A'-'Z' 'a'-'z' '0'-'9' '_']+
let white = [' ' '\t']+
let newline = '\r' | '\n' | "\r\n"

rule token = parse
| white (* also ignore newlines, not only whitespace and tabs *)
    { token lexbuf }
| newline
    { next_line lexbuf; token lexbuf}
(* TODO: multiline comments *)
(* TODO: Rename tokens to be better named *)
| "//" [ ^ '\n']*
    { token lexbuf }
| '.'
    { PERIOD }
| ','
    { COMMA }
| ';'
    { SEMICOLON }
| ":="
    { ASSIGNMENT }
| '|'
    { PIPE }
| '['
    { LBRACE }
| ']'
    { RBRACE }
| '('
    { LPAREN }
| ')'
    { RPAREN }
| '{'
    { LBRACKET }
| '}'
    { RBRACKET }
| variant_message_name '?' as s
    { TAG_MESSAGE (s) }
| variant_message_name as s
    { TAG (s) }
| id as s ':'
    { MESSAGE_ARG (s) }
(* TODO: BIGNUMS, NEGATIVES *)
| digit+ as i
    { INT (int_of_string i) }
| message_name as s
    { MESSAGE_ID (s) }
| id as s
    { ID (s) }
| eof
    { EOF }
| _
    { raise (Error (Printf.sprintf "At offset %d: unexpected character.\n" (Lexing.lexeme_start lexbuf))) }