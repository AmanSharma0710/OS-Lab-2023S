#include"goodmalloc.h"
#include <iostream>
#include <string>
#include <random>

using namespace std;
#define LENGTH 50000

List* mergeSort(List* list, int level){
    if(list->size == 1 || list->size == 0){
        return list;
    }
    startScope();
    int mid = list->size / 2;
    string leftName = "left" + to_string(level);
    string rightName = "right" + to_string(level);
    List* left = createList(leftName, mid);
    List* right = createList(rightName, list->size - mid);
    for(int i = 0; i < mid; i++){
        assignVal(leftName, i, getVal(list->name, i));
    }
    for(int i = mid; i < list->size; i++){
        assignVal(rightName, i - mid, getVal(list->name, i));
    }
    left = mergeSort(left, level + 1);
    right = mergeSort(right, level + 1);
    Node* leftNode = left->head;
    Node* rightNode = right->head;
    for(int i = 0; i < list->size; i++){
        if(leftNode == NULL){
            assignVal(list->name, i, rightNode->data);
            rightNode = rightNode->next;
        }
        else if(rightNode == NULL){
            assignVal(list->name, i, leftNode->data);
            leftNode = leftNode->next;
        }
        else if(leftNode->data < rightNode->data){
            assignVal(list->name, i, leftNode->data);
            leftNode = leftNode->next;
        }
        else{
            assignVal(list->name, i, rightNode->data);
            rightNode = rightNode->next;
        }
    }
    freeList(leftName);
    freeList(rightName);
    endScope();
    return list;
}

int main(){
    createMemory(250*1024*1024);
    startScope();
    List* list = createList("list", LENGTH);
    for(int i = 0; i < LENGTH; i++){
        int val = (rand() % 100000) + 1;
        assignVal("list", i, val);
    }
    list = mergeSort(list, 0);
    // list->print();
    endScope();
    freeMemory();
    return 0;
}

// TODO: Print debug messages in all functions, add print mutex