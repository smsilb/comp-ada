comp-ada
========

##A front-end compiler for Ada/CS


###Description

Comp Ada translates a subset of Ada/CS into abstract intermediate code. 

It is written in C using lex/yacc, so it use it you'll need lex/yacc installed (and of course gcc).

###Language support

Comp Ada provides support for:

     *Integers and booleans
     *Arrays (simple and nested)
       *Array access syntax: foo(bar)
       *Nested arrays are arrays of arrays
     *Records (simple and nested)
       *Record access syntax: foo.bar
       *Nested records are records of records
       *Mixed array and record types are allowed
     *Procedures (with and without parameters)
       *Recursion should work, but is untested	
     *While loops w/ exit/exit when statements
     *If/elsif/else statements
       *Both conditionals and loops allow arbitrary levels of nesting
     *Exceptions and raise statements
       *Explicit raises only, i.e. no exceptions are raised for divide by zero, invalid array accesses, etc.
     *Simple i/o
       *Read booleans & integers into a variable
       *Write an expression
       *Reading and writing assume a fictional STDIN/STDOUT as sources
     
