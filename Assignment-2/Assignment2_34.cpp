#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <glob.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <termios.h>
#include <vector>
#include <set>
#include <stack>
#include <sys/inotify.h>
#include <sys/select.h>
#include <map>
#include <string>
#include <time.h>
#include <readline/readline.h>
#include <queue>

using namespace std;

const string WHITESPACE = " \n\r\t\f\v";

class commands{
    public:
    string command;
    vector<string> args;
    string output_file, input_file;
};

vector<commands> parse(char *user_input, int &background, int &valid_input){
    background = 0;
    valid_input = 1;
    vector<commands> new_procs;
    char* input = strdup(user_input);
    char *complete_command, *token;
    const char *delim_pipe = "|", *delim_space = " ";
    while((complete_command = strsep(&input, delim_pipe))!=NULL){
        if(strlen(complete_command) == 0) continue;
        commands new_command;
        new_command.command = "";
        new_command.args.clear();
        new_command.input_file = "";
        new_command.output_file = "";
        int read_input = 0, read_output = 0;
        while((token = strsep(&complete_command, delim_space))!=NULL){
            if(strlen(token) == 0) continue;
            if(background){
                valid_input = 0;
                break;
            }
            if(strcmp(token, "&") == 0){
                background = 1;
                continue;
            }
            if(new_command.command == ""){
                new_command.command = token;
            }
            else if(token[0] == '<'){
                if(new_command.input_file != ""){
                    valid_input = 0;
                    break;
                }
                if(strlen(token) > 1){
                    new_command.input_file = (token + 1);
                    continue;
                }
                read_input = 1;
            }
            else if(token[0] == '>'){
                if(new_command.output_file != ""){
                    valid_input = 0;
                    break;
                }
                if(strlen(token) > 1){
                    new_command.output_file = (token + 1);
                    continue;
                }
                read_output = 1;
            }
            else if(read_input){
                new_command.input_file = token;
                read_input = 0;
            }
            else if(read_output){
                new_command.output_file = token;
                read_output = 0;
            }
            else{
                new_command.args.push_back(token);
            }
        }
        new_procs.push_back(new_command);
    }
    return new_procs;
}

class History{
    //history is saved in a file called .terminalhist
    public:
        static const string hist_file;
        static const int hist_size;
        //history is a deque of strings, with the most recent command at the top
        //we store a global and local history
        static deque<string> history_global;
        stack<string> up, down;
        History(){
            ifstream hist(hist_file.c_str());
            string line;
            while(getline(hist, line)){
                history_global.push_front(line);
            }
            hist.close();
        }
        void saveHistory(){
            ofstream hist(hist_file.c_str());
            for(int i = (int)history_global.size() - 1; i >= 0; i--){
                hist << history_global[i] << endl;
            }
            hist.close();
        }
        void addHistory(char *user_input){
            //if the first character is a space or if the command is empty, we don't add it to the history
            if(user_input[0] == ' ' || strlen(user_input) == 0) return;
            //if the command is the same as the last command, we don't add it to the history
            if(history_global.size() > 0 && history_global.front() == user_input) return;
            //if the history is full, we remove the last command
            if(history_global.size() == hist_size){
                history_global.pop_back();
            }
            history_global.push_front(user_input);
        }
        void syncHistory(){
            //we empty the local history
            while(!up.empty()) up.pop();
            while(!down.empty()) down.pop();
            //we add the global history to the local history
            for(int i = (int)history_global.size() - 1; i >= 0; i--){
                up.push(history_global[i]);
            }
            //we add an empty string to the local history
            up.push("");
        }
        void goUpInHistory(){
            //if the local history is empty or has just one command, we don't do anything
            if(up.size() <= 1) return;
            //we move the last command to the down stack
            down.push(up.top());
            //we remove the last command from the up stack
            up.pop();
        }
        void goDownInHistory(){
            //if we are at the last command, we don't do anything
            if(down.size() == 0) return;
            //we move the last command to the up stack
            up.push(down.top());
            //we remove the last command from the down stack
            down.pop();
        }
        string getHistory(){
            assert(up.size() > 0);
            return up.top();
        }
        void updateHistory(char *user_input){
            assert(up.size() > 0);
            //we remove the last command from the up stack
            up.pop();
            //we add the new command to the up stack
            up.push(user_input);
        }
};

const string History::hist_file = ".terminalhist";
const int History::hist_size = 1000;
deque<string> History::history_global;
History hist;


int goUpInHistory(int count, int key){
    //save the current command in the history using readlines
    char* current_line = rl_line_buffer;
    hist.updateHistory(current_line);
    //go up in the history
    hist.goUpInHistory();
    //get the new command
    string new_command = hist.getHistory();
    //replace the current command with the new command
    rl_replace_line(new_command.c_str(), 0);
    //move the cursor to the end of the line
    rl_point = rl_end;
    //redraw the line
    rl_redisplay();
    return 0;
}

int goDownInHistory(int count, int key){
    char* current_line = rl_line_buffer;
    hist.updateHistory(current_line);
    hist.goDownInHistory();
    string new_command = hist.getHistory();
    rl_replace_line(new_command.c_str(), 0);
    rl_point = rl_end;
    rl_redisplay();
    return 0;
}


void Execute_pwd(commands comm){
    char *cwd = (char *)NULL;
    cwd = getcwd(cwd, 0);
    if(cwd == (char *)NULL){
        perror("getcwd");
        exit(1);
    }
    if (comm.output_file != ""){
        int fd = open(comm.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1){
            perror("Error in opening file.");
            exit(1);
        }
        write(fd, cwd, strlen(cwd));
        close(fd);
    }
    else{
        cout<<cwd<<endl;
    }
}

void Execute_cd(commands comm){
    if (comm.args.size() == 0){
        chdir(getenv("HOME"));
    }
    else{
        if (chdir(comm.args[0].c_str()) == -1){
            perror("Error in changing directory.");
        }
    }
}

void Execute_sb(commands comm){
    int sug_flag = 0,pid_flag=1;
    vector<int> ancestors;

    if (comm.args.size() == 0){
        cout<<"No arguments given. Give PID of process."<<endl;
        return;
    }
    else{
        for(int i = 0; i < comm.args.size(); i++){
            if(comm.args[i] == "-suggest"){
                sug_flag = 1;
                continue;
            }
            else
                pid_flag--;
            if(pid_flag <0)
                continue;
            int pid = atoi(comm.args[i].c_str());
            if (pid == 0){
                cout<<"Invalid PID."<<endl;
                return;
            }
            //print ancestors of pid
            ancestors.push_back(pid);
            int parent =pid;
            cout<<"Ancestor chain: "<<pid<<" ";
            while(parent!=1)
            {
                // open "/proc/<pid>/stat"
                string proc_stat_path = "/proc/"+to_string(parent)+"/stat";
                FILE *fh = fopen(proc_stat_path.c_str(), "r");
                if (fh == NULL) {
                    if(parent==0)
                    {
                        cout<<"1";
                        parent=1;
                        continue;
                    }
                    cout<<"Invalid PID."<<endl;
                    return;
                }

                //get parent value from fourth field
                char *line = NULL;
                size_t len = 0;
                getline(&line, &len, fh);
                char *pch = strtok(line, " ");
                int count = 0;
                while(pch != NULL){
                    count++;
                    if(count == 4){
                        parent = atoi(pch);
                        break;
                    }
                    pch = strtok(NULL, " ");
                }
                ancestors.push_back(parent);

                // close "/proc/<pid>/stat"
                fclose(fh);

                // print parent
                cout<<parent<<" ";
            }
            cout<<endl;
        }
    }
    if(pid_flag == 1)
    {
        cout<<"No PID given. Give PID of process."<<endl;
        return;
    }
    if(sug_flag == 1){
        //suggest malware among ancestors

        if(ancestors.size() <=2){
            cout<<"Suggested Malware PID: "<<ancestors[0]<<endl;
            return;
        }

        //remove init and scheduler process from ancestors (part of our heuristic)
        ancestors.pop_back();
        ancestors.pop_back();

        int child_count[ancestors.size()];
        int child_count_non_recursive[ancestors.size()];
        long long int time_sum[ancestors.size()];
        long long int min_time = 1e18, min_id=-1;

        //find number of child processes of each ancestor
        for(int i = 0; i < ancestors.size(); i++){
            vector<int> direct_children;
            queue<int> q;
            child_count[i] = 0;
            child_count_non_recursive[i] = 0;
            time_sum[i] = 0;
            q.push(ancestors[i]);
            int rec=0;
            while(!q.empty())
            {
                int pid = q.front();
                q.pop();

                if(pid==0)
                    continue;

                string proc_stat_path = "/proc/"+to_string(pid)+"/task"+"/"+to_string(pid)+"/children";
                FILE *fh = fopen(proc_stat_path.c_str(), "r");
                if (fh == NULL) {
                    continue;
                }
                char *line = NULL;
                size_t len = 0;
                getline(&line, &len, fh);
                char *pch = strtok(line, " ");
                while(pch != NULL){
                    child_count[i]++;
                    if(rec==0)
                    {
                        child_count_non_recursive[i]++;
                        direct_children.push_back(atoi(pch));
                    }
                    if(atoi(pch)!=0)
                        q.push(atoi(pch));

                    pch = strtok(NULL, " ");
                }
                rec=1;
                fclose(fh);
            }
            //find time spent in direct children and grandchildren
            for(int j=0;j<direct_children.size();j++)
            {
                if(direct_children[j]==0)
                    continue;
                string proc_stat_path = "/proc/"+to_string(direct_children[j])+"/stat";
                FILE *fh = fopen(proc_stat_path.c_str(), "r");
                if (fh == NULL) {
                    continue;
                }
                char *line = NULL;
                size_t len = 0;
                getline(&line, &len, fh);
                char *pch = strtok(line, " ");
                int count = 0;
                while(pch != NULL){
                    count++;
                    if(count == 22){
                        time_sum[i] -= atoi(pch);
                        break;
                    }
                    pch = strtok(NULL, " ");
                }
                fclose(fh);

                vector<int> grand_children;

                string proc_stat_path1 = "/proc/"+to_string(direct_children[j])+"/task"+"/"+to_string(direct_children[j])+"/children";
                FILE *fh1 = fopen(proc_stat_path1.c_str(), "r");
                if (fh1 == NULL) {
                    continue;
                }
                char *line1 = NULL;
                size_t len1 = 0;
                getline(&line1, &len1, fh1);
                char *pch1 = strtok(line1, " ");
                while(pch1 != NULL){
                    if(atoi(pch)!=0)
                        grand_children.push_back(atoi(pch1));
                    pch1 = strtok(NULL, " ");
                }
                fclose(fh1);

                for(int k=0;k<grand_children.size();k++)
                {
                    if(grand_children[k]==0)
                        continue;
                    string proc_stat_path2 = "/proc/"+to_string(grand_children[k])+"/stat";
                    FILE *fh2 = fopen(proc_stat_path2.c_str(), "r");
                    if (fh2 == NULL) {
                        continue;
                    }
                    char *line2 = NULL;
                    size_t len2 = 0;
                    getline(&line2, &len2, fh2);
                    char *pch2 = strtok(line2, " ");
                    int count2 = 0;
                    while(pch2 != NULL){
                        count2++;
                        if(count2 == 22){
                            time_sum[i] -= atoi(pch2);
                            break;
                        }
                        pch2 = strtok(NULL, " ");
                    }
                    fclose(fh2);
                }

            }

            // cout<<"Number of direct child processes, total children processes of "<<ancestors[i]<<" are: "<<child_count_non_recursive[i]<<","<<child_count[i]<<endl;
            // cout<<"Time spent in direct children and grandchildren of "<<ancestors[i]<<" is: "<<time_sum[i]<<endl;
            if(time_sum[i]<min_time)
            {
                min_time = time_sum[i];
                min_id = i;
            }
        }
        
        cout<<"Suggested Malware PID:"<<ancestors[min_id]<<endl;
    }
}

void Execute_Command(commands comm, int background, int forkreq){
    string command = comm.command;
    //We expand the arguments to match wildcards
    vector<string> args;
    for(int i = 0; i < comm.args.size(); i++){
        glob_t glob_result;
        int return_value = glob(comm.args[i].c_str(), GLOB_TILDE | GLOB_NOMAGIC, NULL, &glob_result);
        if(return_value != 0){
            args.push_back(comm.args[i]);
            continue;
        }
        for(unsigned int j = 0; j < glob_result.gl_pathc; j++){
            args.push_back(string(glob_result.gl_pathv[j]));
        }
        globfree(&glob_result);
    }
    comm.args = args;
    //We check if the command is a builtin command
    if (command == "exit"){
        hist.saveHistory();
        printf("Exiting the shell.\n");
        exit(0);
    }
    else if (command == "pwd"){
        Execute_pwd(comm);
    }
    else if (command == "cd"){
        Execute_cd(comm);
    }
    else if (command == "sb"){
        Execute_sb(comm);
    }
    else{
        int pid = 0;
        // if forkreq is 1, we fork the process i.e. run the command in a child process
        if (forkreq) pid = fork();
        // we run the command in the child process
        if (pid == 0){
            // setting the input and output files in case of redirection
            if (comm.input_file != ""){
                int fd = open(comm.input_file.c_str(), O_RDONLY);
                if (fd == -1){
                    perror("Error in opening file.");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (comm.output_file != ""){
                int fd = open(comm.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1){
                    perror("Error in opening file.");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            char *args[comm.args.size() + 2];
            args[0] = (char *)command.c_str();
            for (int i = 0; i < comm.args.size(); i++){
                args[i + 1] = (char *)comm.args[i].c_str();
            }
            // the last argument must be NULL
            args[comm.args.size() + 1] = NULL;
            if (execvp(args[0], args) == -1){
                perror("Error in executing command.");
                exit(1);
            }
        }
        else if (pid > 0){
            // if the command is not to be run in the background, we wait for the child to finish
            if (!background){
                waitpid(pid, NULL, 0);
            }
        }
        // error in forking
        else{
            perror("Error in forking.");
            exit(1);
        }
    }
}

sigjmp_buf env;

void Shell(){
    char *cwd = (char *)NULL;
    char *user_input = (char *)NULL;

    while(1){   //we start the shell
        while(sigsetjmp(env, 1)!=0);    //we set the jump point
        //sync the history
        hist.syncHistory();

        if(cwd != (char *)NULL){
            free(cwd);
            cwd = (char *)NULL;
        }
        cwd = getcwd(cwd, 0);  //get the current working directory
        if(cwd == (char *)NULL){
            perror("getcwd");
            exit(1);
        }
        char *prompt = (char *)NULL;
        prompt = (char *)malloc(strlen(cwd) + 3);
        strcpy(prompt, cwd);
        strcat(prompt, "$ ");   //create the prompt

        if(user_input != (char *)NULL){
            free(user_input);
            user_input = (char *)NULL;
        }
        user_input = readline(prompt);  //read the user input
        if(user_input == (char *)NULL){
            perror("readline");
            exit(1);
        }

        //add the command to the history
        hist.addHistory(user_input);

        //Print the user input and length for debugging
        // printf("User input: %s.\n", user_input);
        // printf("Length: %d\n", strlen(user_input));
        int background, valid_input;
        vector<commands> new_procs = parse(user_input, background, valid_input);  //parse the user input
        if(!valid_input){
            printf("Invalid input.\n");
            continue;
        }

        // print all commands parsed 
        // for(int i = 0; i < new_procs.size(); i++){
        //     cout<<"command "<<i<<": ";
        //     cout << new_procs[i].command << endl;
        //     for(int j = 0; j < new_procs[i].args.size(); j++){
        //         cout<<"arg "<<j<<": ";
        //         cout << new_procs[i].args[j] << endl;
        //     }
        //     cout <<"Input File:"<< new_procs[i].input_file << endl;
        //     cout <<"Output File:"<< new_procs[i].output_file << endl;
        //     cout<<endl;
        // }
        // exit(0);

        // no new command
        if (new_procs.size() == 0){
            continue;
        }
        int n = new_procs.size();

        // if there is only one command, we execute it
        if (n == 1) {
            Execute_Command(new_procs[0], background, 1);
            continue;
        }
        // if there are multiple commands, we create pipes and execute them
        else {
            int pipes[n - 1][2];
            for (int i = 0; i < n - 1; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("Error in creating pipe.");
                    exit(1);
                }
            }
            for (int i = 0; i < n; i++) {
                int pid = fork();
                if (pid == 0) {
                    // setting the input and output files in case of redirection
                    if (new_procs[i].input_file != ""){
                        int fd = open(new_procs[i].input_file.c_str(), O_RDONLY);
                        if (fd == -1){
                            perror("Error in opening file.");
                            exit(1);
                        }
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                    }
                    if (new_procs[i].output_file != ""){
                        int fd = open(new_procs[i].output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd == -1){
                            perror("Error in opening file.");
                            exit(1);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                    // setting the input and output pipes
                    if (i != 0) {
                        dup2(pipes[i - 1][0], STDIN_FILENO);
                    }
                    if (i != n - 1) {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }

                    for(int j=0;j<n-1;j++){
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    // executing the command
                    Execute_Command(new_procs[i], background, 0);
                }
                else if (pid > 0){
                    if (i != 0) {
                        close(pipes[i - 1][0]);
                    }
                    if (i != n - 1) {
                        close(pipes[i][1]);
                    }
                    if (!background){
                        waitpid(pid, NULL, 0);
                    }
                }
                else{
                    perror("Error in forking.");
                    exit(1);
                }
                
            }
            // closing the pipes
            for (int i = 0; i < n - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            // // waiting for all the children to finish
            // for (int i = 0; i < n; i++) {
            //     int status;
            //     wait(&status);
            // }
        }
    }
    return ;
}

void handler_sigint(int signum){
    // print a new line
    printf("\n");
    // jump to the shell prompt
    siglongjmp(env, 1);
    return;
}


// If the user presses "Ctrl - z" while a program is executing, the program
// execution should move to the background and the shell prompt should
// reappear.
void handler_sigtstp(int signum){
    // move the running process to the background
    int pid = getpid();
    printf("Process %d moved to the background.\n", pid);
    raise(SIGSTOP);
    return;
}

signed main(){
    struct sigaction signalint;
    signalint.sa_handler = &handler_sigint;
    signalint.sa_flags = SA_RESTART;
    sigaction(SIGINT, &signalint, NULL);

    struct sigaction signaltstp;
    signaltstp.sa_handler = &handler_sigtstp;
    signaltstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &signaltstp, NULL);

    rl_bind_keyseq("\\e[A", goUpInHistory);
    rl_bind_keyseq("\\e[B", goDownInHistory);

    Shell();
    return 0;
}
