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
#include <set>

using namespace std;


int main(){
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
    while(1){
        // We sleep for 50 seconds
        sleep(50);
        int n, m;
        // We read the current graph from the shared memory
        // We read the number of vertices and edges from the shared memory
        // We read the edges from the shared memory
        int *shm_int = (int*) shm;
        n = shm_int[0];
        m = shm_int[1];
        vector<pair<int, int>> edges;
        for(int i=0; i<m; i++){
            edges.push_back(make_pair(shm_int[2*i+2], shm_int[2*i+3]));
        }
        // Now we have the edges in the vector, and the number of vertices and edges
        // We need to add new vertices
        srand(time(NULL));
        int new_vertices = rand()%21 + 10;
        // For every new vertex, we add edges to existing vertices with probability proportional to their degree
        vector<int> all_ends;
        for(int j=0; j<edges.size(); j++){
            all_ends.push_back(edges[j].first);
            all_ends.push_back(edges[j].second);
        }
        for(int i=0; i<new_vertices; i++){
            int new_vertex = n+i;
            int new_edges = rand()%20 + 1;
            // Randomly sample vertices from all_ends until we have new_edges vertices to connect to the new vertex
            set<int> sampled;
            while(sampled.size() < new_edges){
                int idx = rand()%all_ends.size();
                sampled.insert(all_ends[idx]);
            }
            for(auto it=sampled.begin(); it!=sampled.end(); it++){
                edges.push_back(make_pair(new_vertex, *it));
            }
        }
        // Now we have the new graph in the vector edges
        // We need to write the new graph to the shared memory
        shm_int[0] = n+new_vertices;
        shm_int[1] = edges.size();
        for(int i=0; i<edges.size(); i++){
            shm_int[2*i+2] = edges[i].first;
            shm_int[2*i+3] = edges[i].second;
        }
        // Graph is written to the shared memory
    }
    // Detach from the shared memory and exit
    shmdt(shm);
    return 0;
}