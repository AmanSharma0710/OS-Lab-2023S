#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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


signed main(){

    //make the key bindings to go up and down in the history
    rl_bind_keyseq("\\e[A", goUpInHistory);
    rl_bind_keyseq("\\e[B", goDownInHistory);


    char *cwd = (char *)NULL;
    char *user_input = (char *)NULL;

    while(1){   //we start the shell
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
            cout<<"Invalid input"<<endl;
            continue;
        }

        // //print all commands parsed 
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

        if(new_procs.size() == 0) continue;  //if the user input is empty, continue
        else if(new_procs.size()>1) //pipe multiple commands
        {
            int pipes[new_procs.size()-1][2];
            for(int i=0;i<new_procs.size()-1;i++)
            {
                pipe(pipes[i]);
            }

            for(int i=0;i<new_procs.size();i++)
            {
                int pid = fork();
                if(pid == 0)
                {
                    if(i==0)
                    {
                        if(new_procs[i].output_file.size() != 0){
                            int fd = open(new_procs[i].output_file.c_str(), O_WRONLY | O_CREAT, 0666);
                            dup2(fd, 1);
                            close(fd);
                        }
                        if(new_procs[i].input_file.size() != 0){
                            int fd = open(new_procs[i].input_file.c_str(), O_RDONLY);
                            dup2(fd, 0);
                            close(fd);
                        }
                        dup2(pipes[i][1], 1);
                        
                    }
                    else if(i==new_procs.size()-1)
                    {
                        if(new_procs[i].output_file.size() != 0){
                            int fd = open(new_procs[i].output_file.c_str(), O_WRONLY | O_CREAT, 0666);
                            dup2(fd, 1);
                            close(fd);
                        }
                        if(new_procs[i].input_file.size() != 0){
                            int fd = open(new_procs[i].input_file.c_str(), O_RDONLY);
                            dup2(fd, 0);
                            close(fd);
                        }
                        dup2(pipes[i-1][0], 0);

                    }
                    else
                    {
                        if(new_procs[i].output_file.size() != 0){
                            int fd = open(new_procs[i].output_file.c_str(), O_WRONLY | O_CREAT, 0666);
                            dup2(fd, 1);
                            close(fd);
                        }
                        if(new_procs[i].input_file.size() != 0){
                            int fd = open(new_procs[i].input_file.c_str(), O_RDONLY);
                            dup2(fd, 0);
                            close(fd);
                        }
                        dup2(pipes[i-1][0], 0);
                        dup2(pipes[i][1], 1);

                    }
                    //close all pipes
                    for(int j=0;j<new_procs.size()-1;j++)
                    {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    string curr_command = new_procs[i].command;  //get the command
                    
                    if(curr_command == "cd"){ //if the first command is cd
                        if(new_procs[i].args.size() == 0){  //if there is no argument, go to home directory
                            chdir(getenv("HOME"));
                        }
                        else{   //if there is an argument, go to that directory
                            chdir(new_procs[i].args[0].c_str());
                        }
                    }
                    else if(curr_command == "pwd"){ //if the command is pwd
                        cout << cwd << endl;    //print the current working directory
                    }
                    else{
                        char *total_args[new_procs[i].args.size()+2];
                        total_args[0] = new char[new_procs[i].command.length()+1];
                        for(int i = 0; i < new_procs[i].command.length()+1; i++){
                            total_args[0][i] = '\0';
                        }
                        strcpy(total_args[0], new_procs[i].command.c_str());

                        for(int ii = 0; ii < (int)new_procs[i].args.size(); ii++)
                        {
                            total_args[ii+1] = new char[new_procs[i].args[ii].length()+1];
                            for(int j = 0; j < new_procs[i].args[ii].length()+1; j++)
                            {
                                total_args[ii+1][j] = '\0';
                            }
                            strcpy(total_args[ii+1], new_procs[i].args[ii].c_str());
                        }
                        total_args[new_procs[i].args.size()+1] = NULL;
                        execvp(total_args[0], total_args);
                        // exit(1);
                    }
                }
                else
                {
                    if(i==0)
                    {
                        close(pipes[i][1]);
                    }
                    else if(i==new_procs.size()-1)
                    {
                        close(pipes[i-1][0]);
                    }
                    else
                    {
                        close(pipes[i-1][0]);
                        close(pipes[i][1]);
                    }
                    wait(NULL);
                }
            }   
            for(int j=0;j<new_procs.size()-1;j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for(int j = 0; j < new_procs.size()-1; j++)
                wait(NULL);
        }
        else if(new_procs.size() == 1)
        {
            string first_command = new_procs[0].command;  //get the first command

            if(first_command == "exit")
            {
                hist.saveHistory();     //save the history
                cout<<  "Exiting shell" << endl;
                return 0;   //if the first command is exit, exit
            } 
            else if(first_command == "cd"){ //if the first command is cd
                if(new_procs[0].args.size() == 0){  //if there is no argument, go to home directory
                    chdir(getenv("HOME"));
                }
                else{   //if there is an argument, go to that directory
                    chdir(new_procs[0].args[0].c_str());
                }
            }
            else if(first_command == "pwd"){ //if the first command is pwd
                cout << cwd << endl;    //print the current working directory
            }
            else{
                int pid = fork();
                if(pid == 0){
                    if(new_procs[0].input_file != ""){
                        int fd = open(new_procs[0].input_file.c_str(), O_RDONLY);
                        dup2(fd, 0);
                        close(fd);
                    }
                    if(new_procs[0].output_file != ""){
                        int fd = open(new_procs[0].output_file.c_str(), O_WRONLY | O_CREAT, 0666);
                        dup2(fd, 1);
                        close(fd);
                    }
                    char *args[new_procs[0].args.size()+2];
                    args[0] = new char[new_procs[0].command.length()+1];
                    strcpy(args[0], new_procs[0].command.c_str());
                    for(int i = 0; i < new_procs[0].args.size(); i++){
                        args[i+1] = new char[new_procs[0].args[i].length()+1];
                        strcpy(args[i+1], new_procs[0].args[i].c_str());
                    }
                    args[new_procs[0].args.size()+1] = NULL;
                    execvp(args[0], args);
                }
                else{
                    wait(NULL);
                }
            }
        }
    }
}
