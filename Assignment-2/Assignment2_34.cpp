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



signed main(){
    char pwd[1024], user_input[1024];

    while(1){   //we start the shell
        getcwd(pwd, 1024);  //get the current working directory
        cout << pwd << "$ ";   //print the current working directory
        
        cin.getline(user_input, 1024);  //get the user input
        
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
        else{
            execute(new_procs, runinbg);  //execute the commands
        }
    }
}