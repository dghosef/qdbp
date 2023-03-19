open Codegen_ml

let ocaml_of_qdbp_program in_file =
  let ast = Resolve_imports.parse_file in_file in 
  let ast = Resolve_imports.resolve_imports ast in
  let _, ast = Tag_ast.tag_ast 0 ast in 
  let ast, _ = Name_resolver.name_resolve ast emptymap in 
  let ty, _ = Infer.infer (Infer.Env.empty) 0 ast in
  ast, ty, ocaml_of_ast ast