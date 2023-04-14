#ifndef GOODMALLOC_H
#define GOODMALLOC_H

#include <string>
using namespace std;

typedef struct Node{
    int data;
    Node *next;
    Node *prev;
    bool free;
}Node;

class List{
public:
    string name;
    Node *head;
    int size;
    void print();
};

/*
    Function to create a memory block
    @param int the size of the memory block to be created
    @return 0 if the memory block was successfully created, -1 if the memory block was not created
*/
int createMemory(int);

/*
    Function to create a list
    @param string the name of the list to be created
    @param int the size of the list to be created
    @return a list object if the list was successfully created, NULL if the list was not created
*/
List* createList(string, int);

/*
    Function to assign a value to a node in a list
    @param string the name of the list to be assigned to
    @param int the index of the node to be assigned to
    @param int the value to be assigned to the node
    @return 0 if the value was successfully assigned, -1 if the value was not assigned
*/
int assignVal(string, int, int);

/*
    Function to get the value of a node in a list
    @param string the name of the list to be assigned to
    @param int the index of the node to be assigned to
    @return the value of the node if the value was successfully retrieved, -1 if the value was not retrieved
*/
int getVal(string, int);

/*
    Function to free up the memory allocated to a list
    @param string the name of the list to be freed
    @return 0 if the list was successfully freed, -1 if the list was not found
*/
int freeList(string);      //Renamed the function called freeElem to freeList because it is more descriptive of what the function does

/* 
    Additional function that can be called to free up the big block of memory allocated
    @return number of memory blocks freed
*/
int freeMemory();

/*
    User callable function that declares the start of a scope
*/
void startScope();

/*
    User callable function that declares the end of a scope
*/
void endScope();

#endif