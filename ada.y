%{
    //extern int yydebug;
    // yydebug = 1;
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "binTree.h"
#include "idList.h"
#include "functions.h"
#include "codeGen.h"
#include "patch.h"

    symbol getEmptySymbol();
    symbol ref;
    int offset = 4, prevOffset;

    typedef struct name {
        node *sym;//holds the node returned from the symbol table
        memNode *expr;//holds the result of an expression when the
                      //symbol is a procedure
        memNode *var;//holds a memory node when symbol is an array 
        int nestingDepth;//holds the number of indicies seen thus far
        int totalDepth;//total number of nests in given array 
    } name;
    


    %}


%token IS BEG END PROCEDURE ID NUMBER TYPE ARRAY RAISE OTHERS
%token RECORD IN OUT RANGE CONSTANT ASSIGN EXCEPTION NULLWORD LOOP IF
%token THEN ELSEIF ELSE EXIT WHEN AND OR EQ NEQ LT GT GTE LTE TICK
%token NOT EXP ARROW OF DOTDOT ENDIF ENDREC ENDLOOP EXITWHEN
%type <memNode> expression rel_list relation condition optAssign
%type <memNode> simple_list simple_expr term_list expr_list
%type <memNode> term factor_list factor primary_list primary
%type <integer> NUMBER constant mult_op add_op bool_op rel_op
%type <integer> const_expr loop choice_list handle_list
%type <var> ID EXCEPTION type_name
%type <idList> id_list
%type <mode> mode
%type <symbolTablePtr> comp_list parameters var_dec record_head proc_spec
%type <symbolTablePtr> proc_head proc_init proc_body begin
%type <nameNode> name
%union {
    int integer;
    char *var, *mode;
    struct idnode* idList;
    struct node* symbolTablePtr;
    struct memNode* memNode;
    struct name *nameNode;
    //struct stackNode exprList[100];
}
%%

program  : prog_head IS decl_part proc_body 
{
    printf("OK\n");
    emitReturn();

    //add patch for end of code/beginning of main's AR
    fixPatch(0, instCt);

    //add patch for beginning of next AR after main
    fixPatch(1, instCt + $4->data.offset);
}
;

prog_head : PROCEDURE ID
{
    //this is the first thing recognized in the program
    //print out the program prologue and push a scope
    emitPrologue();
    addPatch(6);
    addPatch(0);
    addPatch(1);
    push($2, offset);
    offset = 4;
    tabs++;
}
;

begin : BEG 
{
    //when a begin is recognized, we store the offset and instruction
    //count so that those values can be stored for later use with
    //activation records
    $$ = (node*)malloc(sizeof(node));
    $$->data.offset = offset;
    $$->data.address = instCt;

    //push a scope for exceptions
    pushPS('e');

    //add a patch for the beginning of main program's code
    if (top == 1) {
        fixPatch(6, instCt);
    }
}
;

proc_spec : proc_head IS decl_part proc_body 
{
    //now that we've seen the whole procedure we store the data
    //we'd previously saved, and print the code for a return statement
    if ($1 != NULL) {
        $1->data.offset = $4->data.offset;
        $1->data.address = $4->data.address;

        //if the procedure has parameters, copy out
        //all 'o' and 'io' parameters
        if ($1->data.next != NULL) {
            emitParamCopyOut($1);
        }

        emitReturn();
    }
}
;

proc_head : proc_init '(' parameters ')'
{
    //copy all of the parameter data into a tail hanging off
    //of the procedure node stored in $1
    if ($1 != NULL) {
        //$1->data.next = $3;
        node *temp = $3;
        node *param = (node*)malloc(sizeof(node));
        param->data = temp->data;
        param->data.next = NULL;
        $1->data.next = param;

        while (temp->data.next != NULL) {
            temp = temp->data.next; //get the next node
            
            //allocate space for the next one
            param->data.next = (node*)malloc(sizeof(node));
            param = param->data.next;//point to the new node
            param->data = temp->data;//copy the data
            param->data.reg = NULL;
        }
        param->data.next = NULL;
    } 
    $$ = $1;
}
| proc_init
{}
;

proc_init : PROCEDURE ID
{
    //create a node for the prodecure and add it to the current scope
    //this will allow us to reference its code at a later time
    //then push a new scope for the declarations in the procedure
    ref = getEmptySymbol();
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("proc");
    ref.next = NULL;
    $$ = addSymbol(ref);
    printTabs(tabs);
    push($2, offset);
    offset = 4;
    //printf("storing offset as %d, setting it to 4\n", offset);
    //    prevOffset = offset;
    tabs++;
}
;

proc_body : begin stmt_list excpt_part END ';' 
{
    printTabs(tabs);
    offset = pop();
    tabs--;
    $$ = $1;
}
;
parameters :  id_list ':' mode type_name ';' parameters
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode($4);
    ref.kind  = mallocAndCpy("parm");
    ref.mode  = mallocAndCpy($3);
    ref.size = ref.pType->data.size;
    ref.next = $6;
    $$ = addIdsToStack($1, ref);
    printf("line %i: ", lineno);
    print($1);
    printf(" - %s parameter of type %s\n", ref.mode, ref.pType->data.name);
}
| id_list ':' mode type_name 
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode($4);
    ref.kind  = mallocAndCpy("parm");
    ref.mode  = mallocAndCpy($3);
    ref.size = ref.pType->data.size;
    ref.next = NULL;
    $$ = addIdsToStack($1, ref);
    printf("line %i: ", lineno);
    print($1);
    printf(" - %s parameter of type %s\n", $$->data.mode, $$->data.pType->data.name);
}
;

mode : IN 
{
    $$ = (char*)malloc(2);
    strcpy($$, "i");
}
| OUT 
{
    $$ = (char*)malloc(2);
    strcpy($$, "o");}
| IN OUT 
{
    $$ = (char*)malloc(3);
    strcpy($$, "io");}
| 
{
    $$ = (char*)malloc(2);
    strcpy($$, "i");
}
;

id_list : ID ',' id_list
{
    currentList = addId(currentList, $1);
    $$ = currentList;
}
| ID 
{ 
    currentList = insertIdList();
    currentList = addId(currentList, $1);
    $$ = currentList;
}
;

type_name : ID 
{
    $$ = $1;
}
;

decl_part : decl_list proc_part
{
}
| proc_part
{
}
;

decl_list : decl_list decl 
{
}
| decl 
{
}
;

decl : array_dec 
{}
| record_dec 
{}
| type_dec 
{}
| var_dec 
{}
| const_dec 
{}
| excpt_dec
{}
;
proc_part : proc_part proc_spec 
{
}
| 
{
}
;

array_dec : TYPE ID IS ARRAY '(' constant DOTDOT constant ')' OF type_name ';'
{
    ref = getEmptySymbol();
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("array");
    ref.cType = getTypeNode($11);
    ref.size = ref.cType->data.size * ($8 - $6 + 1);
    ref.lower = $6;
    ref.upper = $8;
    addSymbol(ref);
    printf("line %i: array type %s of %s from %i to %i\n", lineno, $2, $11, $6, $8);
}
;

record_dec : record_head comp_list ENDREC ';' 
{
    node *temp = $2;
    ref = $1->data;
    while (temp != NULL) {
        ref.size += temp->data.size;
        temp = temp->data.next;
    }
    offset = pop();
    tabs--;
    addSymbol(ref);
    temp = searchStack(ref.name);
    temp->data.next = $2;
}
;

record_head : TYPE ID IS RECORD
{
    ref = getEmptySymbol();
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("record");
    ref.size = 0;
    push($2, offset);
    $$ = (node*)malloc(sizeof(node));
    $$->data = ref;
    tabs++;
}
;

comp_list : var_dec comp_list 
{
    $1->data.next = $2;
    $$ = $1;
}
| var_dec 
{
    $1->data.next = NULL;
    $$ = $1;
}
;

type_dec : TYPE ID IS RANGE constant DOTDOT constant ';' 
{
    symbol ref;
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("range");
    ref.pType = getTypeNode("integer");
    ref.size = ref.pType->data.size;
    ref.lower = $5;
    ref.upper = $7;
    addSymbol(ref);
    printf("line %i: type %s defined\n", lineno, $2);
}

;

var_dec : id_list ':' type_name ';' 
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode($3);
    ref.kind = mallocAndCpy("variable");
    ref.size = ref.pType->data.size;
    ref.reg = NULL;
    $$ = addIdsToStack($1, ref);
    if ($$ != NULL) {
        printf("line %i: ", lineno);
        print(currentList);
        printf(" of type %s\n", $$->data.pType->data.name);
    }
}
;

const_dec : id_list ':' CONSTANT ASSIGN const_expr ';' 
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode("integer");
    ref.kind = mallocAndCpy("constant");
    ref.value = $5;
    ref.size = ref.pType->data.size;
    node *returned = addIdsToStack($1, ref);
    if (returned != NULL) {
        printf("line %i: ", lineno);
        print(currentList);
        printf(" constant of value %i\n", returned->data.value);
    }
}
;

excpt_dec : id_list ':' EXCEPTION ';' 
{
    ref = getEmptySymbol();
    ref.pType = NULL;
    ref.kind = mallocAndCpy("exception");
    addIdsToStack($1, ref);
    printf("line %i: ", lineno);
    print(currentList);
    printf(" : exception\n");
}
;

constant : ID 
{
    node *temp = searchStack($1);
    /*if (temp == NULL) {
      char mes[200] = "Var: '";
      char* s;
      strcat(mes, $1);
      strcat(mes, "' is not defined in this context");
      s = (char*)malloc(sizeof(mes)+1);
      strcpy(s, mes);
      yyerror(s);
      }*/
    $$ = temp->data.value;
}
| NUMBER 
{
    $$ = $1;
}
;

const_expr : expression 
{
    if ($1->constant == 0) {
        $$ = $1->value;
    } else {
        yyerror("Variable used in constant expression");
        $$ = 0;
    }
}
;

stmt_list : stmt_list statement 
{}
| statement 
{}
;

statement : call_stmt 
{}
| loop_stmt 
{}
| if_stmt 
{}
| raise_stmt 
{}
| NULLWORD ';'
{}
;

call_stmt : name optAssign ';' 
{
    node *proc = $1->sym;
    memNode *temp = $1->expr;

    if (proc != NULL) {
        if (!strcmp(proc->data.kind, "write_routine")) {
            while(temp != NULL) {
                emitWrite(temp);
                temp = temp->next;
            }
        } else if(!strcmp(proc->data.kind, "read_routine")) {
            while(temp != NULL) {
                if (strcmp(temp->kind, "address")) {
                    //we can assume that if the expression is not an
                    //address, it was used in an operation of some sort
                    //which means that it is not just a variable, which is
                    //the only thing accepted as a parameter in read routines
                    yyerror("expression used in place of variable in read routine");
                    break;
                } else {
                    emitRead(temp);
                }
                temp = temp->next;
            }
        } else if (!strcmp(proc->data.kind, "variable")
                   || !strcmp(proc->data.kind, "parm")) {
            memNode* id;
            id = $1->var;
            id->offset->value -= proc->data.pType->data.lower;
            
            

            emitAssign(id, $2);
            proc->data.reg = NULL;
        } else {
            int baseReg = emitProcCall(proc);

            emitParamCopyIn(proc, temp, baseReg);

            emitProcJump(proc, baseReg);
        }
    }
}
;

name : ID
{
    $$ = (name*)malloc(sizeof(name));
    $$->sym = searchStack($1);
    if (!strcmp($$->sym->data.kind, "variable")) {
        $$->var = emitPrimId($$->sym);
    }
    $$->expr = NULL;
    $$->nestingDepth = 0;
}
| name '(' expr_list ')'
{
    if (!strcmp($1->sym->data.kind, "variable")) {
        if ($3->next == NULL) {
            int i = 0;
            node *temp = $1->sym->data.pType;

            

            /*
              if the symbol is an var, not a procedure, we need to generally
              increase the offset according to whatever 'position' in a 
              single or multi dimensional array this might be.
              In order to do so, we need to multiply the expression by
              the size of this element, stored in its component type
            */
            
            while(temp != NULL) {
                if ($1->totalDepth - i == $1->nestingDepth) {
                    if ($1->var->offset->constant + $3->constant == 0) {
                        $1->var->offset->value += ($3->value  - 1) * temp->data.size;
                    } else {
                        memNode *size = (memNode*)malloc(sizeof(memNode));
                        size->kind = mallocAndCpy("number");
                    
                        //take advantage of size to do necessary subtraction
                        size->value = 1;
                        $3 = emitAddSub($3, size, -1);

                        //multiply the result by the real size
                        size->value = temp->data.size;
                        $3 = emitMulDiv($3, size, 1);
                        $1->var->offset = emitAddSub($1->var->offset, $3, 1);
                    }
                
                    break;
                }
                i++;
                temp = temp->data.cType;
            }
        } else {
            yyerror("Too many expression in array access");
        }
    } else {
        if ($1->expr == NULL) {
            $1->expr = $3;
            $$ = $1;
        } else {
            yyerror("Too many expression lists in procedure call");
        }
    }
}
;

 optAssign : ASSIGN expression
    {
        $$ = $2;
    }
    |
    {
        $$ = NULL;
    }
    ;

 expr_list : expression ',' expr_list
    {
        $1->next = $3;
        $$ = $1;
    }
    | expression 
      {
          $1->next = NULL;
          $$ = $1;
      }
    ;

 loop_stmt : loop loop_stmt_list ENDLOOP ';' 
    {
        emitJump($1);
        popPS(instCt, 'p');
    }
    ;

 loop : LOOP 
    {
        $$ = instCt;
        pushPS('p');
    }
    ;

 loop_stmt_list : statement loop_stmt_list 
    {
    }
    | exit loop_stmt_list
    {
    }
    |
          ;              

 exit : EXIT ';' 
    {
        addLabel(instCt, 'p');

        emitJumpQ();
    }
    | EXITWHEN condition ';' 
    {
        emitJumpTrue($2);
        addLabel(instCt - 1, 'p');
    }
    ;

 if_stmt : if_head elsif else_clause ENDIF ';' 
    {
        popPS(instCt, 'p');
    }
    ;

 if_head : if THEN stmt_list 
                   {
                       addLabel(instCt, 'p');
                       emitJumpQ();
                   }
    ;

    if : IF condition
            {
                pushPS('p');

                //this is kind of an unfortunate case where 
                //we need to reference a line either 2 ahead or 1 behind
                //of the function call. so we subtract 1
                emitJumpFalse($2);
                addLabel(instCt - 1, 'p');
            }
    ;

 elsif : elsif elseif_head THEN stmt_list
    {
        addLabel(instCt, 'p');
        emitJumpQ();
    }
    | 
    {}
    ;

 elseif_head : ELSEIF condition
    {
        //i'm really not sure why these are two higher than
        //they should be.....
        //maybe its because we've already emitted a jump in the previous
        //if/elseif

        //the first pop is for the statements inside the then body,
        //the second is for the branch jump at the beginning
        popPS(instCt - 2, 'p');
        pushPS('p');
        emitJumpFalse($2);
        addLabel(instCt - 1, 'p');
    }
    ;

 else_clause  : else_head stmt_list 
    {
        addLabel(instCt, 'p');
        emitJumpQ();
    }
    | 
    {}
    ;

 else_head : ELSE
    {
        popPS(instCt, 'p');
        pushPS('p');
    }
    ;

 raise_stmt : RAISE ID ';' 
    {
        node *excpt = searchStack($2);

        if (excpt == NULL) {
            yyerror("Undeclared exception raised");
        } else {
            emitRaise(excpt->data.value);
            addLabel(instCt - 1, 'e');
        }
    }


 condition : expression 
    {
        $$ = $1;
    }
    ;

 expression : rel_list 
    {
        $$ = $1;
    }
    ;

 rel_list: rel_list bool_op relation 
    {
        if ($1->constant + $3->constant == 0) {
            $$ = $1;
            $$->value = (1 - $2) * ($1->value && $3->value) + $2 * ($1->value || $3->value);
        } else {
            $$ = emitBooP($1, $3, $2);
        }
    }
    | relation 
      {
          $$ = $1;
      }
    ;

 bool_op : AND 
    {
        $$ = 0;
    }
    | OR 
      {
          $$ = 1;
      }
    ;

 relation : simple_list 
    {
        $$ = $1;
    }
    ;

 simple_list : simple_list rel_op simple_expr 
    {
        if ($1->constant + $3->constant == 0) {
            $$ = $1;
            switch($2){
            case 1:
                $$->value = $1->value == $3->value;
                break;
            case 2:
                $$->value = $1->value != $3->value;
                break;
            case 3:
                $$->value = $1->value < $3->value;
                break;
            case 6:
                $$->value = $1->value <= $3->value;
                break;
            case 5:
                $$->value = $1->value > $3->value;
                break;
            case 4:
                $$->value = $1->value >= $3->value;
                break;
            }
        } else {
            $$ = emitRelOp($1, $3, $2);
        }
    }
    | simple_expr 
      {
          $$ = $1;
      }
    ;

 rel_op : EQ 
    {
        $$ = 1;
    }
    | NEQ 
      {
          $$ = 2;
      }
    | LT 
      {
          $$ = 3;
      }
    | GT 
      {
          $$ = 4;
      }
    | LTE 
      {
          $$ = 5;
      }
    | GTE 
      {
          $$ = 6;
      }
    ;

 simple_expr : '-' term_list 
    {
        if ($2->constant == 0) {
            $$ = $2;
            $$->value = $2->value * -1;
        } else {
            $$ = emitNeg($2);
        }
    }
    | term_list 
      {
          $$ = $1;
      }
    ;

 term_list : term_list add_op term 
    {
        if ($1->constant + $3->constant == 0) {
            //by multiplying the term
            //by the add_op, we can choose
            //between + and -
            $$ = $1;
            $$->value = $1->value + ($2 * $3->value);
        } else {
            $$ = emitAddSub($1, $3, $2);
        }
    }
    | term 
      {
          $$ = $1;
      }
    ;

 add_op : '+' 
    {
        $$ = 1;
    }
    | '-' 
      {
          $$ = -1;
      }
    ;

 term : factor_list 
    {
        $$ = $1;
    }
    ;

 factor_list : factor_list mult_op factor 
    {
        if ($1->constant + $3->constant == 0) {
            //by raising the factor to the power from
            //the mult_op, we can choose between * and /
            $$ = $1;
            $$->value = $1->value * pow($3->value, $2);
        } else {
            if ($2 == 1) {
                $$ = emitMulDiv($1, $3, $2);
            } else {
                $$ = emitMulDiv($1, $3, $2);
            }
        }
    }
    | factor 
      {
          $$ = $1;
      }
    ;

 mult_op : '*' 
    {
        $$ = 1;
    }
    | '/' 
      {
          $$ = -1;
      }
    ;

 factor : primary_list 
    {
        $$ = $1;
    }
    | NOT primary 
    {
        if ($2->constant == 0) {
            $$ = $2;
            $$->value = -1 * $2->value;
        } else {
            $$ = emitNot($2);
        }
    }
    ;

 primary_list : primary_list EXP primary 
    {
        if ($1->constant + $3->constant == 0) {
            $$ = $1;
            $$->value = pow($1->value, $3->value);
        } else {
            $$ = emitExp($1, $3);
        }
 
    
    }
    | primary 
      {
          $$ = $1;
      }
    ;

 primary : NUMBER 
    {
        $$ = emitPrimNum($1);
    }
    | ID 
      {
          node *idPtr = searchStack($1);
          if (idPtr == NULL) {
              char mes[200];
              sprintf(mes, "Var: %s is not defined in this context", $1);
              yyerror(mes);
          } else {
              if (!strcmp(idPtr->data.kind, "constant")) {
                  $$ = emitPrimNum(idPtr->data.value);
              } else {
                  $$ = emitPrimId(idPtr);
              }
          }
      }
    | ID '(' expression  ')'
      {
          node *idPtr = searchStack($1);
          if (idPtr == NULL) {
              char mes[200];
              sprintf(mes, "Var: %s is not defined in this context", $1);
              yyerror(mes);
          } else {
              $$ = emitPrimId(idPtr);
              $$->offset->value -= idPtr->data.pType->data.lower;

              if ($3->constant == 0) {
                  $$->offset->value += $3->value;
              } else {
                  $$->offset = emitAddSub($3, $$->offset, 1);
              }
          } 
      }
    | '(' expression ')' 
      {
          $$ = $2;
      }
    ;

 excpt_part : exception handle_list 
    {
        //pop the jumps to the table at the beginning
        popPS(instCt, 'p');

        jumpTable[0] = instCt + next_exception;

        if ($2 == 0) {
            int i;
            for (i = 1; i <= next_exception; i++) {
                if (jumpTable[i] == 0) {
                    jumpTable[i] = jumpTable[0];
                }
            }
        }

        int i;
        for (i = 1; i <= next_exception; i++) {
            emitJump(jumpTable[i]);
        }

        //pop the jumps after the table after
        popPS(instCt, 'p');
        popPS(instCt, 'e');
    }
    | 
    {
        popPS(instCt, 'e');
    }
    ;

 exception : EXCEPTION
    {
        //push a scope and emit a jump to the regular return line
        //this will be executed when there are no exceptions raised
        pushPS('p');
        addLabel(instCt, 'p');
        emitJumpQ();

        //pop a scope to patch the jump to this line 
        //from the raise statements above
        popPS(instCt, 'e');

        //push a scope for the jumps from exceptions to the return line
        pushPS('e');

        //push a scope for the jump to the table
        pushPS('p');

        //this is the jump to the jump table
        addLabel(instCt, 'p');
        fprintf(fp, "%d: pc := r3, ?\n", instCt++);

   
    
        inExceptionPart = 1;
        handlerDone = 0;

        int i;
        for (i = 1; i < 100; i++) {
            jumpTable[i] = 0;
        }
    }
    ;

 handle_list : handle_list WHEN choice_list ARROW excpt_stmt_list
    {
        //probably need to change this to right recursive and 
        //check whether or not there is an 'others'
        emitHandle();
        addLabel(instCt - 1, 'e');

        $$  = $1 + $3;
    }
    | WHEN choice_list ARROW excpt_stmt_list 
    {
        emitHandle();
        addLabel(instCt - 1, 'e');

        $$ = $2;
    }
    ;

 excpt_stmt_list : excpt_stmt_list statement
        | excpt_stmt_list RAISE ';'
    {
        //reraising the current exception functions by jumping
        //out of the exception part
        addLabel(instCt, 'r');
        emitJumpQ();
    }
    | statement
          ;

 choice_list : choice_list '|' ID 
    {
        if (!handlerDone) {
            node *excpt = searchStack($3);

            if (!strcmp(excpt->data.kind, "exception")) {
                jumpTable[excpt->data.value] = instCt;
            }
        }
        $$ = 0;
    }
    | ID 
      {
          if (!handlerDone) {
              node *excpt = searchStack($1);
        
              if (excpt != NULL) {
                  if (!strcmp(excpt->data.kind, "exception")) {
                      jumpTable[excpt->data.value] = instCt;
                  }
              } else {
                  yyerror("Illegal handler for undeclared exception");
              }
          }
          $$ = 0;
      }
    | OTHERS 
      {
          if (!handlerDone) {
              int i;
              handlerDone = 1;

              for (i = 1; i <= next_exception; i++) {
                  if (jumpTable[i] == 0) {
                      jumpTable[i] = instCt;
                  }
              }
          }
          $$ = 1;
      }


    %%

          extern int lineno;
          extern int error;

          idnodeptr currentList = NULL;

          node* addIdsToStack(idnodeptr idList, symbol ref) {
              idnodeptr temp = idList;
              node* next = ref.next;
              node* prev = NULL;

              while(temp->next != NULL) {
                  ref.name = mallocAndCpy(temp->name);
                  ref.offset = offset;
    
                  if (prev == NULL) {
                      prev = addSymbol(ref);
                  } else {
                      prev->data.next = addSymbol(ref);
                      prev = prev->data.next;
                  }

                  if (prev != NULL) {
                      if (!strcmp(prev->data.kind, "variable")) {
                          offset += prev->data.size;
                      } else if (!strcmp(prev->data.kind, "exception")) {
                          prev->data.value = nextException();
                      } else if (!strcmp(prev->data.kind, "parm")) {
                          //parameters that will be copied out need an additional
                          //space to store the address to copy to
                          if (!strcmp(prev->data.mode, "io")
                              || !strcmp(prev->data.mode, "o")) {
                              prev->data.size++;
                          }

                          offset += prev->data.size;
                      }
                  }

                  temp = temp->next;
              }
    
              if (prev != NULL) {
                  prev->data.next = next;
              } else {
                  return NULL;
              }

              return search(stack[top].root, idList->name);
          }

          main()
          {

              fp = fopen ("joe.sucks", "w+");
              //fp = fopen("out.txt", "w+");
   
              printf("Outer context\n");
              outerContext();
              printNode(stack[0].root);
              yyparse();

              fclose(fp);
    
              unlink("out.txt");

              if (!error) {
                  writePatches("joe.sucks", lineno);
              }

              unlink("joe.sucks");
    
              return(0);
          }
