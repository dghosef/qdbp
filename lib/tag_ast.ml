let rec tag_ast id (* id to set current node to *) ast =
  match ast with 
  | Ast.RecordExtension re ->
    (* Comment this *)
    let re = { re with extension_id = Some id } in 
    let id = id + 1 in 
    let field = re.field in 
    let field = {field with field_id = Some id} in
    let id = id + 1 in
    let meth = field.field_value in 
    let meth = {meth with meth_id = Some id} in
    let id = id + 1 in
    let id, arg_ids = List.fold_left_map (fun id _ -> (id + 1, Some id)) id meth.args in
    let id, body = tag_ast id meth.method_body in
    let meth = {meth with arg_ids = arg_ids; method_body = body; } in 
    let field = {field with field_value = meth} in
    let id, extension = tag_ast id re.extension in
    let re = {re with field = field; extension = extension} in
    id, (Ast.RecordExtension re)
  | Ast.EmptyRecord _ -> id + 1, (Ast.EmptyRecord (Some id))
  | Ast.Record_Message rm ->
    let rm = { rm with rm_id = Some id } in
    let id = id + 1 in
    let args = rm.rm_arguments in
    let id, (args: Ast.argument list) = List.fold_left_map 
        (fun id (arg: Ast.argument) -> 
          (id + 1, {arg with arg_id = Some id})) id args 
        in
    let id, args = List.fold_left_map 
        (fun id (arg: Ast.argument) -> 
           let id, value = tag_ast id arg.value in 
           id, {arg with value= value}) id args 
    in
    let id, receiver = tag_ast id rm.rm_receiver in
    id, Ast.Record_Message {rm with rm_arguments = args; rm_receiver = receiver}
  | Ast.Sequence s ->
    let s = { s with seq_id = Some id } in
    let id = id + 1 in
    let id, l = tag_ast id s.l in
    let id, r = tag_ast id s.r in
    id, Ast.Sequence {s with l = l; r = r}
  | Ast.Declaration d ->
    let d = { d with decl_id = Some id } in
    let id = id + 1 in
    let id, rhs = tag_ast id d.decl_rhs in
    id, Ast.Declaration {d with decl_rhs = rhs}
  | Ast.Variable v ->
    let v = { v with var_id = Some id } in
    id + 1, Ast.Variable v
  | Ast.OcamlCall oc ->
    let oc = { oc with call_id = Some id } in
    let id = id + 1 in
    let id, arg = tag_ast id oc.fn_arg in
    id, Ast.OcamlCall {oc with fn_arg = arg}