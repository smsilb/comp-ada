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
label* toPatch[100];
int patchTop = -1;

pushPS() {
    patchTop++;
    toPatch[patchTop] = NULL;
    lhead = ltail = toPatch[patchTop];
}

popPS(int patch) {
    label *temp = lhead;

    while(temp != NULL) {
        tail->next = (patchNode*)malloc(sizeof(patchNode));

        tail = tail->next;

        tail->address = temp->address;
        tail->val = patch;
        tail->next = NULL;

        temp = temp->next;
    }

    patchTop--;

    if (patchTop >= 0 && toPatch[patchTop] != NULL) {
        ltail = lhead = toPatch[patchTop];

        while (ltail->next != NULL) {
            ltail = ltail->next;
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

addLabel(int address) {
    if (lhead == NULL) {
        lhead = toPatch[patchTop] = (label *)malloc(sizeof(label));
        ltail = lhead;
    } else {
        ltail->next = (label *)malloc(sizeof(label));
        ltail = ltail->next;
    }
    ltail->address = address;
    ltail->next = NULL;
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
    ssize_t read;
    char s;
    int i, j, linect = 0, count = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
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

    unlink(inFile);
}
