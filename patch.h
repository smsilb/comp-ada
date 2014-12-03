typedef struct label {
    int address;
    struct label *next;
} label;

typedef struct patchNode {
    int address, val;
    struct patchNode *next;
    char *name;
} patchNode;

patchNode *head = NULL, *tail = NULL;
label *lhead = NULL, *ltail = NULL;
label *raiseHead = NULL, *raiseTail = NULL;
label* toPatch[100];
label* raiseStack[100];
int raiseTop = -1;
int patchTop = -1;
int inExceptionPart;
int handlerDone;
int currentException = 0;
int jumpTable[100];
int jumpCount = 0;

pushPS(char c) {
    if (c == 'p') {
        patchTop++;
        toPatch[patchTop] = NULL;
        lhead = ltail = toPatch[patchTop];
    } else if (c == 'e') {
        raiseTop++;
        raiseStack[raiseTop] = NULL;
        raiseHead = raiseTail = raiseStack[raiseTop];
    }
}

popPS(int patch, char c) {
    label *temp;
    if (c == 'p') {
        temp = lhead;
    } else if (c == 'e') {
        temp = raiseHead;
    }

    while(temp != NULL) {
        tail->next = (patchNode*)malloc(sizeof(patchNode));

        tail = tail->next;
        
        tail->address = temp->address;
        tail->val = patch;
        tail->next = NULL;

        temp = temp->next;
    }

    if (c == 'p') {
        patchTop--;

        if (patchTop >= 0 && toPatch[patchTop] != NULL) {
            ltail = lhead = toPatch[patchTop];

            while (ltail->next != NULL) {
                ltail = ltail->next;
            }
        }
    } else if (c == 'e') {
        raiseTop--;
        
        if (raiseTop >= 0 && raiseStack[raiseTop] != NULL) {
            raiseTail = raiseHead = raiseStack[raiseTop];

            while (raiseTail->next != NULL) {
                raiseTail = raiseTail->next;
            }
        }
    }
}

/*label* getRefLabel(char *labelName) {
  label *temp = lhead;

    while(temp != NULL) {
        if (!strcmp(temp->name, labelName)) {
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
    }*/

addLabel(int address, char c) {
    label *tHead, *tTail;
    if (c == 'p') {
        tHead = lhead;
        tTail = ltail;
    } else if (c == 'e') {
        tHead = raiseHead;
        tTail = raiseTail;
    }

    if (tHead == NULL) {
        tHead = toPatch[patchTop] = (label *)malloc(sizeof(label));
        tTail = tHead;
    } else {
        tTail->next = (label *)malloc(sizeof(label));
        tTail = tTail->next;
    }

    tTail->address = address;
    tTail->next = NULL;

    if (c == 'p') {
        lhead = tHead;
        ltail = tTail;
    } else if (c == 'e') {
        raiseHead = tHead;
        raiseTail = tTail;
    }
}

fixPatch(int address, int value) {
    patchNode *temp = head;
    
    while (temp->next != NULL && temp->address != address) {
        temp = temp->next;
    }
    
    temp->val = value;
}

addPatch(int address) {
    if (head == NULL) {
        head = (patchNode *)malloc(sizeof(patchNode));
        tail = head;
    } else {
        tail->next = (patchNode *)malloc(sizeof(patchNode));
        tail = tail->next;
    }

    tail->address = address;
    tail->next = NULL;
}

printPatches() {
    patchNode *temp = head;

    printf("Patches:\n");
    
    while(temp != NULL) {
        printf("Address: %d; Value: %d;\n", temp->address, temp->val);

        temp = temp->next;
    }
}

int getPatchVal(int address) {
    patchNode *temp = head;

    while (temp->next != NULL && temp->address != address) {
        temp = temp->next;
    }
                
    return temp->val;
}

writePatches(char * inFile, int lineno) {
    FILE * in = fopen(inFile, "r");
    FILE * out = fopen("out.txt", "w+");

    char buffer[150];
    char * line = NULL;
    size_t len = 0;
    char s;
    int i, linect = 0;

    while (getline(&line, &len, fp) != -1) {
        strcpy(buffer, line);

        for (i = 0; i < len; i++) {
            if (buffer[i] == '\0') {
                break;
            } else if (buffer[i] == '?') {
                fprintf(out, "%d", getPatchVal(linect));
            } else {
                fprintf(out, "%c", buffer[i]);
            }
        }
        
        linect++;
    }

    fclose(in);
    fclose(out);
}
