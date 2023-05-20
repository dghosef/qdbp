module StringMap = Map.Make(String)

let add_variable varnames name =
  let id = Oo.id (object end) in
  StringMap.add name (id) varnames

let varname varnames name = 
  StringMap.find name varnames

let names_to_ints ast =
  (* FIXME: Make functional *)
  let label_map = Hashtbl.create 10 in
  let labels_added = Hashtbl.create 10 in
  for i = 0 to 1000 do
    Hashtbl.add labels_added (Int64.of_int i) ()
  done;
  (* MUST keep in sync with int_proto.h and infer.ml *)
  (* Must be less than 1000 *)
  Hashtbl.add label_map "Val:this" (Int64.of_int 0);
  Hashtbl.add label_map "Print:this" (Int64.of_int 1);
  Hashtbl.add label_map "+:this:that" (Int64.of_int 2);
  Hashtbl.add label_map "-:this:that" (Int64.of_int 3);
  Hashtbl.add label_map "*:this:that" (Int64.of_int 4);
  Hashtbl.add label_map "/:this:that" (Int64.of_int 5);
  Hashtbl.add label_map "%:this:that" (Int64.of_int 6);
  Hashtbl.add label_map "=:this:that" (Int64.of_int 7);
  Hashtbl.add label_map "!=:this:that" (Int64.of_int 8);
  Hashtbl.add label_map "<:this:that" (Int64.of_int 9);
  Hashtbl.add label_map ">:this:that" (Int64.of_int 10);
  Hashtbl.add label_map "<=:this:that" (Int64.of_int 11);
  Hashtbl.add label_map ">=:this:that" (Int64.of_int 12);
  (* Save the first 1000 labels for reserved labels*)
  let get_label name =
    match Hashtbl.find_opt label_map name with
    | Some label -> label
    | None ->
      let label = ref (Random.bits64 ()) in
      while (Hashtbl.mem labels_added !label) do
        label := Random.bits64 ()
      done;
      Hashtbl.add label_map name !label;
      Hashtbl.add labels_added !label ();
      !label in

  let tag_map = Hashtbl.create 10 in
  (* Save the first 100 for reserved tags *)
  let cur_tag = ref 100 in
  let get_tag name =
    match Hashtbl.find_opt tag_map name with
    | Some tag -> tag
    | None ->
      let tag = !cur_tag in
      cur_tag := tag + 1;
      Hashtbl.add tag_map name tag;
      tag in
  (* IMPORTANT: Must keep this updated with 
      qdbp_true() and qdbp_false() in runtime.h
  *)


  let _ = Hashtbl.add tag_map "False" 20 in
  let _ = Hashtbl.add tag_map "True" 21 in
  (* FIXME: Make this purely functional *)
  let rec names_to_ints varnames ast =
    match ast with
    | `EmptyPrototype loc ->
      `EmptyPrototype loc
    | `PrototypeCopy
        (ext, ((name, labelLoc), (args, body, methLoc), fieldLoc), loc, op) -> 
      let ext = names_to_ints varnames ext in
      let arg_names = List.map (fun (name, _) -> name) args in
      let varnames = List.fold_left add_variable varnames arg_names in
      let body = names_to_ints varnames body in
      let args = List.map (fun (name, loc) -> (varname varnames name, loc)) args in
      `PrototypeCopy
        (ext, (((get_label name), labelLoc), (args, body, methLoc), fieldLoc), loc, op)
    | `TaggedObject ((tag, tagLoc), value, loc) -> 
      let value = names_to_ints varnames value in
      `TaggedObject (((get_tag tag), tagLoc), value, loc)
    | `MethodInvocation (receiver, (name, labelLoc), args, loc) ->
      let args = List.map (fun (name, arg, loc) -> name, names_to_ints varnames arg, loc) args in
      let receiver = names_to_ints varnames receiver in
      `MethodInvocation (receiver, ((get_label name), labelLoc), args, loc)
    | `PatternMatch (receiver, cases, loc) -> 
      let receiver = names_to_ints varnames receiver in
      let cases = List.map (
          fun ((name, nameLoc), ((arg, argLoc), body, patternLoc), loc) ->
            let varnames = add_variable varnames arg in
            let body = names_to_ints varnames body in
            (((get_tag name), nameLoc), ((varname varnames arg, argLoc), body, patternLoc), loc)
        ) cases in
      let receiver_id = Oo.id (object end) in
      `Declaration ((receiver_id, loc), receiver, 
                    `PatternMatch (receiver_id, cases, loc), loc)
    | `Declaration ((name, nameLoc), rhs, body, loc) ->
      let rhs = names_to_ints varnames rhs in
      let varnames = add_variable varnames name in
      let body = names_to_ints varnames body in
      `Declaration (((varname varnames name), nameLoc), rhs, body, loc)
    | `VariableLookup (name, loc) ->
      let name = varname varnames name in
      `VariableLookup (name, loc)
    | `ExternalCall ((name, nameLoc), args, loc) ->
      let args = List.map (names_to_ints varnames) args in
      `ExternalCall ((name, nameLoc), args, loc)
    | `IntLiteral (i, loc) ->
      `IntLiteral (i, loc)
    | `IntProto (i, loc) -> `IntProto (i, loc)
    | `FloatLiteral (f, loc) -> 
      `FloatLiteral (f, loc)
    | `StringLiteral (s, loc) ->
      `StringLiteral (s, loc)
    | `Abort loc ->
      `Abort loc
  in 
  let ast = names_to_ints StringMap.empty ast in
  assert (!cur_tag < 4_000_000_000);
  assert (Hashtbl.length tag_map < 4_000_000_000);
  ast