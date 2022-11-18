
let program = "
// comment
x := 
{
  (x) (y)
  self Y := 3
  self Z := #tag [x y | x Message;
                        x Message arg1: 3 arg2: 4.;
                        x #True? 
                            y 
                              #True?  4,
                              #False? 3.,
                          #False? (4).;
                        z := message := 5;
                        4 ]
  self Asdf := 35
};
y := 15
"
let () = print_endline (Qdbp.Ast_utils.pprint_program program)