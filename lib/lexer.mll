(* TODO: Add Shebang Support *)
(* TODO: Figure out how to have shebang allowed ontop *)
(* TODO: Unicode *)
(* TODO: Import mechanism *)
(* TODO: multiline comments including nesting *)
(* TODO: Make sure whitespace/newlines are robust *)
(* TODO: BIGNUMS, NEGATIVES *)
(* FIXME: Make sure EVERY name in every file makes sense *)
(* FIXME: Error handling *)
(* FIXME: Figure out what to do about syntax of assigns in records and variables *)
(* FIXME: Return error *)
(* FIXME: Add hex/binary int literals *)
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
let filename = ['.' '/' 'a' - 'z' 'A' - 'Z' '_']+
let upper_id = (symbol | upper | underscore) (symbol | upper | lower | digit | underscore)*
let lower_id = (lower) (upper | lower | digit | underscore | '\'')*
let white = [' ' '\t']+
let newline = '\r' | '\n' | "\r\n"

rule token = parse
| white
    { token lexbuf }
| newline
    { next_line lexbuf; token lexbuf}
| "ABORT"
    { ABORT }
| ';' [ ^ '\n']*
    { token lexbuf }
| "(*" { comments 0 lexbuf }
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
| "@" (filename as name)
    { IMPORT(name) }
(* FIXME: Allow asdf"text"asdf *)
| '"' ([ ^ '"']* as text) '"'
    { STRING(text) }
(* FIXME: Error checking for bigints *)
(* FIXME: Ambiguities with messages? *)
| '-'? digit+ as s
    { INT (int_of_string (s)) }
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
and comments level = parse
    | "*)" {
        if level = 0 then token lexbuf
        else comments (level - 1) lexbuf
    }
    | "(*" {
        comments (level + 1) lexbuf
    }
    | _ { comments level lexbuf }
    (* FIXME: Better message *)
    | eof 
        { raise (Error (Printf.sprintf "At line %d: eof in multiline comment.\n" (lexbuf.lex_curr_p.pos_lnum))) }
