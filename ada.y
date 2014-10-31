%{
    extern int yydebug;
    yydebug = 1;
    #include <stdio.h>
    #include <string.h>
    #include "lib/treeStack/binTree.h"
    #include "lib/idNodeList/idList.h"

    symbol getEmptySymbol();
    symbol ref;
 %}


%token IS BEG END PROCEDURE ID NUMBER TYPE ARRAY RAISE OTHERS
%token RECORD IN OUT RANGE CONSTANT ASSIGN EXCEPTION NULLWORD LOOP IF
%token THEN ELSEIF ELSE EXIT WHEN AND OR EQ NEQ LT GT GTE LTE TICK
%token NOT EXP ARROW OF DOTDOT ENDIF ENDREC ENDLOOP EXITWHEN
%type <integer> NUMBER constant
%type <var> ID type_name
%type <idList> id_list
%type <mode> mode
%type <node> comp_list parameters
%union {
    int integer;
    char *var, *mode;
    struct idnode* idList;
    struct node* node;
}
%%

program  : proc_head IS decl_part proc_body 
{printf("OK\n");}
;

begin : BEG 
{}
;

proc_spec : proc_head '(' parameters ')' IS decl_part proc_body 
{}
| proc_head IS decl_part proc_body 
{}
;

proc_head : PROCEDURE ID 
{
    push($2);
}

proc_body : begin stmt_list excpt_part END ';' 
{
    pop();
}

parameters :  id_list ':' mode type_name ';' parameters
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode($4);
    ref.kind  = mallocAndCpy("parm");
    ref.mode  = mallocAndCpy($3);
    ref.next = $6;
    $$ = addIdsToStack($1, ref);
    printf("line %d: ", lineno);
    print($1);
    printf(" - %s parameter of type %s\n", ref.mode, ref.pType->data.name);
}
| id_list ':' mode type_name 
{
    ref = getEmptySymbol();
    ref.pType = getTypeNode($4);
    ref.kind  = mallocAndCpy("parm");
    ref.mode  = mallocAndCpy($3);
    $$ = addIdsToStack($1, ref);
    printf("line %d: ", lineno);
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
 strcpy($$, "i");}
;

id_list : ID ',' id_list
{
 currentList = addId(currentList, $1);
 $$ = currentList;}
| ID 
{ 
 currentList = insertIdList();
 currentList = addId(currentList, $1);
 $$ = currentList;}
;

type_name : ID 
{
    $$ = $1;
}
;

decl_part : decl_list proc_part
{}
| proc_part
{}
;

decl_list : decl_list decl 
{}
| decl 
{}
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
;
proc_part : proc_part proc_spec 
{}
| {}
;

array_dec : TYPE ID IS ARRAY '(' constant DOTDOT constant ')' OF type_name ';'
{
    ref = getEmptySymbol();
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("array");
    ref.cType = getTypeNode($11);
    ref.lower = $6;
    ref.upper = $8;
    addSymbol(ref);
    printf("line %d: array type %s of %s from %d to %d\n", lineno, $2, $11, $6, $8);
}
;

record_dec : TYPE ID IS RECORD comp_list ENDREC ';' 
{}
;

comp_list : var_dec comp_list 
{}
| var_dec 
{
    
}
;

type_dec : TYPE ID IS RANGE constant DOTDOT constant ';' 
{
    symbol ref;
    ref.name = mallocAndCpy($2);
    ref.kind = mallocAndCpy("type");
    ref.pType = getTypeNode("integer");
    ref.lower = $5;
    ref.upper = $7;
    addSymbol(ref);
    printf("line %d: type %s defined\n", lineno, $2);
}

;

var_dec : id_list ':' type_name ';' 
{
    symbol ref;
    ref.pType = getTypeNode($3);
    ref.kind = mallocAndCpy("variable");
    node *returned = addIdsToStack($1, ref);
    printf("line %d: ", lineno);
    print(currentList);
    printf(" of type %s\n", returned->data.pType->data.name);
}
| id_list ':' EXCEPTION ';' 
{}
;

const_dec : id_list ':' CONSTANT ASSIGN const_expr ';' 
{}
;

constant : ID 
{}
| NUMBER 
{}
;

const_expr : expression 
{}
;

stmt_list : stmt_list statement 
{}
| statement 
{}
;

statement : assign_stmt 
{}
| call_stmt 
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

assign_stmt : ID ASSIGN expression ';' 
{}
;

call_stmt : ID '(' expr_list ')' ';' 
{}
| ID ';' 
{}
;

expr_list : expr_list ',' expression 
{}
| expression 
{}
;

loop_stmt : loop loop_stmt_list ENDLOOP ';' 
{}
;

loop : LOOP 
{}
;

loop_stmt_list : statement loop_stmt_list 
{}
| exit 
{}
;              

exit : EXIT ';' 
{}
| EXITWHEN condition ';' 
{}
;

if_stmt : if_head elsif else_clause ENDIF ';' 
{}
;

if_head : IF condition THEN stmt_list 
{}
;

elsif : elsif ELSEIF condition THEN stmt_list 
{}
| 
{}
;

else_clause  : ELSE stmt_list 
{}
| 
{}
;

raise_stmt : RAISE ID ';' 
{}
;

condition : expression 
{}
;

expression : rel_list 
{}
;

rel_list: rel_list bool_op relation 
{}
| relation 
{}
;

bool_op : AND 
{}
| OR 
{}
;

relation : simple_list 
{}
;

simple_list : simple_list rel_op simple_expr 
{}
| simple_expr 
{}
;

rel_op : EQ 
{}
| NEQ 
{}
| LT 
{}
| GT 
{}
| LTE 
{}
| GTE 
{}
;

simple_expr : '-' term_list 
{}
| term_list 
{}
;

term_list : term_list add_op term 
{}
| term 
{}
;

add_op : '+' 
{}
| '-' 
{}
;

term : factor_list 
{}
;

factor_list : factor_list mult_op factor 
{}
| factor 
{}
;

mult_op : '*' 
{}
| '/' 
{}
;

factor : primary_list 
{}
| NOT primary 
{}
;

primary_list : primary_list EXP primary 
{}
| primary 
{}
;

primary : NUMBER 
{}
| ID 
{}
| '(' expression ')' 
{}
;

excpt_part : EXCEPTION handle_list 
{}
| 
{}
;

handle_list : handle_list WHEN choice_list ARROW stmt_list 
{}
| WHEN choice_list ARROW stmt_list 
{}
;

choice_list : choice_list '|' ID 
{}
| ID 
{}
| OTHERS 
{}

%%

extern int lineno;

idnodeptr currentList;


char* mallocAndCpy(char* string) {
    char* pointer;
    pointer = (char*)malloc(sizeof(string)+1);

    if (strlen(string) > 0) {
        strcpy(pointer, string);
        return pointer;
    }    

    return pointer;
}

node* getTypeNode(char *type) {
    node *typeNode = searchStack(type);

    if (typeNode == NULL) {
        char mes[200] = "Type: '";
        char* s;
        strcat(mes, type);
        strcat(mes, "' is not defined in this context");
        s = (char*)malloc(sizeof(mes)+1);
        strcpy(s, mes);
        yyerror(s);
    }

    return typeNode;
}

node* addIdsToStack(idnodeptr idList, symbol ref) {
    idnodeptr temp = idList;

    while(temp->next != NULL) {
        ref.name = mallocAndCpy(temp->name);
        addSymbol(ref);
        temp = temp->next;
    }

    return search(stack[top].root, idList->name);
}

printNode(node *root){
    if (root->left != NULL) {
        printNode(root->left);
    }
    if (root == NULL) {
        printf("i shouldnt be here\n");
    }

    printf("%s:", root->data.name);
    printf(" %s", root->data.kind);
    if (!strcmp(root->data.kind, "array")) {
        printf(" of components %s from %d to %d\n", root->data.cType->data.name, root->data.lower, root->data.upper);
    } else if (!strcmp(root->data.kind, "type")) {
        printf(" from %d to %d\n", root->data.lower, root->data.upper);
    } else if (!strcmp(root->data.kind, "variable")) {
        printf(" of type %s\n", root->data.pType->data.name);
    } else if (strstr("parm", root->data.kind) != NULL) {
        printf(" %s of type %s\n", root->data.mode, root->data.pType->data.name);

        if (root->data.next != NULL) {
            printf("\tname of next: %s\n", root->data.next->data.name);
            printNode(root->data.next);
        }
    }

    if (root->right != NULL){
        printNode(root->right);
    }
}

symbol getEmptySymbol(){
    symbol sym;
    return sym;
}

outerContext(){
    symbol oc;
    oc.name = mallocAndCpy("integer");
    oc.kind = mallocAndCpy("type");
    addSymbol(oc);
    
    oc.name = mallocAndCpy("boolean");
    oc.kind = mallocAndCpy("type");
    addSymbol(oc);

    oc.name = mallocAndCpy("true");
    oc.kind = mallocAndCpy("constant");
    oc.value = 1;
    addSymbol(oc);
    
    oc.name = mallocAndCpy("false");
    oc.kind = mallocAndCpy("constant");
    oc.value = 0;
    addSymbol(oc);
    
    oc.name = mallocAndCpy("maxint");
    oc.kind = mallocAndCpy("constant");
    oc.value = 12;
    addSymbol(oc);
}

main()
{
    outerContext();
    yyparse();




    /*
      You're currently working on parameters, don't recompile, run ada with 
      Gdecls.ada
      The parameters for Y1 (A, B) are fine when pushing, but are not printing
      correctly. You are looking into why.
      
      Current guess:
         perhaps the next pointers are set up incorrectly.
     */
}
