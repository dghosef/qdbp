module FileMap = Map.Make(String)
let read_whole_file filename =
  let ch = open_in filename in
  let s = really_input_string ch (in_channel_length ch) in
  close_in ch;
  s

let parse_file files filename =
  if (Filename.is_relative filename) then
    Error.internal_error 
    ("parse_file: " ^ filename ^ " is relative")
  else
    let src = read_whole_file filename in
    let lexbuf = Lexing.from_string src in
    let files = FileMap.add filename src files in
    Lexing.set_filename lexbuf filename;
    try
      files, Parser.program Lexer.token lexbuf
    with Parser.Error ->
      let message = "Syntax error" in
      Error.compile_error message (lexbuf.lex_curr_p, lexbuf.lex_curr_p) files
