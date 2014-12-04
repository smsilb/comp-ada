/*
  code generation is all based off of values stored in 'memory nodes'

  'kind' is used to differentiate between numbers, registers, and memory
  addresses
  'base' holds the number of a register. if 'kind' is address, this is 
  presumed to be the base of an activation record
  'value' holds a number, but only in the case where 'kind' is a number
  'constant' is a flag used to tell whether an expression is constant
  i.e. only contains numbers, or not, i.e. contains variables
  'offset' is a pointer to a memory node which contains the offset value
  for use in memory addresses (could be a register or a number)
  'next' is a pointer to a memory node for use in lists of expressions
 */
typedef struct memNode {
    char *kind;
    int base, value, constant;
    struct memNode *offset;
    node *var;
    struct memNode *next;
} memNode;

//integers to count instructions and available registers
int instCt = 0;
int nextReg = 0;


FILE * fp;

char* getOffset(memNode *exp);
memNode* emitPrimId(node *idPtr);
memNode* emitAddSub(memNode *loc1, memNode *loc2, int op);
void emitAssign(memNode *loc1, memNode *loc2);
int isParmIn(node *parm);
int isParmOut(node *proc);
void emitCopyIn(node *sym, memNode *parm, memNode *expr);
void emitCopyOut(node *parm);

char * getOffset(memNode *exp) 
/*
  helper function that stringifies the 'offset' node of a node elsewhere
*/
{
    char *offset;
    if (!strcmp(exp->kind, "register")) {
        offset = (char*)malloc(sizeof(exp->base + 4));
        sprintf(offset, ", r%d", exp->base);
    } else if (!strcmp(exp->kind, "number")) {
        offset = (char*)malloc(sizeof(exp->value) + 3);
        sprintf(offset, ", %d", exp->value);
    }

    return offset;
}

char * getLocationString(memNode* exp) 
/*
  helper function that stringifies the memory location referred to by
  a memory node. this is used later on to simplify the other operations
 */
{
    char *location;
    if (!strcmp(exp->kind, "register")) {
        location = (char*)malloc(sizeof(exp->base) + 2);
        sprintf(location, "r%d", exp->base);
    } else if (!strcmp(exp->kind, "address")) {
        location = (char*)malloc(sizeof(exp->base) + sizeof(getOffset(exp->offset)) + 13);
        if (exp->base == 0) 
            sprintf(location, "contents b%s", getOffset(exp->offset));
        else
            sprintf(location, "contents r%d%s", exp->base, getOffset(exp->offset));
    } else if (!strcmp(exp->kind, "number")) {
        int reg = newReg();
        location = (char*)malloc(sizeof("r") + sizeof(reg));
        sprintf(location, "r%d", reg);
        fprintf(fp, "%d: %s := %d\n", instCt++, location, exp->value);
    }

    return location;
}

int getRegister(memNode *exp) 
/*
  helper function that returns a register where the data at the supplied
  memory node can be accessed. checks whether the node supplied refers to
  a register, a regular number, or a memory address. the latter two are
  put in new registers, the former's register number is simply returned
 */
{
    int reg;

    if (!strcmp(exp->kind, "register")) {
        return exp->base;
    } else if(!strcmp(exp->kind, "number")) {
        reg = newReg();

        fprintf(fp, "%d: r%d := %d\n", instCt++, reg, exp->value);

        return reg;
    } else if(!strcmp(exp->kind, "address")) {
        if (exp->var->data.reg == NULL) {
            reg = newReg();
            char * location = getLocationString(exp);
            
            fprintf(fp, "%d: r%d := %s\n", instCt++, reg, location);
            
            exp->var->data.reg = (memNode*)malloc(sizeof(memNode));
            exp->var->data.reg->kind = mallocAndCpy("register");
            exp->var->data.reg->base = reg;
        
            return reg;
        } else {
            return exp->var->data.reg->base;
        }
    }
}

int getResultReg(memNode *op1, memNode *op2) 
/*
  helper function which checks if two operands are registers.
  if either one is, that register will be used to store the result
  of the operation.
 */
{
    if (!strcmp(op1->kind, "register")) {
        return op1->base;
    } else if (!strcmp(op2->kind, "register")) {
        return op2->base;
    } else {
        return newReg();
    }
}

freeRegisters(int count) 
/*
  helper function that would reduce the register count to free a register
  
  not currently used
 */
{
    nextReg -= count;
}

int newReg() 
/*
  helper function that abstracts the process of assigning new registers
 */
{
    nextReg++;
    return nextReg;
}

memNode* walkBack(node *ptr) 
/*
  helper function that prints the appropriate amount of static link
  walkbacks for variable lookups
 */
{
    memNode* exp = (memNode*)malloc(sizeof(memNode));

    int result = newReg();
    fprintf(fp, "%d: r%d := contents b, 2\n", instCt++, result);
    while (ptr->data.depth > 1) {
        fprintf(fp, "%d: r%d := contents r%d, 2\n", instCt++, result, result);
        ptr->data.depth--;
    }

    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

emitPrologue() 
/*
  prints the beginning of every program

  sets up the first activation record and sets aside 3 registers
  the first two are just for use as constants 1 and 0 in arithmetic
  and boolean operations
  the third holds the exception state used throughout the program
  lastly, prints a jump to the main program's code
 */
{
    fprintf(fp, "%d: b := ?\n", instCt++);
    fprintf(fp, "%d: contents b, 0 := ?\n", instCt++);
    fprintf(fp, "%d: contents b, 1 := 7\n", instCt++);
    fprintf(fp, "%d: r%d := 0\n", instCt++, newReg());
    fprintf(fp, "%d: r%d := 1\n", instCt++, newReg());
    //this one is for exceptions
    fprintf(fp, "%d: r%d := 0\n", instCt++, newReg());
    fprintf(fp, "%d: pc := ?\n", instCt++);
    fprintf(fp, "%d: halt\n", instCt++);
}

emitReturn() 
/*
  prints a vanilla 'return' statement from a procedure
  i.e. jumps to the address stored in the return address slot
 */
{
    int r1 = newReg();
    fprintf(fp, "%d: r%d := contents b, 1\n", instCt++, r1);
    fprintf(fp, "%d: b := contents b, 3\n", instCt++);
    fprintf(fp, "%d: pc := r%d\n", instCt++, r1);
}

int emitProcCall(node *proc) 
/*
  prints the statements to create a new activation record for a 
  called procedure. this includes fixing the next base, dynamic link,
  and static link with any potential walkbacks needed
 */
{
    memNode* exp;
    int r1 = newReg(),r2 = newReg();

    //save current base
    fprintf(fp, "%d: r%d := b\n", instCt++, r1);

    //set new base of AR
    fprintf(fp, "%d: b := contents r%d, 0\n", instCt++, r1);
    
    //set dynamic link
    fprintf(fp, "%d: contents b, 3 := r%d\n", instCt++, r1);

    //set static link
    if (proc->data.depth > 0) {
        while (proc->data.depth > 1) {
            fprintf(fp, "%d: r%d := contents r%d, 2\n", instCt++, r1, r1);
            proc->data.depth--;
        }
        fprintf(fp, "%d: contents b, 2 := contents r%d, 2\n", instCt++, r1);
    } else {
        //static link == dynamic link
        fprintf(fp, "%d: contents b, 2 := r%d\n", instCt++, r1);
    }

    //adjust new base w/ offset of procedure
    fprintf(fp, "%d: r%d := %d\n", instCt++, r2, proc->data.offset);
    fprintf(fp, "%d: contents b, 0 := b + r%d\n", instCt++, r2);

    return r1;
}

int isParmIn(node *parm) 
/*
  helper function which determines whether a parameter
  copies data into a procedure
 */
{
    if (!strcmp(parm->data.mode, "i")
        || !strcmp(parm->data.mode, "io")) {
        return 1;
    }
    
    return 0;
}

int isParmOut(node *parm) 
/*
  helper function which determines whether a paramter 
  copies data out of a procedure
 */
{
    if (!strcmp(parm->data.mode, "o")
        || !strcmp(parm->data.mode, "io")) {
        return 1;
    }
    
    return 0;
}

emitParamCopyIn(node *proc, memNode *expr, int base) 
/*
  used to copy data from the expressions supplied to a procedure
  call to the local parameters which will store the data
  also, sets up 'out' parameters by storing the address to copy
  data to at the end of the procedure
 */
{
    node *temp = proc->data.next;

    while (temp != NULL) {
        if (expr == NULL) {
            yyerror("Too few arguments in procedure call");
            break;
        } else {
            //since all parameters are local to their procedure
            temp->data.depth = 0;
            
            //put the parameter in a memory node
            memNode *parm = emitPrimId(temp);


            if (isParmIn(temp)) {
                //we always need to do some sort of 'walkback'
                //since we're sort of in between scopes,
                //so we need to make sure we will use the previously
                //stored base instead of the current one
                if (expr->base == 0) {
                    //we're sort of cheating here by
                    //just using the most recently allocated register
                    expr->base = base;
                }

                emitCopyIn(temp, parm, expr);

                if (isParmOut(temp)) {
                    //if we also need to copy out to this variable, make sure
                    //that we copy the address to the right spot
                    parm->offset->value += temp->data.size - 1;
                }
            }

            if (isParmOut(temp)) {
                if (strcmp(expr->kind, "address")) {
                    yyerror("expression used in place of variable in procedure call");
                } else {
                    memNode *add = (memNode*)malloc(sizeof(memNode));

                    //since we know that the expression is just a variable, we
                    //can use whatever offset is stored there to get the memory address
                    int offset = getRegister(expr->offset);

                    //rather than changing the entire
                    //way i do addition for this one exception, i manually
                    //do the necessary register manipulation here
                    fprintf(fp, "%d: r%d := r%d + r%d\n", instCt++, offset, offset, base);
                    add->base = nextReg;
                    add->kind = mallocAndCpy("register");
                    emitAssign(parm, add);
                }
            }
            expr = expr->next;
            temp = temp->data.next;
        }
    }

    if (expr != NULL) {
        yyerror("Too many arguments in procedure call");
    }
}

void emitCopyIn(node *sym, memNode *parm, memNode *expr) 
/*
  helper function which differentiates between different type
  of parameters and copies data slightly differently depending
  on context
*/
{
    if (!strcmp(sym->data.pType->data.kind, "array")) {
        if (!strcmp(expr->kind, "address")) {
            int i;
            int elements = sym->data.pType->data.upper
                - sym->data.pType->data.lower + 1;
                
            for (i = 0; i < elements; i++) {
                emitAssign(parm, expr);
                parm->offset->value++;
                expr->offset->value++;
            }
        } else {
            yyerror("Expression passed in place of array parameter");
        }
    } else if (!strcmp(sym->data.pType->data.kind, "record")) {
        node *memDest = sym->data.next, *memSrc;
        memNode *src, *dest;
        if (sym->data.pType == expr->var->data.pType) {

            int i, elements = sym->data.size;

            if (!strcmp(sym->data.mode, "io")
                ||!strcmp(sym->data.mode, "o")) {
                elements--;
            }

            for (i = 0; i < elements; i++) {
                emitAssign(parm, expr);
                parm->offset->value++;
                expr->offset->value++;
            }
        } else {
            yyerror("Incompatible record type used as parameter");
        }
        //end of record copy
    } else {
        emitAssign(parm, expr);
    }
}

emitParamCopyOut(node *proc) 
/*
  helper function which, similarly to emitCopyIn, copies data out
  to different types of variables
*/
{
    node *temp = proc->data.next;

    while (temp != NULL) {
        if (isParmOut(temp)) {
            temp->data.depth = 0;

            emitCopyOut(temp);
        }                                       
        temp = temp->data.next;
    }
}

void emitCopyOut(node *parm) 
/*
  used to copy data out from parameters back to the provided variables
 */
{
    int i, size;

    parm->data.reg = NULL;
    //get a memory node for the data
    memNode *src = emitPrimId(parm);

    //get a memory node for the address to 
    //copy to
    memNode *dest = emitPrimId(parm);
    dest->offset->value += parm->data.size - 1;
    dest->base = getRegister(dest);

    //this is just for the sake of printing
    dest->offset->value = 0;

    size = parm->data.size - 1;

    for (i = 0; i < size; i++) {
        emitAssign(dest, src);
        dest->offset->value++;
        src->offset->value++;
    }

    /* if (!strcmp(parm->data.pType->data.kind, "array")) { */
    /*     int i; */
    /*     int elements = parm->data.pType->data.upper */
    /*         - parm->data.pType->data.lower + 1; */
                
    /*     fprintf(fp, "copying arrays\n"); */
    /*     for (i = 0; i < elements; i++) { */
    /*         emitAssign(dest, src); */
    /*         dest->offset->value++; */
    /*         src->offset->value++; */
    /*     } */
    /* } else if (!strcmp(parm->data.pType->data.kind, "record")) { */
    /*     node *temp = parm->data.next; */

        
    /* } else { */
    /*     emitAssign(dest, src); */
    /* } */
}

emitProcJump(node *proc, int base) 
/*
  prints the instructions to fix the return address
  and jump to the called procedure
 */
{
    //more cheating with bases
    int r3 = base;

    //set return address
    fprintf(fp, "%d: r%d := %d\n", instCt++, r3, instCt + 3);
    fprintf(fp, "%d: contents b, 1 := r%d\n", instCt++, r3);

    //jump to procedure
    fprintf(fp, "%d: pc := %d\n", instCt++, proc->data.address);
}

memNode* emitExp(memNode* loc1, memNode* loc2) 
/*
  prints the instructions to perform exponentiation with the supplied
  memory locations
 */
{
    //this code does not store the base in a register for use in
    //multiplication, and should probably be changed to do so

    memNode *exp;

    //set aside some registers
    int result = getResultReg(loc1, loc2), power = newReg(), bool = newReg(), zero = newReg(), op1 = getRegister(loc1), op2 = getRegister(loc2);

    fprintf(fp, "%d: r%d := 1\n", instCt++, result);//result = 1
    fprintf(fp, "%d: r%d := r%d\n", instCt++, power, op2);//store power
    fprintf(fp, "%d: r%d := 0\n", instCt++, zero);//we need to store 0

    //if zero result is 1
    fprintf(fp, "%d: r%d := r%d = r%d\n", instCt++, bool, power, zero);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 11, bool);

    //if power is negative, flip sign
    fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, bool, zero, power);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 2, bool);
    fprintf(fp, "%d: r%d := -r%d\n", instCt++, power, power);

    //the loop. multiply result by base, subtract 1 from power
    fprintf(fp, "%d: r%d := r%d * r%d\n", instCt++, result, result, op1);
    fprintf(fp, "%d: r%d := r%d - r2\n", instCt++, power, power);
    fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, bool, zero, power);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt - 3, bool);

    //if power was negative, result is 1/result
    fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, result, zero, op2);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 2, bool);
    fprintf(fp, "%d: r%d := r2 / r%d\n", instCt++, result, result);
 
    exp = (memNode*)malloc(sizeof(memNode));
    exp->kind = "register";
    exp->base = result;
    exp->constant = 1;

    return exp;
}

memNode* emitPrimNum(int num) 
/*
  creates a memory node to hold a number
  (it doesn't actually emit anything so that's sorta misleading)
 */
{
    memNode* exp = (memNode*)malloc(sizeof(memNode));
    exp->kind = mallocAndCpy("number");
    exp->value = num;
    exp->constant = 0;

    return exp;
}

memNode* emitPrimId(node *idPtr) 
/*
  creates a memory node to hold the address of an id
  prints walkback statements if necessary
 */
{
    memNode *exp;
    exp = (memNode*)malloc(sizeof(memNode));   
    exp->base = 0;

    if (idPtr->data.depth > 0) {
        exp = walkBack(idPtr);
        //fprintf(fp, "%d: r%d := contents r%d, %d\n", instCt++, exp->base, exp->base, idPtr->data.offset);
    } 
    exp->kind = mallocAndCpy("address");
    exp->constant = 1;
    exp->offset = (memNode*)malloc(sizeof(memNode));
    exp->offset->value = idPtr->data.offset;
    exp->offset->kind = mallocAndCpy("number");
    exp->var = idPtr;

    return exp;
}

memNode* emitMulDiv(memNode *loc1, memNode *loc2, int op) 
/*
  performs multiplication or division, based on the given
  operation
 */
{
    memNode* exp = (memNode*)malloc(sizeof(memNode));
  
    int result, op1, op2;

    op1 = getRegister(loc1);
    op2 = getRegister(loc2);
    result = getResultReg(loc1, loc2);

    if (op == 1)
        fprintf(fp, "%d: r%d := r%d * r%d\n", instCt++, result, op1, op2);
    else
        fprintf(fp, "%d: r%d := r%d / r%d\n", instCt++, result, op1, op2);

    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;
    
    return exp;
}

memNode* emitAddSub(memNode *loc1, memNode *loc2, int op) 
/*
  performs addition or subtraction, based on the given operation
 */
{
    memNode* exp = (memNode*)malloc(sizeof(memNode));

    int result, op1, op2;

    op1 = getRegister(loc1);
    op2 = getRegister(loc2);
    result = getResultReg(loc1, loc2);
   
    if (op == 1)
        fprintf(fp, "%d: r%d := r%d + r%d\n", instCt++, result, op1, op2);
    else
        fprintf(fp, "%d: r%d := r%d - r%d\n", instCt++, result, op1, op2);

    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

memNode* emitNot(memNode *loc) 
/*
  'not's the supplied memory node
 */
{
    memNode *exp = (memNode*)malloc(sizeof(memNode));

    int op = getRegister(loc);
    int result = getResultReg(loc, loc);

    fprintf(fp, "%d: r%d := not r%d\n", instCt++, result, op);

    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

memNode* emitNeg(memNode *loc) 
/*
  negates the supplied memory location
 */
{
    memNode *exp = (memNode*)malloc(sizeof(memNode));
    
    int op = getRegister(loc);
    int result = getResultReg(loc, loc);

    fprintf(fp, "%d: r%d := - r%d\n", instCt++, result, op);

    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

memNode* emitRelOp(memNode *loc1, memNode *loc2, int op) 
/*
  prints a relational operation based on the supplied operation
 */
{
    memNode *exp = (memNode*)malloc(sizeof(memNode));
    int result, op1, op2;

    op1 = getRegister(loc1);
    op2 = getRegister(loc2);
    result = getResultReg(loc1, loc2);

    switch(op) {
    case 1:
        fprintf(fp, "%d: r%d := r%d = r%d\n", instCt++, result, op1, op2);
        break;
    case 2:
        fprintf(fp, "%d: r%d := r%d /= r%d\n", instCt++, result, op1, op2);
        break;
    case 3:
        fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, result, op1, op2);
        break;
    case 4:
        fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, result, op2, op1);
        break;
    case 5:
        fprintf(fp, "%d: r%d := r%d <= r%d\n", instCt++, result, op1, op2);
        break;
    case 6:
        fprintf(fp, "%d: r%d := r%d <= r%d\n", instCt++, result, op2, op1);
        break;
    }
    
    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

memNode* emitBooP(memNode *loc1, memNode *loc2, int op) 
/*
  prints a boolean operation based on the supplied operation
 */
{
    memNode* exp = (memNode*)malloc(sizeof(memNode));
    int result, op1, op2;

    op1 = getRegister(loc1);
    op2 = getRegister(loc2);
    result = getResultReg(loc1, loc2);
    
    if (op == 0)
        fprintf(fp, "%d: r%d := r%d and r%d\n", instCt++, result, op1, op2);
    else
        fprintf(fp, "%d: r%d := r%d or r%d\n", instCt++, result, op1, op2);
    
    exp->kind = mallocAndCpy("register");
    exp->base = result;
    exp->constant = 1;

    return exp;
}

void emitAssign(memNode *loc1, memNode *loc2) 
/*
  prints a simple assignment between two memory nodes
 */
{
    char * op1 = getLocationString(loc1);
    char * op2 = getLocationString(loc2);

    fprintf(fp, "%d: %s := %s\n", instCt++, op1, op2);
}

emitJump(int address) 
/*
  prints a jump to the supplied address
 */
{
    fprintf(fp, "%d: pc := %d\n", instCt++, address);
}

emitJumpQ() 
/*
  prints a jump with a '?' as the address, which will later
  be patched
 */
{
    fprintf(fp, "%d: pc := ?\n", instCt++);
}

emitJumpTrue(memNode *condition) 
/*
  prints a conditional jump (again with a question mark) in the case
  that the supplied condition is true by comparing it against 0
 */
{
    int op1 = getRegister(condition);

    int result = getResultReg(condition, condition);
    //    fprintf(fp, "instCt is %d\n", instCt);
    //compare value to zero
    fprintf(fp, "%d: r%d := r%d /= r1\n", instCt++, result, op1);
    
    fprintf(fp, "%d: pc := ? if r%d\n", instCt++, result);
}

emitJumpFalse(memNode *condition) 
/*
  prints a condtional jump in the case that the supplied condition
  is false
 */
{
    int op1 = getRegister(condition);

    int result = getResultReg(condition, condition);
    //    fprintf(fp, "instCt is %d\n", instCt);
    //compare value to zero
    //    fprintf(fp, "%d: r%d := r%d = r1\n", instCt++, result, op1);
    
    fprintf(fp, "%d: pc := ? if not r%d\n", instCt++, result);
}

emitRead(memNode *var) 
/*
  prints a read_integer or read_boolean statement depending
  on the type of the provided variable
 */
{
    if (!strcmp(var->var->data.pType->data.name, "boolean")) {
        fprintf(fp, "%d: read_boolean %s\n", instCt++, getLocationString(var));
    } else {
        fprintf(fp, "%d: read_integer %s\n", instCt++, getLocationString(var));
    }
}

emitWrite(memNode *expr) 
/*
  prints a write instruction
 */
{
    fprintf(fp, "%d: write %s\n", instCt++, getLocationString(expr));
}

emitExcpStateCheck() {
    fprintf(fp, "%d: pc := ? if r3\n", instCt++);
}

emitRaise(int eID) 
/*
  changes the exception state and jumps (to the exception part)
 */
{
    if (eID > 0) {
        fprintf(fp, "%d: r3 := %d\n", instCt++, eID);
    }
    
    fprintf(fp, "%d: pc := ?\n", instCt++);
}

emitHandle() 
/*
  prints the instructions for the end of an exception handler
  which sets the exception state to 'no_exception'
  and jumps (to the normal return)
 */
{
    fprintf(fp, "%d: r3 := 0\n", instCt++);
    fprintf(fp, "%d: pc := ?\n", instCt++);
}
