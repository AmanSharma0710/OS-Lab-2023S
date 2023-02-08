#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void Execute_Command(commands comm, int background, int forkreq){
    string command = comm.command;
    if (command == "exit"){
        printf("Exiting the shell.\n");
        exit(0);
    }
    else if (command == "pwd"){
        Execute_pwd(comm);
    }
    else if (command == "cd"){
        Execute_cd(comm);
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
                dup2(fd, 0);
                close(fd);
            }
            if (comm.output_file != ""){
                int fd = open(comm.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1){
                    perror("Error in opening file.");
                    exit(1);
                }
                dup2(fd, 1);
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

void Shell(){
    char *cwd = (char *)NULL;
    char *user_input = (char *)NULL;

    while(1){   //we start the shell
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
                        dup2(fd, 0);
                        close(fd);
                    }
                    if (new_procs[i].output_file != ""){
                        int fd = open(new_procs[i].output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd == -1){
                            perror("Error in opening file.");
                            exit(1);
                        }
                        dup2(fd, 1);
                        close(fd);
                    }
                    // setting the input and output pipes
                    if (i != 0) {
                        dup2(pipes[i - 1][0], 0);
                    }
                    if (i != n - 1) {
                        dup2(pipes[i][1], 1);
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
            // waiting for all the children to finish
            for (int i = 0; i < n; i++) {
                int status;
                wait(&status);
            }
        }
    }
    return ;
}

signed main(){
    Shell();
    return 0;
}
