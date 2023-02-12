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
#include <string>
#include <climits>
#include <queue>
#include <sys/stat.h>
using namespace std;

string filename = "./output/consumer_";

void run_dijstra(vector<vector<int>> &edges, int n, int m, int start, int end){
    // We run dijstra's algorithm on the graph
    // We store the shortest path from start to every other node in the array dist
    vector<int> dist(n, INT_MAX);
    vector<int> parent(n, -1);
    for(int i=start; i<=end; i++) dist[i] = 0;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    for(int i=start; i<=end; i++){
        pq.push(make_pair(0, i));
    }
    while(!pq.empty()){
        pair<int, int> p = pq.top();
        pq.pop();
        int u = p.second;
        int w = p.first;
        if(w > dist[u]) continue;
        for(int i=0; i<edges[u].size(); i++){
            int v = edges[u][i];
            if(dist[v] > dist[u] + 1){
                dist[v] = dist[u] + 1;
                parent[v] = u;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    // We write the shortest path from start to every other node to the file
    FILE *fp = fopen(filename.c_str(), "a");
    if(fp == NULL){
        perror("fopen");
        exit(1);
    }
    for(int i=0; i<n; i++){
        if(dist[i] == INT_MAX) continue;
        if((i >= start) && (i <= end)) continue;
        fprintf(fp, "%d: ", i);
        int u = i;
        vector<int> path(0);
        while(u != -1){
            path.push_back(u);
            u = parent[u];
        }
        for(int j=path.size()-1; j>=0; j--){
            fprintf(fp, "%d", path[j]);
            if(j != 0) fprintf(fp, "->");
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "------------------------------------------------------------------------------------------\n");
    fclose(fp);
    return;
}

void run_optimised(vector<vector<int>> &edges, int n, int m, int start, int end){   //TODO: Implement this function
    cout<<"Optimised"<<endl;
    // We run the optimised algorithm on the graph
    // We store the shortest path from start to every other node in the array dist
    vector<int> dist(n, INT_MAX);
    vector<int> order(0);
    for(int i=start; i<end; i++) dist[i] = 0;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    for(int i=start; i<end; i++){
        pq.push(make_pair(0, i));
        order.push_back(i);
    } 
    while(!pq.empty()){
        pair<int, int> p = pq.top();
        pq.pop();
        int u = p.second;
        int w = p.first;
        if(w > dist[u]) continue;
        for(int i=0; i<edges[u].size(); i++){
            int v = edges[u][i];
            if(dist[v] > dist[u] + 1){
                dist[v] = dist[u] + 1;
                pq.push(make_pair(dist[v], v));
                order.push_back(v);
            }
        }
    }
    // We write the shortest path from start to every other node to the file
    FILE *fp = fopen(filename.c_str(), "a");
    if(fp == NULL){
        perror("fopen");
        exit(1);
    }
    for(int i=0; i<order.size(); i++){
        fprintf(fp, "%d ", order[i]);
    }
    fprintf(fp, "\n");
    fclose(fp);
    return;
}


int main(int argc, char *argv[]){
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
    // If the directory output does not exist, we create it
    if(access("./output", F_OK) == -1){
        mkdir("./output", 0777);
    }
    // Add the consumer number to the filename
    filename += argv[1];
    filename += ".txt";
    while(1){
        // We sleep for 30 seconds
        sleep(30);
        int n, m;
        // We read the current graph from the shared memory
        // We read the number of vertices and edges from the shared memory
        // We read the edges from the shared memory
        int *shm_int = (int*) shm;
        n = shm_int[0];
        m = shm_int[1];
        vector<vector<int>> edges(n);
        for(int i=0; i<m; i++){
            edges[shm_int[2*i+2]].push_back(shm_int[2*i+3]);
            edges[shm_int[2*i+3]].push_back(shm_int[2*i+2]);
        }
        // first argument passed is the number of consumer
        // If we are the ith consumer, we take the nodes from i*n/10 to ((i+1)*n/10)-1 of the graph
        int consumer_number = atoi(argv[1]);
        // If 2 arguments are passed, then we call the optimised version of the algorithm
        bool optimised = false;
        if(argc == 3){
            optimised = true;
        }
        int start = (consumer_number*n)/10;
        int end = ((consumer_number+1)*n)/10;
        if(consumer_number == 9){
            end = n-1;
        }
        else end--;
        // Now we call the algorithm
        if(optimised) run_optimised(edges, n, m, start, end);
        else run_dijstra(edges, n, m, start, end);
        edges.clear();
    }
    // Detach from the shared memory and exit
    shmdt(shm);
    return 0;
}