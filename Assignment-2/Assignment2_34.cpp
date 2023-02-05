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

using namespace std;

const string WHITESPACE = " \n\r\t\f\v";

class commands{
    public:
    string command;
    vector<string> args;
    string output_file, input_file;
};

vector<commands> parse(char *user_input){
    vector<commands> new_procs;
    commands new_proc;
    new_proc.command = "";
    new_proc.args.clear();
    new_proc.output_file = "";
    new_proc.input_file = "";
    int input_idx = 0;
    char input;
    do{
        input = user_input[input_idx++];
        if(input == '|')
        {
            if(new_proc.command.size() != 0){
                new_procs.push_back(new_proc);
                new_proc.command = "";
                new_proc.args.clear();
                new_proc.output_file = "";
                new_proc.input_file = "";
            }
        }
        else if(input == '\n' || input == '\t' || input == ' ' ){
            if(new_proc.command.size() != 0){
                string argument="";
                do{
                    input = user_input[input_idx++];
                    
                    if(input=='|'){
                        if(argument.length()!=0)
                        {
                            new_proc.args.push_back(argument);
                            argument="";
                        }
                        input_idx--;
                        break;
                    }
                    else if(input == '\n' || input == '\t' || input == ' ')
                    {
                        if(argument.length()!=0)
                        {
                            new_proc.args.push_back(argument);
                            argument="";
                        }
                    }
                    else if(input == '<'){
                        if(argument.length()!=0)
                        {
                            new_proc.args.push_back(argument);
                            argument="";
                        }
                        input = user_input[input_idx++];
                        string file_name = "";
                        while(input == ' ' || input == '\t' || input == '\n')
                        {
                            input = user_input[input_idx++];
                        }
                        while(input != ' ' && input != '\t' && input != '\n' && input != '|'){
                            file_name += input;
                            input = user_input[input_idx++];
                        }
                        new_proc.input_file = file_name;
                        input_idx--;
                    }
                    else if(input == '>'){
                        if(argument.length()!=0)
                        {
                            new_proc.args.push_back(argument);
                            argument="";
                        }
                        input = user_input[input_idx++];
                        string file_name = "";
                        while(input == ' ' || input == '\t' || input == '\n')
                        {
                            input = user_input[input_idx++];
                        }
                        while(input != ' ' && input != '\t' && input != '\n' && input != '|'){
                            file_name += input;
                            input = user_input[input_idx++];
                        }
                        new_proc.output_file = file_name;
                        input_idx--;
                    }
                    else
                    {
                        argument += input;
                    }
                }while(input != '\n' && input != '|');
                new_procs.push_back(new_proc);
                new_proc.command = "";
                new_proc.args.clear();
                new_proc.output_file = "";
                new_proc.input_file = "";

                }
            }
        else{
            new_proc.command += input;
        }
    }while(input != '\n');
    return new_procs;
}

signed main(){
    char pwd[1024], user_input[1024];

    while(1){   //we start the shell
        getcwd(pwd, 1024);  //get the current working directory
        cout << pwd << "$ ";   //print the current working directory
        
        for(int i = 0; i < 1024; i++)
            user_input[i] = '\n';
        cin.getline(user_input, 1024);  //get the user input

        vector<commands> new_procs = parse(user_input);  //parse the user input

        // // //print all commands parsed 
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

        //handling trailing whitespaces
        for(int i = 0; i < new_procs.size(); i++)
        {
            if(new_procs[i].args.size()==0)
                continue;
            int j = new_procs[i].args.size() -1;
            if(new_procs[i].args[j][0] == ' ' || new_procs[i].args[j][0] == '\t' || new_procs[i].args[j][0] == '\n' || new_procs[i].args[j][0] == '\0' || new_procs[i].args[j][0] == '\r')
            {
                new_procs[i].args.erase(new_procs[i].args.begin() + j);
                break;
            }
        }

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
                        cout << pwd << endl;    //print the current working directory
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

            if(first_command[0] == 'e' && first_command[1] == 'x' && first_command[2] == 'i' && first_command[3] == 't' )
            {
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
                cout << pwd << endl;    //print the current working directory
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
