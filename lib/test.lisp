(:= 
    (x) 
    (Sequence 
        (Record 
            (
                (self) 
                (Y) 
                (Int 3))
            (
                (self) 
                (Z) 
                (Variant 
                    (#tag) 
                    (Closure 
                        (x y )
                        (Sequence 
                            (Record Message 
                                (Var x) 
                                (Message)) 
                            (Sequence 
                                (Record Message 
                                    (Var x) 
                                    (Message)
                                    (arg1 
                                        (Int 3))
                                    (arg2 
                                        (Int 4))) 
                                (Sequence 
                                    (Variant Message 
                                        (Var x)
                                        (#True? 
                                            (Variant Message 
                                                (Var y)
                                                (#True? 
                                                    (Int 4))
                                                (#False? 
                                                    (Int 3))))
                                        (#False? 
                                            (Int 4))) 
                                    (:= 
                                        (z) 
                                        (:= 
                                            (message) 
                                            (Sequence 
                                                (Int 5) 
                                                (Int 4))))))))))
            (
                (self) 
                (Asdf) 
                (Int 35))
            (destructure 
                (Var object1))) 
        (:= 
            (y) 
            (Int 15))))