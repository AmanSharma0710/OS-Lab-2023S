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

#include<chrono>
using namespace std::chrono;

string filename = "./output/consumer_";
int N;
set<int> source_nodes;
set<int> new_source_nodes;
vector<int> distances_global;
vector<int> parent_global;

void run_dijstra(vector<vector<int>> &edges, int n, int m){
    // We run dijstra's algorithm on the graph
    // We store the shortest path from start to every other node in the array dist
    vector<int> dist(n, INT_MAX);
    vector<int> parent(n, -1);
    for(auto it: source_nodes){
        dist[it] = 0;
    }
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    for(auto it: source_nodes){
        pq.push(make_pair(dist[it], it));
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
        fprintf(fp, "%d: ", i);
        if(dist[i] == INT_MAX){
            fprintf(fp, "No path found\n");
            continue;
        }
        if(source_nodes.find(i) != source_nodes.end()){
            fprintf(fp, "Source node\n");
            continue;
        }
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

void run_optimised(vector<vector<int>> &edges, int n, int m, int new_nodes){
    // Add the new nodes to the graph, i.e., for all the new added nodes, distance is min global_distance to all neighbours+1
    for(int i=n-new_nodes; i<n; i++){
        int min_dist = INT_MAX, parent_curr = -1;
        for(int j=0; j<edges[i].size(); j++){
            int v = edges[i][j];
            if(v>=n-new_nodes) continue;
            if(distances_global[v] < min_dist){
                min_dist = distances_global[v];
                parent_curr = v;
            }
        }
        if(min_dist == INT_MAX) continue;
        distances_global[i] = min_dist + 1;
        parent_global[i] = parent_curr;
    }

    // We run dijstra's algorithm on the graph using only the newly added nodes as the source nodes
    vector<int> dist(n, INT_MAX);
    vector<int> parent(n, -1);
    for(auto it: source_nodes){ // change to source_nodes
        dist[it] = 0;
    }
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    vector<bool> shorter_path_found(n, false);
    for(auto it: new_source_nodes){
        pq.push(make_pair(dist[it], it));
        shorter_path_found[it] = true;
    }
    while(!pq.empty()){
        pair<int, int> p = pq.top();
        pq.pop();
        int u = p.second;
        int w = p.first;
        if(w > dist[u]) continue;
        if(w > distances_global[u]) continue;   //This is the optimisation
        for(int i=0; i<edges[u].size(); i++){
            int v = edges[u][i];
            if((dist[v] > dist[u] + 1) && (dist[u]+1 < distances_global[v])){   //This is the optimisation
                dist[v] = dist[u] + 1;
                parent[v] = u;
                shorter_path_found[v] = true;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    // We update the parents for the nodes which have a shorter path
    for(int i=0; i<n; i++){
        if(shorter_path_found[i]){
            parent_global[i] = parent[i];
            distances_global[i] = dist[i];
        }
    }
    // We write the shortest path from start to every other node to the file
    FILE *fp = fopen(filename.c_str(), "a");
    if(fp == NULL){
        perror("fopen");
        exit(1);
    }
    for(int i=0; i<n; i++){
        fprintf(fp, "%d: ", i, distances_global[i]);
        if(distances_global[i] == INT_MAX){
            fprintf(fp, "No path found\n");
            continue;
        }
        if(source_nodes.find(i) != source_nodes.end()){
            fprintf(fp, "Source node\n");
            continue;
        }
        int u = i;
        vector<int> path(0);
        while(u != -1){
            path.push_back(u);
            u = parent_global[u];
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
    int consumer_number = atoi(argv[1]);

    //Initialise global variables
    N = 0;
    source_nodes.clear();
    distances_global.clear();
    parent_global.clear();
    while(1){
        // We sleep for 30 seconds
        sleep(30);
        new_source_nodes.clear();
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
        int new_nodes = n - N;
        // If we are the ith consumer, we add 1/10th of the new nodes to the source nodes
        // We add nodes from i*n/10 to ((i+1)*n/10)-1 to the source nodes
        int start = (consumer_number*new_nodes)/10;
        int end = ((consumer_number+1)*new_nodes)/10;
        if(consumer_number == 9){
            end = new_nodes-1;
        }
        else end--;

        for(int i=start; i<=end; i++){
            source_nodes.insert(N+i);
            new_source_nodes.insert(N+i);
        }
        // For all the new nodes, add the distance from the source nodes as INT_MAX
        for(int i=N; i<n; i++){
            distances_global.push_back(INT_MAX);
            parent_global.push_back(-1);
        }

        // Update the value of N
        N = n;

        // If 2 arguments are passed, then we call the optimised version of the algorithm
        bool optimised = false;
        if(argc == 3){
            optimised = true;
        }
        // Now we call the algorithm
        if(optimised) run_optimised(edges, n, m, new_nodes);
        else run_dijstra(edges, n, m);
        edges.clear();
    }
    // Detach from the shared memory and exit
    shmdt(shm);
    return 0;
}