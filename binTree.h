/***********************************************************
 * binTree.c
 * 
 * Author: Sam Silberstein
 * Date: 9/8/14
 * Course: CSCI 364, Prof. King
 * Purpose: Implements a datatype for a stack of binary trees
 *********************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  symbol contains a number for data, a pointer to allow links
  to other trees, and a character array to store a name
 */
struct symbol {
    int value, lower, upper, size, offset, depth, address;
    struct node *next, *pType, *cType;
    char *name, *kind, *mode;
    struct memNode* reg;
};


/*
  node structs are used to implement binary trees

  they contain a symbol struct to hold the data, and two
  pointers to nodes for left and right subtrees
 */
struct node {
  struct symbol data;
  struct node* left;
  struct node* right;
};


/*
  stackNode structs represent elements of the stack
  (the stack was initially implemented as a linked list, which
  is why they are stack'Node's)
 */
struct stackNode {
    char* name;
    struct node* root;
    int ARsize;
};

typedef struct symbol symbol;
typedef struct node node;
typedef struct stackNode stackNode;

/*
  stack and tree functions operate on a global stack array,
  so it (and an integer to track the top of the stack) are initialized
  here
 */

char* mallocAndCpy(char* string);
stackNode stack[100];
int top = 0;


// pushed a new tree onto the stack, unless the stack is full
push(char* name, int offset) {
  stackNode temp;
  if (top < 999) {
      stack[top].ARsize = offset;
      top++;
      temp.root = NULL;
      temp.name = (char*)malloc(sizeof(name)+1);
      strcpy(temp.name, name);
      stack[top] = temp;
      //printf("Pushing scope for %s\n", name);
  } else {
      yyerror("Stack is full and you, the end user, know what this means since you wrote this compiler\n");
  }
}

// deletes all of the sub trees of a node
// (used when popping a tree off the stack)
deleteNode(node* root) {
  if (root != NULL) {
    deleteNode(root->left);
    deleteNode(root->right);
    free(root);
  }
}

// pops a tree off of the stack, and frees the memory associated
// with it
int pop() {
  if (top >= 0) {
      //printf("Popping scope for %s\n", stack[top].name);
      //if (stack[top].root != NULL) {
      //printNode(stack[top].root);
      //}
    deleteNode(stack[top].root);
    top--;
    return stack[top].ARsize;
  } else {
    yyerror("Stack is empty\n");
    return -1;
  }
}

// binary search 
// compares a given key to the 'number' data member of the
// root node's 'data' data member and continues the search
// based on the result
node* search( node* root, char* sym) {
    if (root == NULL || strcmp(root->data.name, sym) == 0) {
        return root;
    } else if (strcmp(sym, root->data.name) < 0) {
        return search(root->left, sym);
    } else {
        return search(root->right, sym);
    }
}

// helper function for search which prompts
// the user for a key to search for, and searches
// down through the stack if the key is not found
// in the top
node* searchStack(char* sym) {
  stackNode temp;
  int count = top;
  node *retVal;

  while (count >= 0) {
    temp = stack[count];
    retVal = search(temp.root, sym);
    if (retVal != NULL) {
        retVal->data.depth = top - count;
        return retVal;
    }

    count--;
  }

  return NULL;
}

/*
  recursively inserts a new node into the tree.
  only compares against less than and greater that so that
  if the data is already present in the tree it is left 
  unchanged.
*/
node* insert(node* root, symbol sym) {
  if (root == NULL) {
    root = (node *)malloc(sizeof(node));
    root->left = NULL;
    root->right = NULL;
    root->data = sym;  
    return root;
  } else if (strcmp(sym.name, root->data.name) < 0) {   
    root->left = insert(root->left, sym);
    return root;
  } else if (strcmp(sym.name, root->data.name) > 0) {   
    root->right = insert(root->right, sym);
    return root;
  }  
}

// helper function for insert. prompts for a symbol
// to insert, and checks to see if that symbol is already
// in the tree before calling insert
node* addSymbol(symbol sym) {
  if (top >= 0 ) {
    if (search(stack[top].root, sym.name) == NULL) {
      stack[top].root = insert(stack[top].root, sym);
      return search(stack[top].root, sym.name);
    } else {
        char mes[200] = "Var: '";
        char* s;
        strcat(mes, sym.name);
        strcat(mes, "' is already defined in this context");
        s = (char*)malloc(sizeof(mes)+1);
        strcpy(s, mes);
        yyerror(s);
    }
    return NULL;
  }
}
