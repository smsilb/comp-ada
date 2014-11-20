comp-ada
========

#A front-end compiler for Ada/CS


##Description

Comp Ada translates a subset of Ada/CS into abstract intermediate code. 

It is written in C using lex/yacc, so it use it you'll need lex/yacc installed (and of course gcc).

##Language support

Comp Ada provides support for:

     -integers, and all thereby derived types (i.e. booleans)
     -arrays (simple and complex)
     -records (simple and complex)
     -ranges
     -procedures (with and without parameters)
     -while loops w/ exit/exit when statements
     -if/elsif/else statements
     -exceptions and raise statements
     -simple i/o
     
