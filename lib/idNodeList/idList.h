#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct idnode {
  char *name;
  struct idnode *next;
} *idnodeptr;

typedef struct list {
  int size;
  idnodeptr roots[50];
} list;

list theListOfLists; 

print(idnodeptr theList) {
  while (theList->next != NULL) {
    printf("%s ", theList->name);
    theList = theList->next;
  }
}

idnodeptr addId(idnodeptr theList, char* id) {
  char stringbuf[25];
  idnodeptr tempNode = (idnodeptr)malloc(sizeof(struct idnode));
  //  printf("Enter an id (25 character maximum): ");
  // scanf("%s", &stringbuf);

  tempNode->name = malloc(strlen(id)+1);
  strcpy(tempNode->name, id);

  tempNode->next = theList;

  return tempNode;
}

idnodeptr insertIdList() {
  idnodeptr root = (idnodeptr)malloc(sizeof(struct idnode));
  theListOfLists.roots[theListOfLists.size] = root;
  theListOfLists.size++;

  return root;
}
