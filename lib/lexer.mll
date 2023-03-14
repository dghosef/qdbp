(* TODO: Unicode *)
(* TODO: Import mechanism *)
(* TODO: multiline comments including nesting *)
(* TODO: Make sure whitespace/newlines are robust *)
(* TODO: BIGNUMS, NEGATIVES *)
(* FIXME: Make sure EVERY name in every file makes sense *)
(* FIXME: Error handling *)
(* FIXME: Figure out what to do about syntax of assigns in records and variables *)
(* FIXME: Return error *)
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
(*FIXME: Allow ' in variable names?*)
let digit = ['0' - '9']
let lower = ['a' - 'z']
let upper = ['A' - 'Z']
let symbol = ['!' '%' '&' '*' '~' '-' '+' '=' '\\' '/' '>' '<']
let underscore = '_'
let upper_id = (symbol | upper | underscore) (symbol | upper | lower | digit | underscore)*
let lower_id = (lower) (upper | lower | digit | underscore)*
let white = [' ' '\t']+
let newline = '\r' | '\n' | "\r\n"

rule token = parse
| white
    { token lexbuf }
| newline
    { next_line lexbuf; token lexbuf}
| "//" [ ^ '\n']*
    { token lexbuf }
| '.'
    { PERIOD }
| ":="
    { DECLARATION }
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
| '#'
    { TAG }
| '?'
    { QUESTION }
| '$'
    { MONEY }
| upper_id as s
    { UPPER_ID (s) }
| lower_id as s ':'
    { ARG (s) }
| lower_id as s
    { LOWER_ID (s) }
| eof
    { EOF }
| _
    { raise (Error (Printf.sprintf "At line %d: unexpected character.\n" (lexbuf.lex_curr_p.pos_lnum))) }