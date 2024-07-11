let () =
  Qdbp.UnionFind.run_tests ();
  Qdbp.Driver.compile (Qdbp.Driver.args ());
  prerr_newline ();