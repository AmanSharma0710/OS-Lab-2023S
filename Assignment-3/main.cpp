#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <vector>
#include <utility>
#include <string>
using namespace std;


int main(int argc, char *argv[]){
    if(argc > 2){
        cout << "Usage: ./main [-optimize]" << endl;
        exit(1);
    }
    if(argc == 2){
        string s = argv[1];
        if(s != "-optimize"){
            cout << "Usage: ./main [-optimize]" << endl;
            exit(1);
        }
    }
    bool optimize = false;
    if(argc == 2){
        optimize = true;
    }
    // Create shared memory
    int shmid;
    key_t key = ftok("shmfile", 69);
    char *shm, *s;
    // Create the segment of size 2MB
    shmid = shmget(key, 2*1024*1024, 0666|IPC_CREAT);
    if(shmid < 0){
        perror("shmget");
        exit(1);
    }
    shm = (char*) shmat(shmid, NULL, 0);
    if(shm == (char*) -1){
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    // Now we open the file and load the initial graph into the shared memory
    // We also load the number of vertices and edges into the shared memory
    FILE *fp;
    fp = fopen("facebook_combined.txt", "r");
    if(fp == NULL){
        perror("fopen");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }
    int n=0;
    vector<pair<int, int>> edges;
    int u, v;
    while(fscanf(fp, "%d %d", &u, &v) != EOF){
        edges.push_back(make_pair(u, v));
        n = max(n, max(u, v));
    }
    fclose(fp);
    // Now we have the edges in the vector
    // We need to load the edges into the shared memory
    // We also need to load the number of vertices and edges into the shared memory
    // We will load the number of vertices and edges in the first 2 integers
    // We will load the edges in the next 2*edges.size() integers
    int *shm_int = (int*) shm;
    shm_int[0] = n+1;
    shm_int[1] = edges.size();
    for(int i=0; i<edges.size(); i++){
        shm_int[2*i+2] = edges[i].first;
        shm_int[2*i+3] = edges[i].second;
    }
    // Now we have loaded the graph into the shared memory
    // We create the producer and consumer processes

    // Create the producer process
    pid_t pid = fork();
    if(pid < 0){
        perror("fork");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }
    else if(pid == 0){
        // This is the producer process
        // Run the producer program
        execl("./producer", "producer", NULL);
        perror("execl");
        exit(1);
    }
    // Create the 10 consumer processes
    for(int i=0; i<10; i++){
        pid = fork();
        if(pid < 0){
            perror("fork");
            shmctl(shmid, IPC_RMID, NULL);
            exit(1);
        }
        else if(pid == 0){
            // This is the consumer process
            // Run the consumer program
            char *arg = (char*) malloc(15*sizeof(char));
            sprintf(arg, "%d", i);
            if(optimize){
                execl("./consumer", "consumer", arg, "-optimize", NULL);
                perror("execl");
                exit(1);
            }
            execl("./consumer", "consumer", arg, NULL);
            perror("execl");
            exit(1);
        }
    }
    // Detach from the shared memory and exit
    shmdt(shm);
    return 0;
}