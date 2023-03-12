#include<iostream>
#include<vector>
#include<fstream>
#include<string>
#include<cmath>
#include<pthread.h>
#include<queue>
#include<stack>
#include<set>
#include<time.h>
#include<algorithm>
#include<unistd.h>

#define NNODES 37700

// compile using g++ SNS.cpp -o sns -lpthread -mcmodel=medium

using namespace std;

class Action{
    public:
        int user_id;
        int action_id;
        string action_type;
        time_t timestamp;
        Action(int user_id, int action_id, string action_type, time_t timestamp){
            this->user_id = user_id;
            this->action_id = action_id;
            this->action_type = action_type;
            this->timestamp = timestamp;
        }
};

class Node{
    public:
        int id;
        vector<int> neighbors;
        int priority;
        //     Each node U should maintain two queues:
        // ● Wall Queue: Will store the actions generated by the node U herself (see
        // below userSimulator thread function).
        // ● Feed Queue: Will store the actions generated by all neighbors of U (see
        // below readPost and pushUpdate functionalities).
        queue<Action> wallQueue;
        vector<pair<int, Action>> feedQueue;
        int numLikes;
        int numComments;
        int numPosts;
        Node(){
            this->id = -1;
            this->neighbors = vector<int>();
            this->priority = rand()%2;
            numLikes = 0;
            numComments = 0;
            numPosts = 0;
        }
        Node(int id){
            this->id = id;
            this->neighbors = vector<int>();
            this->priority = rand()%2;
            numLikes = 0;
            numComments = 0;
            numPosts = 0;
        }
        void addNeighbor(int id){
            neighbors.push_back(id);
        }
};

// global variables
vector<Node> nodes(NNODES);
int commonNeighbors[NNODES][NNODES];
set<int> changedFeedQueues;
// create a shared queue for userSimulator thread to push actions and pushUpdate threads to pop actions
queue<Action> actionQueue;

pthread_mutex_t ActionQueueMutex;

pthread_mutex_t printMutex;

pthread_mutex_t pushUpdateMutex;
pthread_cond_t pushUpdateCond;

pthread_mutex_t readPostMutex;
pthread_cond_t readPostCond;

void ComputeCommonNeighbors(){
    // this function will compute the number of common neighbors of each pair of nodes
    // and store it in the matrix commonNeighbors
    for(int i=0; i<NNODES; i++){
        for(int j=0; j<NNODES; j++){
            commonNeighbors[i][j] = 0;
        }
    }
    for(int i=0; i<NNODES; i++){
        for(int j=0; j<(int)nodes[i].neighbors.size(); j++){
            int neighbor = nodes[i].neighbors[j];
            for(int k=0; k<(int)nodes[neighbor].neighbors.size(); k++){
                int neighbor_of_neighbor = nodes[neighbor].neighbors[k];
                if (neighbor_of_neighbor != i){
                    commonNeighbors[i][neighbor_of_neighbor]++;
                }
            }
        }
    }
}

void ReadGraph(){
    int edges = 0;
    for(int i=0; i<NNODES; i++){
        nodes[i] = Node(i+1);
    }
    // read the nodes from the musae_git_edges.csv file
    // the first row contains the headers, ignore that
    // after that each row will have 2 columns denoting an edge between the nodes
    ifstream file("musae_git_edges.csv");
    string line;
    // ignoring the headers;
    getline(file, line);
    while(getline(file, line)){
        int node1, node2;
        int pos = line.find(",");
        node1 = stoi(line.substr(0, pos));
        node2 = stoi(line.substr(pos+1, line.length()));
        nodes[node1].addNeighbor(node2);
        nodes[node2].addNeighbor(node1);
        edges++;
    }
    // cout<<"Number of edges: " << edges << endl;
    // cout<<"Number of nodes: " << NNODES << endl;
    file.close();
}

void PushToFeedQueue(Action act, int node_id, pthread_t thread_id){
    // this function will push the action to the feed queue of all the neighbors of the node
    // with id node_id
    pthread_mutex_lock(&printMutex);
    FILE* fp = fopen("sns.log", "a");
    fprintf(fp,"-------------------------------------------------------\n");
    fprintf(fp, "Thread %ld Pushing to Feed Queue the action:-\n", thread_id);
    fprintf(fp, "User_id: %d Action_id: %d Action: %s Timestamp: %s\n", act.user_id, act.action_id, act.action_type.c_str(), ctime(&act.timestamp));
    for(int i=0; i<(int)nodes[node_id].neighbors.size(); i++){
        int neighbor_id = nodes[node_id].neighbors[i];
        nodes[neighbor_id].feedQueue.push_back(make_pair(node_id, act));
        // write this into the log file
        fprintf(fp, "Pushed to Feed Queue of Node: %d\n", neighbor_id);
        // add the neighbor_id to the set of changed feed queues
        changedFeedQueues.insert(neighbor_id);
        // everytime a feed queue is changed, we will signal the readPost thread
        // so that it can reorder the feed queue of the node
        pthread_cond_signal(&readPostCond);
    }
    fprintf(fp,"------------------------------------------------------\n");
    fclose(fp);
    pthread_mutex_unlock(&printMutex);
}

void ReorderFeedQueue(int nodeid){
    // this function will reorder the feed queue of the node with id nodeid
    // based on the priority of the nodes
    // if priority is 1 then order the actions chronologically
    // if priority is 0 then order the actions based on common neighbors of the nodes

    // sort the feed queue based on the priority of the nodes
    if (nodes[nodeid].priority == 1){
        sort(nodes[nodeid].feedQueue.begin(), nodes[nodeid].feedQueue.end(), [](const pair<int, Action>& a, const pair<int, Action>& b){
            return a.second.timestamp < b.second.timestamp;
        });
    }
    else{
        // we have precomputed the number of common neighbors of each pair of nodes
        // and stored it in the matrix commonNeighbors
        // sort the feed queue based on the number of common neighbors, then the nodeid, then the timestamp
        sort(nodes[nodeid].feedQueue.begin(), nodes[nodeid].feedQueue.end(), [](const pair<int, Action>& a, const pair<int, Action>& b){
            if (commonNeighbors[a.first][b.first] != commonNeighbors[b.first][a.first]){
                return commonNeighbors[a.first][b.first] > commonNeighbors[b.first][a.first];
            }
            else if (a.first != b.first){
                return a.first < b.first;
            }
            else{
                return a.second.timestamp < b.second.timestamp;
            }
        });
    }
    
}

void *userSimulatorFn(void *arg){
    // this function will be executed by the userSimulator thread
    // it will generate random actions for 100 random users 
    // and push them to the wall queue of the user node
    // and push them to a queue monitored by pushUpdate threads
    while(true){
        // We will also log these statements in sns.log file
        pthread_mutex_lock(&printMutex);
        FILE* fp = fopen("sns.log", "a");
        cout<<"-------------------------------------------------------------------------"<<endl;
        // write this into the log file
        fprintf(fp, "-------------------------------------------------------------------------\n");
        cout<<"User Simulator Thread Started: " << pthread_self()<<endl;
        fprintf(fp, "User Simulator Thread Started: %ld\n", pthread_self());
        // choose 100 random nodes from the graph
        // TODO: change this to 100
        for(int i=0;i<5;i++){
            int node_id = rand() % NNODES + 1;
            fprintf(fp, "\nChosen Node: %d Neighbors: %d\n", node_id, (int)nodes[node_id].neighbors.size());
            int n = floor(log2(nodes[node_id].neighbors.size()));
            n = 10*(n+1);
            fprintf(fp, "Number of actions to be generated: %d\n", n);
            for(int j=0;j<n;j++){
                int action_no = rand() % 3;
                int action_id;
                string action_type;
                if (action_no == 0){
                    action_type = "Post";
                    nodes[node_id].numPosts++;
                    action_id = nodes[node_id].numPosts;
                }
                else if (action_no == 1) {
                    action_type = "Comment";
                    nodes[node_id].numComments++;
                    action_id = nodes[node_id].numComments;
                }
                else{
                    action_type = "Like";
                    nodes[node_id].numLikes++;
                    action_id = nodes[node_id].numLikes;
                }
                Action action(node_id, action_id, action_type, time(NULL));
                fprintf(fp, "Action %d generated:\n", j);
                fprintf(fp, "User_id: %d Action_id: %d Action: %s Timestamp: %s", node_id, action_id, action_type.c_str(), ctime(&action.timestamp));
                // push it to the wall queue of the user node
                nodes[node_id].wallQueue.push(action);
                // push it to a queue monitored by pushUpdate threads
                actionQueue.push(action);
                // everytime an action is pushed into the actionQueue, we will wake up one of the pushUpdate threads
                pthread_cond_signal(&pushUpdateCond);
            }
        } 
        cout<<"\nUser Simulator Thread Finished: " << pthread_self()<<endl;
        fprintf(fp, "\nUser Simulator Thread Finished: %ld\n", pthread_self());
        cout<<"-------------------------------------------------------------------------"<<endl;
        fprintf(fp, "-------------------------------------------------------------------------\n");
        fclose(fp);
        pthread_mutex_unlock(&printMutex);
        // sleep for 2 minutes;
        sleep(120);
    }    
}

void *readPostFn(void *arg){
    while(true){
        pthread_mutex_lock(&readPostMutex);
        // To monitor the feed queues we will use a condition variable
        while(changedFeedQueues.empty()){
            pthread_cond_wait(&readPostCond, &readPostMutex);
        }
        // choose one of the nodes from set changedFeedQueues
        int node_id = *changedFeedQueues.begin();
        // remove the node from the set changedFeedQueues
        changedFeedQueues.erase(node_id);
        pthread_mutex_unlock(&readPostMutex);
        // read the feed queue of the node
        ReorderFeedQueue(node_id);
        // write the feed queue to the log file
        pthread_mutex_lock(&printMutex);
        FILE* fp = fopen("sns.log", "a");
        cout<<"-------------------------------------------------------------------------"<<endl;
        fprintf(fp, "-------------------------------------------------------------------------\n");
        cout<<"Read Post Thread Started: " << pthread_self()<<endl;
        fprintf(fp, "Read Post Thread Started: %ld\n", pthread_self());
        for(int i=0;i<(int)nodes[node_id].feedQueue.size();i++){
            // read the action
            Action action = nodes[node_id].feedQueue[i].second;            
            fprintf(fp, "I have read Action: %d %s by user %d at %s from feed of user %d\n", action.action_id, action.action_type.c_str(), action.user_id, ctime(&action.timestamp), node_id);
        }
        cout<<"Read Post Thread Finished: " << pthread_self()<<endl;
        fprintf(fp, "Read Post Thread Finished: %ld\n", pthread_self());
        cout<<"-------------------------------------------------------------------------"<<endl;
        fprintf(fp, "-------------------------------------------------------------------------\n");
        fclose(fp);
        pthread_mutex_unlock(&printMutex);
        // clear the feed queue of the node
        nodes[node_id].feedQueue.clear();
    }
}

void *pushUpdateFn(void *arg){
    // To monitor the actionQueue we will use a condition variable
    // we will also use a mutex to lock the condition variable
    // so that only one thread can pop an action at a time
    while(true){
        // wait for the condition variable to be set
        pthread_mutex_lock(&pushUpdateMutex);
        // wait for the condition variable to be set
        while(actionQueue.empty()){
            pthread_cond_wait(&pushUpdateCond, &pushUpdateMutex);
        }
        // pop the action from the queue
        Action action = actionQueue.front();
        actionQueue.pop();
        pthread_mutex_unlock(&pushUpdateMutex);
        // push the action to the feed queues of the neighbours
        PushToFeedQueue(action, action.user_id, pthread_self());
        }

}

int main(int argc, char *argv[]){
    // initializing the global variables
    changedFeedQueues.clear();
    actionQueue = queue<Action>();
    pthread_mutex_init(&readPostMutex, NULL);
    pthread_mutex_init(&pushUpdateMutex, NULL);
    pthread_cond_init(&readPostCond, NULL);
    pthread_cond_init(&pushUpdateCond, NULL);
    pthread_mutex_init(&printMutex, NULL);

    // The main thread will be responsible for reading the graph
    // it will then create a userSimulator thread
    // a pool of 10 readPost threads and another pool of 25 pushUpdate threads and wait for the threads
    srand(time(NULL));
    ReadGraph();
    pthread_t userSimulator;
    pthread_t readPost[10];
    pthread_t pushUpdate[25];

    ComputeCommonNeighbors();

    //create the userSimulator thread
    // we create an empty log file sns.log
    FILE* fp = fopen("sns.log", "w");
    fclose(fp);
    pthread_create(&userSimulator, NULL, userSimulatorFn, NULL);

    //create the readPost threads
    for(int i=0;i<10;i++){
        pthread_create(&readPost[i], NULL, readPostFn,NULL);
    }

    //create the pushUpdate threads
    for(int i=0;i<25;i++){
        pthread_create(&pushUpdate[i], NULL, pushUpdateFn, NULL);
    }
    
    // wait for the readPost threads to finish
    for(int i=0;i<10;i++){
        pthread_join(readPost[i], NULL);
    }

    // wait for the pushUpdate threads to finish
    for(int i=0;i<25;i++){
        pthread_join(pushUpdate[i], NULL);
    }

    //wait for the userSimulator thread to finish
    pthread_join(userSimulator, NULL);

    return 0;
}