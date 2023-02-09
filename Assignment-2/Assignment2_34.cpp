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
#include <fcntl.h>
#include <set>
#include <stack>
#include <sys/inotify.h>
#include <sys/select.h>
#include <map>
#include <ctype.h>
#include <string>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fstream>
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

sigjmp_buf env;

void handler_sigint(int signum){
    // print a new line
    printf("Process Terminated\n");
    // jump to the shell prompt
    siglongjmp(env, 1);
}

void handler_sigtstp(int signum){
    // print a new line
    printf("Moved to background\n");
    // jump to the shell prompt
    siglongjmp(env, 1);
}

void handler_nothing(int signum){
    // do nothing
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

int is_number(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) {
            return 0;
        }
        i++;
    }
    return 1;
}

void Execute_delep(commands comm){
    int shm_fd;
    int m_size = 102 * sizeof(int);
    int* shared_memory;
    const char* name = "Process_ID";
    shm_fd = shm_open(name, O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR);
    if (shm_fd < 0) {
        perror("Error in shm_open");
        return ;
    }
    ftruncate(shm_fd, m_size);
    shared_memory = (int *) mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == NULL ){
        perror("Error in map");
        return ;
    }
    else {
        //cout << "Shared memory created successfully.\n";
    }
    int status;

    char* target_file = new char[comm.args[0].length() + 1];
    strcpy(target_file, comm.args[0].c_str());

    if (fork() == 0) {
        vector<int> proc_open, proc_lock;
        DIR* proc_dir = opendir("/proc");
        if (proc_dir == NULL) {
            perror("Cannot open /proc directory");
            exit(0);
        }
        struct dirent *dir_entry;

        while (dir_entry = readdir(proc_dir)) {
            if (dir_entry -> d_type == 4 && is_number(dir_entry -> d_name)) {
                string path = "/proc/";
                path += dir_entry -> d_name;
                path += "/fd";
                char* cpath = new char[path.length() + 1];
                strcpy(cpath, path.c_str());
                DIR* fd_dir = opendir(cpath);
                if (fd_dir == NULL) {
                    perror("Cannot open /proc directory");
                    exit(0);
                }
                struct dirent *fd_entry;
                while (fd_entry = readdir(fd_dir)) {
                    if (fd_entry -> d_type == 10) {
                        char* link_val = new char[100];
                        char* link = new char[100];
                        sprintf(link, "%s/%s", path.c_str(), fd_entry -> d_name);
                        ssize_t val = 0;
                        val = readlink(link, link_val, 100);
                        link_val[val] = '\0';
                        if (strcmp(link_val, target_file) == 0) {
                            proc_open.push_back(stoi(dir_entry -> d_name));
                        }
                    }
                }
            } 
        }

        int file = 0;
        file = open(target_file, O_RDONLY);
        struct stat file_stat;
        fstat(file, &file_stat);
        freopen("/proc/locks", "r", stdin);
        while (feof(stdin) == 0) {
            string arr[8];
            for (int i = 0; i < 8; i++) {
                cin >> arr[i];
            }
            if (arr[1] == "FLOCK") {
                int proc_id = stoi(arr[4]);
                int count = 0;
                int i;
                for (i = 0; i < arr[5].length(); i++) {
                    if (arr[5][i] == ':') {
                        count++;
                        if (count == 2) {
                            i++;
                            break;
                        }
                    }
                }
                string id = arr[5].substr(i, arr[5].length() - i);
                char* cid = new char[id.length() + 1];
                strcpy(cid, id.c_str());
                if (stoi(cid) == file_stat.st_ino) {
                    proc_lock.push_back(proc_id);
                }
            }
            
        }
        shared_memory[0] = proc_open.size();
        shared_memory[1] = proc_lock.size();
        for (int i = 0; i < proc_open.size(); i++) {
            shared_memory[i + 2] = proc_open[i];
        }
        for (int i = 0; i < proc_lock.size(); i++) {
            shared_memory[i + 2 + proc_open.size()] = proc_lock[i];
        }
        exit(0);
    }
    wait(&status);
    int num_open = shared_memory[0];
    int num_lock = shared_memory[1];
    set<int> processes;
    cout << "Processes using file: ";
    for (int i = 0; i < num_open; i++) {
        cout << shared_memory[i + 2] << " ";
        processes.insert(shared_memory[i + 2]);
    }
    cout << endl;
    cout << "Processes locking file: ";
    for (int i = 0; i < num_lock; i++) {
        cout << shared_memory[i + 2 + num_open] << " ";
        processes.insert(shared_memory[i + 2 + num_open]);
    }
    cout << endl;
    if (num_lock + num_open == 0) {
        cout << "No processes are using or locking the file.\n";
        return;
    }
    cout << "Kill all processes and delete the file? (y/n): " ;
    char c;
    cin >> c;
    while (c != 'y' && c != 'n') {
        cout << "Enter y or n: ";
        cin >> c;
    }
    if (c == 'y') {
        for (auto it = processes.begin(); it != processes.end(); it++) {
            int r = kill(*it, SIGKILL);
            if (r == -1) {
                perror("Error in kill");
            }
        }
        remove(target_file);
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
    else if (command == "delep"){
        Execute_delep(comm);
    }
    else{
        int pid = 0;
        // if forkreq is 1, we fork the process i.e. run the command in a child process
        if (forkreq) {
            pid = fork();
        }
        // we run the command in the child process
        if (pid == 0){
            // setting the input and output files in case of redirection
            if (forkreq){
                struct sigaction act_nothing;
                act_nothing.sa_handler = &handler_nothing;
                sigemptyset(&act_nothing.sa_mask);
                act_nothing.sa_flags = SA_RESTART;

                struct sigaction act_default;
                act_default.sa_handler = SIG_DFL;
                sigemptyset(&act_default.sa_mask);
                act_default.sa_flags = SA_RESTART;

                if (background){
                    sigaction(SIGINT, &act_nothing, NULL);
                    sigaction(SIGTSTP, &act_nothing, NULL);
                }
                else{
                    sigaction(SIGINT, &act_default, NULL);
                    sigaction(SIGTSTP, &act_nothing, NULL);
                }
            }
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
            if (forkreq && !background){
                exit(0);
            }
        }
        else if (pid > 0){
            // if the command is not to be run in the background, we wait for the child to finish
            if (!background){
                struct sigaction act_tstp;
                act_tstp.sa_handler = &handler_sigtstp;
                sigemptyset(&act_tstp.sa_mask);
                act_tstp.sa_flags = SA_RESTART;
                sigaction(SIGTSTP, &act_tstp, NULL);
                // wait for the current child
                int status;
                // if ctrl z is pressed we dont wait for the child to finish
                if (waitpid(pid, &status, WUNTRACED) == -1){
                    perror("Error in waitpid");
                }
            }
            else{
                struct sigaction sig_shell_int;
                sig_shell_int.sa_handler = &handler_sigint;
                sig_shell_int.sa_flags = SA_RESTART;
                sigemptyset(&sig_shell_int.sa_mask);
                sigaction(SIGINT, &sig_shell_int, NULL);
            
                struct sigaction sig_shell_tstp;
                sig_shell_tstp.sa_handler = &handler_sigtstp;
                sig_shell_tstp.sa_flags = SA_RESTART;
                sigemptyset(&sig_shell_tstp.sa_mask);
                sigaction(SIGTSTP, &sig_shell_tstp, NULL);
                waitpid(pid, NULL, WNOHANG);
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
                    struct sigaction act_nothing;
                    act_nothing.sa_handler = &handler_nothing;
                    sigemptyset(&act_nothing.sa_mask);
                    act_nothing.sa_flags = SA_RESTART;

                    struct sigaction act_default;
                    act_default.sa_handler = SIG_DFL;
                    sigemptyset(&act_default.sa_mask);
                    act_default.sa_flags = SA_RESTART;

                    if (background){
                        sigaction(SIGINT, &act_nothing, NULL);
                        sigaction(SIGTSTP, &act_nothing, NULL);
                    }
                    else{
                        sigaction(SIGINT, &act_default, NULL);
                        sigaction(SIGTSTP, &act_nothing, NULL);
                    }
                    Execute_Command(new_procs[i], background, 0);
                }
                else if (pid > 0){
                    struct sigaction act_sigtstp;
                    act_sigtstp.sa_handler = &handler_sigtstp;
                    sigemptyset(&act_sigtstp.sa_mask);
                    act_sigtstp.sa_flags = SA_RESTART;
                    sigaction(SIGTSTP, &act_sigtstp, NULL);

                    if (i != 0) {
                        close(pipes[i - 1][0]);
                    }
                    if (i != n - 1) {
                        close(pipes[i][1]);
                    }
                    if (!background){
                        int status;
                        waitpid(pid, &status, WUNTRACED);
                    }
                    else{
                        waitpid(pid, NULL, WNOHANG);
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

signed main(){
    // the shell will ignore the sigint signal
    struct sigaction sig_shell_int;
    sig_shell_int.sa_handler = &handler_sigint;
    sig_shell_int.sa_flags = SA_RESTART;
    sigemptyset(&sig_shell_int.sa_mask);
    sigaction(SIGINT, &sig_shell_int, NULL);
   
    // the shell will have a default action for sigtstp
    struct sigaction sig_shell_tstp;
    sig_shell_tstp.sa_handler = &handler_sigtstp;
    sig_shell_tstp.sa_flags = SA_RESTART;
    sigemptyset(&sig_shell_tstp.sa_mask);
    sigaction(SIGTSTP, &sig_shell_tstp, NULL);

    rl_bind_keyseq("\\e[A", goUpInHistory);
    rl_bind_keyseq("\\e[B", goDownInHistory);

    Shell();
    return 0;
}
