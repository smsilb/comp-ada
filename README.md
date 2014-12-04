comp-ada
========

##A front-end compiler for Ada/CS


###Description

Comp Ada translates a subset of Ada/CS into abstract intermediate code. 

It is written in C using lex/yacc, so it use it you'll need lex/yacc installed (and of course gcc).

###Language support

Comp Ada provides support for:

     *integers and booleans
     *arrays (simple and nested)
     *records (simple and nested)
     *mixed records and arrays
     *ranges
     *procedures (with and without parameters)
     *while loops w/ exit/exit when statements
     *if/elsif/else statements
     *exceptions and raise statements
     *simple i/o
     
