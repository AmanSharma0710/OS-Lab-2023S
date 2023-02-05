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
    int output_file, input_file;
};

vector<commands> parse(char *user_input){
    vector<commands> new_procs;
    commands new_proc;
    new_proc.output_file = -1;
    new_proc.input_file = -1;
    int input_idx = 0;
    char input;
    do{
        input = user_input[input_idx++];
        if(input == ' ' || input == '\t' || input == '\n'){
            if(new_proc.command.size() != 0){
                new_procs.push_back(new_proc);
                new_proc.command = "";
                new_proc.args.clear();
                new_proc.output_file = -1;
                new_proc.input_file = -1;
            }
        }
        else if(input == '<'){
            input = user_input[input_idx++];
            string file_name = "";
            while(input != ' ' && input != '\t' && input != '\n'){
                file_name += input;
                input = user_input[input_idx++];
            }
            new_proc.input_file = open(file_name.c_str(), O_RDONLY);
            input_idx--;
        }
        else if(input == '>'){
            input = user_input[input_idx++];
            string file_name = "";
            while(input != ' ' && input != '\t' && input != '\n'){
                file_name += input;
                input = user_input[input_idx++];
            }
            new_proc.output_file = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            input_idx--;
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
        // else{
        //     execute(new_procs, runinbg);  //execute the commands
        // }
    }
}
