typedef struct exprNode {
    char *kind;
    int offset, base, value, constant;
    node *var;
} exprNode;

int instCt = 0;
int nextReg = 0;

FILE * fp;

char * getLocation(exprNode* exp) {
    char *location;
    if (!strcmp(exp->kind, "register")) {
        location = (char*)malloc(sizeof(exp->base) + 2);
        sprintf(location, "r%d", exp->base);
    } else if (!strcmp(exp->kind, "address")) {
        location = (char*)malloc(sizeof(exp->base) + sizeof(exp->offset) + 13);
        if (exp->base == 0) 
            sprintf(location, "contents b, %d", exp->offset);
        else
            sprintf(location, "contents r%d, %d", exp->base, exp->offset);
    } else if (!strcmp(exp->kind, "number")) {
        int reg = newReg();
        location = (char*)malloc(sizeof("r") + sizeof(reg));
        sprintf(location, "r%d", reg);
        fprintf(fp, "%d: %s := %d\n", instCt++, location, exp->value);
    }

    return location;
}

int getRegister(exprNode *exp) {
    int reg;

    if (!strcmp(exp->kind, "register")) {
        return exp->base;
    } else if(!strcmp(exp->kind, "number")) {
        reg = newReg();

        fprintf(fp, "%d: r%d := %d\n", instCt++, reg, exp->value);

        return reg;
    } else if(!strcmp(exp->kind, "address")) {
        /*if (exp->var->data.reg == NULL) {
            reg = newReg();
            char * location = getLocation(exp);
            
            fprintf(fp, "%d: r%d := %s\n", instCt++, reg, location);
            
            exp->var->data.reg = (exprNode*)malloc(sizeof(exprNode));
            exp->var->data.reg->kind = mallocAndCpy("register");
            exp->var->data.reg->base = reg;
        */
            return reg;
            /*        } else {
            return exp->var->data.reg->base;
            }*/
    }
}

freeRegisters(int count) {
    nextReg -= count;
}

int newReg() {
    nextReg++;
    return nextReg;
}

exprNode* walkBack(node *ptr) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));

    int result = newReg();
    fprintf(fp, "%d: r%d := b\n", instCt++, result);
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

emitPrologue() {
    fprintf(fp, "%d: b := ?\n", instCt++);
    fprintf(fp, "%d: contents b, 0 := ?\n", instCt++);
    fprintf(fp, "%d: contents b, 1 := 4\n", instCt++);
    fprintf(fp, "%d: pc := ?\n", instCt++);
    fprintf(fp, "%d: halt\n", instCt++);
}

emitReturn() {
    int r1 = newReg();
    fprintf(fp, "%d: r%d := contents b, 1\n", instCt++, r1);
    fprintf(fp, "%d: b := contents b, 3\n", instCt++);
    fprintf(fp, "%d: pc := r%d\n", instCt++, r1);
}

emitProcCall(node *proc) {
    exprNode* exp;
    int r1 = newReg(),r2 = newReg(),r3 = newReg();

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
        fprintf(fp, "%d: contents b, 2 := r%d, 2\n", instCt++, r1);
    } else {
        //static link == dynamic link
        fprintf(fp, "%d: contents b, 2 := r%d\n", instCt++, r1);
    }

    //adjust new base w/ offset of procedure
    fprintf(fp, "%d: r%d := %d\n", instCt++, r2, proc->data.offset);
    fprintf(fp, "%d: contents b, 0 := b + r%d\n", instCt++, r2);

    //set return address
    fprintf(fp, "%d: r%d := %d\n", instCt++, r3, instCt + 3);
    fprintf(fp, "%d: contents b, 1 := r%d\n", instCt++, r3);

    //jump to procedure
    fprintf(fp, "%d: pc := %d\n", instCt++, proc->data.address);
}

exprNode* emitExp(int loc1, int loc2) {
    //this code does not store the base in a register for use in
    //multiplication, and should probably be changed to do so

    exprNode *exp;

    //set aside some registers
    int result = newReg(), power = newReg(), bool = newReg(), zero = newReg(), one = newReg();


    fprintf(fp, "%d: r%d := 1\n", instCt++, result);//result = 1
    fprintf(fp, "%d: r%d := r%d\n", instCt++, power, loc2);//store power
    fprintf(fp, "%d: r%d := 0\n", instCt++, zero);//we need to store 0
    fprintf(fp, "%d: r%d := 1\n", instCt++, one);//we need to store 1

    //if zero result is 1
    fprintf(fp, "%d: r%d := r%d = r%d\n", instCt++, bool, power, zero);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 11, bool);

    //if power is negative, flip sign
    fprintf(fp, "%d: r%d := r%d <= r%d\n", instCt++, bool, zero, power);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 2, bool);
    fprintf(fp, "%d: r%d := -r%d\n", instCt++, power, power);

    //the loop. multiply result by base, subtract 1 from power
    fprintf(fp, "%d: r%d := r%d * r%d\n", instCt++, result, result, loc1);
    fprintf(fp, "%d: r%d := r%d - r%d\n", instCt++, power, power, one);
    fprintf(fp, "%d: r%d := r%d <= r%d\n", instCt++, bool, zero, power);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt - 3, bool);

    //if power was negative, result is 1/result
    fprintf(fp, "%d: r%d := r%d <= r%d\n", instCt++, result, zero, loc2);
    fprintf(fp, "%d: pc := %d if r%d\n", instCt++, instCt + 2, bool);
    fprintf(fp, "%d: r%d := r%d / r%d\n", instCt++, result, one, result);
 
    exp = (exprNode*)malloc(sizeof(exprNode));
    exp->kind = "register";
    exp->base = result;
    exp->constant = 1;
}

exprNode* emitPrimNum(int num) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));
    exp->kind = mallocAndCpy("number");
    exp->value = num;
    exp->constant = 0;

    return exp;
}

exprNode* emitPrimId(node *idPtr) {
    exprNode *exp;
    exp = (exprNode*)malloc(sizeof(exprNode));
    exp->kind = mallocAndCpy("address");
    exp->constant = 1;
    exp->offset = idPtr->data.offset;
    exp->base = 0;
    /*exp->var = idPtr;
    
    if (idPtr->data.reg == NULL) {
        idPtr->data.reg = exp;
        }*/

    if (idPtr->data.depth > 0) {
        exp = walkBack(idPtr);
    } 

    return exp;
}

exprNode* emitMulDiv(int loc1, int loc2, int op) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));
  
    if (op == 1)
        fprintf(fp, "%d: r%d := r%d * r%d\n", instCt++, loc1, loc1, loc2);
    else
        fprintf(fp, "%d: r%d := r%d / r%d\n", instCt++, loc1, loc1, loc2);

    exp->kind = mallocAndCpy("register");
    exp->base = loc1;
    exp->constant = 1;
    
    return exp;
}

exprNode* emitAddSub(int loc1, int loc2, int op) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));
   
    if (op == 1)
        fprintf(fp, "%d: r%d := r%d + r%d\n", instCt++, loc1, loc1, loc2);
    else
        fprintf(fp, "%d: r%d := r%d - r%d\n", instCt++, loc1, loc1, loc2);

    exp->kind = mallocAndCpy("register");
    exp->base = loc1;
    exp->constant = 1;
    
    return exp;
}

exprNode* emitNot(int loc) {
    exprNode *exp = (exprNode*)malloc(sizeof(exprNode));

    fprintf(fp, "%d: r%d := - r%d\n", loc, loc);

    exp->kind = mallocAndCpy("register");
    exp->base = loc;
    exp->constant = 1;

    return exp;
}

exprNode* emitRelOp(int loc1, int loc2, int op) {
    exprNode *exp = (exprNode*)malloc(sizeof(exprNode));

    if (op <= 2) {
        fprintf(fp, "%d: r%d := r%d = r%d\n", instCt++, loc1, loc1, loc2);
    } else if (op <= 4) {
        fprintf(fp, "%d: r%d := r%d < r%d\n", instCt++, loc1, loc1, loc2);
    } else {
        fprintf(fp, "%d: r%d := r%d > r%d\n", instCt++, loc1, loc1, loc2);
    }

    if (op == 2 || op == 4 || op == 6) {
        fprintf(fp, "%d: r%d := not r%d\n", instCt++, loc1, loc1);
    }

    exp->kind = mallocAndCpy("register");
    exp->base = loc1;
    exp->constant = 1;

    return exp;
}

exprNode* emitBooP(int loc1, int loc2, int op) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));
    
    if (op == 0)
        fprintf(fp, "%d: r%d := r%d and r%d\n", instCt++, loc1, loc1, loc2);
    else
        fprintf(fp, "%d: r%d := r%d or r%d\n", instCt++, loc1, loc1, loc2);
    
    exp->kind = mallocAndCpy("register");
    exp->base = loc1;
    exp->constant = 1;

    return exp;
}

exprNode* emitAssign(char *loc1, char *loc2) {
    exprNode* exp = (exprNode*)malloc(sizeof(exprNode));

    fprintf(fp, "%d: %s := %s\n", instCt++, loc1, loc2);

    return exp;
}
