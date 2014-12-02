int tabs = 0;
node *words = NULL;
int next_exception = 2;

printTabs(int tabs) {
    int i = 0;
    for (i = 0; i < tabs; i++) {
        printf("\t");
    }
}

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

printNode(node *root){
    if (root->left != NULL) {
        printNode(root->left);
    }
    if (root == NULL) {
        printf("i shouldnt be here\n");
    }

    printTabs(tabs);
    printf("%s:", root->data.name);
    printf(" %s", root->data.kind);
    if (!strcmp(root->data.kind, "array")) {
        printf(" of components %s from %d to %d", root->data.cType->data.name, root->data.lower, root->data.upper);
    } else if (!strcmp(root->data.kind, "range")) {
        printf(" from %d to %d", root->data.lower, root->data.upper);
    } else if (!strcmp(root->data.kind, "variable")) {
        printf(" of type %s", root->data.pType->data.name);
        printf(" with size %d and offset %d", root->data.size, root->data.offset);
    } else if (!strcmp(root->data.kind, "proc")) {
        node *temp = root->data.next;
        while(temp != NULL){
            printf("\n");
            printTabs(tabs);
            printf("\tParameter: %s, %s of type %s with size %d and offset %d", temp->data.name, temp->data.mode, temp->data.pType->data.name, temp->data.size, temp->data.offset);
            temp = temp->data.next;
        }
    } else if (!strcmp(root->data.kind, "record")) {
        node *temp = root->data.next;
        printf(", with members:\n");
        /* while(temp != NULL) {
            printTabs(tabs);
            printf("\t%s: %s with size %d and offset %d\n", temp->data.name, temp->data.kind, temp->data.size, temp->data.offset);
            temp = temp->data.next;
            }*/
    }
    printf("\n");
    if (root->right != NULL){
        printNode(root->right);
    }
}

symbol getEmptySymbol(){
    symbol sym;
    return sym;
}

int nextException() {
    next_exception++;
    return next_exception;
}

outerContext(){
    symbol oc;
    oc.pType = NULL;
    oc.cType = NULL;

    oc.name = mallocAndCpy("integer");
    oc.kind = mallocAndCpy("predefined");
    oc.size = 1;
    oc.offset = 0;
    addSymbol(oc);

    oc.name = mallocAndCpy("boolean");
    oc.kind = mallocAndCpy("predefined");
    addSymbol(oc);

    oc.name = mallocAndCpy("true");
    oc.kind = mallocAndCpy("constant");
    oc.pType = getTypeNode("boolean");
    oc.size = 0;
    oc.value = 1;
    addSymbol(oc);
    
    oc.name = mallocAndCpy("false");
    oc.kind = mallocAndCpy("constant");
    oc.pType = getTypeNode("boolean");
    oc.value = 0;
    addSymbol(oc);
    
    oc.name = mallocAndCpy("maxint");
    oc.kind = mallocAndCpy("constant");
    oc.pType = getTypeNode("integer");
    oc.value = 12;
    addSymbol(oc);

    oc.name = mallocAndCpy("constraint_error");
    oc.kind = mallocAndCpy("exception");
    oc.value = 1;
    addSymbol(oc);

    oc.name = mallocAndCpy("numeric_error");
    oc.value = 2;
    addSymbol(oc);

    oc.name = mallocAndCpy("read");
    oc.kind = mallocAndCpy("read_routine");
    oc.pType = NULL;
    addSymbol(oc);

    oc.name = mallocAndCpy("write");
    oc.kind = mallocAndCpy("write_routine");
    addSymbol(oc);

    // add all the reserved words
    /* addReserved("is");
    addReserved("end");
    addReserved("if");
    addReserved("record");
    addReserved("loop");
    addReserved("exit");
    addReserved("when");
    addReserved("of");
    addReserved("begin");
    addReserved("procedure");
    addReserved("type");
    addReserved("array");
    addReserved("record");
    addReserved("in");
    addReserved("out");
    addReserved("range");
    addReserved("constant");
    addReserved("constant");
    addReserved("exception");
    addReserved("null");
    addReserved("loop");
    addReserved();*/
}

addReserved(char* s) {
    symbol ref;
    ref.kind = mallocAndCpy("reserved");
    insert(words, ref);
}

int checkReserved(char *s) {

}
