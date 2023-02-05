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

class commands{
    public:
    string command;
    vector<string> args;
    string output_file, input_file;
};

vector<commands> parse(char *user_input){
    vector<commands> new_procs;
    commands new_proc;
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
        else if(input == '\n' || input == '\t' || input == ' '){
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
        
        // cin.getline(user_input, 1024);  //get the user input
        
        int input_idx = 0;
        char input;
        do{
            input = getchar();
            user_input[input_idx++] = input;
        }while(input != '\n');

        vector<commands> new_procs = parse(user_input);  //parse the user input

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

        if(new_procs.size() == 0) continue;  //if the user input is empty, continue
        
        string first_command = new_procs[0].command;  //get the first command
        if(first_command == "exit") break;   //if the first command is exit, break the loop
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
            //execute the command using execlp
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
                args[0] = new char[new_procs[0].command.size()+1];
                strcpy(args[0], new_procs[0].command.c_str());
                for(int i = 0; i < new_procs[0].args.size(); i++){
                    args[i+1] = new char[new_procs[0].args[i].size()+1];
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
