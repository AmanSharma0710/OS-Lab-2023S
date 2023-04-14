#include"goodmalloc.h"
#include <iostream>
#include <string>
#include <random>
#include <sys/resource.h>
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
    // comment below for without freeList
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
    int who = RUSAGE_SELF;
    struct rusage usage;
    int ret = getrusage(who, &usage);
    if (ret == -1) {
        cout << "Error in getrusage" << endl;
        return -1;
    }
    double mem_usage = usage.ru_maxrss / 1024.0;
    struct timeval time = usage.ru_utime;
    double tot_time = time.tv_sec + (time.tv_usec / 1000000.0);
    FILE *fp = fopen("mem.txt", "a");
    fprintf(fp, "%lf\n", mem_usage);
    fclose(fp);
    fp = fopen("time.txt", "a");
    fprintf(fp, "%lf\n", tot_time);
    fclose(fp);
    return 0;
    // uncomment below for without freeList
    // FILE *fp = fopen("mem_without.txt", "a");
    // fprintf(fp, "%lf\n", mem_usage);
    // fclose(fp);
    // fp = fopen("time_without.txt", "a");
    // fprintf(fp, "%lf\n", tot_time);
    // fclose(fp);
    // return 0;
}
