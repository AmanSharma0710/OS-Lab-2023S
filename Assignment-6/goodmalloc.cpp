#include "goodmalloc.h"
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <pthread.h>

using namespace std;

char* BIG_MEMORY_BLOCK = NULL;
int maxNodes = 0;
int nodesInMemory = 0;
int current_scope = 0;

pthread_mutex_t lockMemory = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockPageTable = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockScope = PTHREAD_MUTEX_INITIALIZER;

int createMemory(int size){
    pthread_mutex_lock(&lockMemory);
    if(BIG_MEMORY_BLOCK == NULL){
        BIG_MEMORY_BLOCK = new char[size];
        maxNodes = size / sizeof(Node);
        for(int i = 0; i < maxNodes; i++){
            Node *node = (Node*)BIG_MEMORY_BLOCK;
            node += i;
            node->free = true;
        }
        pthread_mutex_unlock(&lockMemory);
        return 0;
    }
    pthread_mutex_unlock(&lockMemory);
    return -1;
}

struct _pageTableEntry{
    int offset;
    int scope;
    List* list;
};
typedef struct _pageTableEntry pageTableEntry;

class PageTable{
public:
    map<string, pageTableEntry> table;
    void addEntry(string name, int offset, List *list){
        pageTableEntry entry;
        entry.offset = offset;
        entry.scope = current_scope;
        entry.list = list;
        table[name] = entry;
    }
    bool findEntry(string name){
        return table.find(name) != table.end();
    }
    int getOffset(string name){
        return table[name].offset;
    }
    int getScope(string name){
        return table[name].scope;
    }
};


PageTable *pageTable = new PageTable();


void startScope(){
    pthread_mutex_lock(&lockScope);
    current_scope++;
    pthread_mutex_unlock(&lockScope);
}

void endScope(){
    pthread_mutex_lock(&lockScope);
    current_scope--;
    pthread_mutex_unlock(&lockScope);
    //Remove all the entries in the page table that have a scope greater than the current scope
    vector<string> temp;
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    map<string, pageTableEntry>::iterator it;
    for(it = pageTable->table.begin(); it != pageTable->table.end(); it++){
        if(it->second.scope > current_scope){
            temp.push_back(it->first);
        }
    }
    //Mark the nodes in the list as free, and remove the entries from the page table
    for(int i = 0; i < temp.size(); i++){
        Node *node = (Node*)BIG_MEMORY_BLOCK;
        node += pageTable->getOffset(temp[i]);
        while(node->next != NULL){
            node->free = true;
            node = node->next;
        }
        pageTable->table.erase(temp[i]);
    }
    pthread_mutex_unlock(&lockPageTable);
    pthread_mutex_unlock(&lockMemory);
}

List* createList(string name, int len){
    //Check if the list already exists in the page table
    pthread_mutex_lock(&lockScope);
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    if(!pageTable->findEntry(name)){
        //Check if the list can fit in memory
        if(nodesInMemory + len <= maxNodes){
            //Create the list
            List *list = new List();
            list->name = name;
            list->size = len;
            //Find the first free node
            Node *node = (Node*)BIG_MEMORY_BLOCK;
            while(node->free == false){
                node++;
            }
            //Set the head of the list to the first free node
            list->head = node;
            //Set the first node to be the head of the list
            node->free = false;
            node->prev = NULL;
            node->next = NULL;
            //Add the entry to the page table
            pageTable->addEntry(name, node - (Node*)BIG_MEMORY_BLOCK, list);
            //Add the rest of the nodes to the list
            for(int i = 1; i < len; i++){
                Node* nextNode = node + 1;
                //Find the next free node
                while(nextNode->free == false){
                    nextNode++;
                }
                //Set the next node to be the next node in the list
                node->next = nextNode;
                nextNode->prev = node;
                nextNode->free = false;
                nextNode->next = NULL;
                node = nextNode;
            }
            nodesInMemory += len;
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            pthread_mutex_unlock(&lockScope);
            return list;
        }
        else{
            //The list cannot fit in memory
            cout<<"createList(): The list cannot fit in memory."<<endl;
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            pthread_mutex_unlock(&lockScope);
            exit(1);
        }
    }
    pthread_mutex_unlock(&lockPageTable);
    pthread_mutex_unlock(&lockMemory);
    pthread_mutex_unlock(&lockScope);
    //The list already exists in the page table
    cout<<"createList(): The list already exists in the page table."<<endl;
    return NULL;
}

int freeList(string name){
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    //Check if the list exists in the page table
    if(pageTable->findEntry(name)){
        //Mark the nodes in the list as free
        Node *node = (Node*)BIG_MEMORY_BLOCK;
        node += pageTable->getOffset(name);
        while(node->next != NULL){
            node->free = true;
            node = node->next;
        }
        //Remove the entry from the page table
        pageTable->table.erase(name);
        pthread_mutex_unlock(&lockPageTable);
        pthread_mutex_unlock(&lockMemory);
        return 0;
    }
    else{
        //The list does not exist in the page table
        cout<<"freeList(): The list does not exist in the page table."<<endl;
        pthread_mutex_unlock(&lockPageTable);
        pthread_mutex_unlock(&lockMemory);
        return -1;
    }
}

int freeMemory(){
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    if(BIG_MEMORY_BLOCK != NULL){
        //Reset the page table
        while(!pageTable->table.empty()){
            freeList(pageTable->table.begin()->first);
        }
        //Free the memory
        delete[] BIG_MEMORY_BLOCK;
        BIG_MEMORY_BLOCK = NULL;
        maxNodes = 0;
        nodesInMemory = 0;
        pthread_mutex_unlock(&lockPageTable);
        pthread_mutex_unlock(&lockMemory);
        return 0;
    }
    //The memory has already been freed
    cout<<"freeMemory(): The memory has already been freed."<<endl;
    pthread_mutex_unlock(&lockPageTable);
    pthread_mutex_unlock(&lockMemory);
    return -1;
}

int assignVal(string name, int index, int val){
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);

    //Check if the list exists in the page table
    if(pageTable->findEntry(name)){
        //Check if the index is in the list
        if(index < pageTable->table[name].list->size){
            //Find the node at the index
            Node *node = (Node*)BIG_MEMORY_BLOCK;
            node += pageTable->getOffset(name);
            for(int i = 0; i < index; i++){
                node = node->next;
            }
            //Assign the value to the node
            node->data = val;
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            return 0;
        }
        else{
            //The index is not in the list
            cout<<"assignVal(): The index is not in the list."<<endl;
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            return -1;
        }
    }
    //The list does not exist in the page table
    cout<<"assignVal(): The list does not exist in the page table."<<endl;
    pthread_mutex_unlock(&lockPageTable);
    pthread_mutex_unlock(&lockMemory);
    return -1;
}

int getVal(string name, int index){
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    //Check if the list exists in the page table
    if(pageTable->findEntry(name)){
        //Check if the index is in the list
        if(index < pageTable->table[name].list->size){
            //Find the node at the index
            Node *node = (Node*)BIG_MEMORY_BLOCK;
            node += pageTable->getOffset(name);
            for(int i = 0; i < index; i++){
                node = node->next;
            }
            //Return the value of the node
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            return node->data;
        }
        else{
            //The index is not in the list
            cout<<"getVal(): The index is not in the list."<<endl;
            pthread_mutex_unlock(&lockPageTable);
            pthread_mutex_unlock(&lockMemory);
            return -1;
        }
    }
    //The list does not exist in the page table
    cout<<"getVal(): The list does not exist in the page table."<<endl;
    pthread_mutex_unlock(&lockPageTable);
    pthread_mutex_unlock(&lockMemory);
    return -1;
}

void List::print(){
    pthread_mutex_lock(&lockMemory);
    pthread_mutex_lock(&lockPageTable);
    //Check if the list exists in the page table
    if(pageTable->findEntry(name)){
        //Print the list
        Node *node = (Node*)BIG_MEMORY_BLOCK;
        node += pageTable->getOffset(name);
        while(node->next != NULL){
            cout<<node->data<<" ";
            node = node->next;
        }
        cout<<node->data<<endl;
        pthread_mutex_unlock(&lockPageTable);
        pthread_mutex_unlock(&lockMemory);
    }
    else{
        //The list does not exist in the page table
        cout<<"print(): The list does not exist in the page table."<<endl;
        pthread_mutex_unlock(&lockPageTable);
        pthread_mutex_unlock(&lockMemory);
    }
}