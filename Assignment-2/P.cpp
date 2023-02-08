#include <unistd.h>
using namespace std;

int main(){
    while(1){   //Infinite loop
        for(int i=0; i<5; i++){
            //Forking 5 processes
            pid_t pid = fork();
            if(pid == 0){   //Child process
                for(int j=0; j<10; j++){
                    //Forking 10 processes
                    pid_t pid = fork();
                    if(pid == 0){   //Grandchild process
                        while(1){
                            //Infinite loop
                        }
                    }
                }
                while(1){
                    //Infinite loop
                }
            }
        }
        sleep(120);     //Sleeping for 2 minutes
    }
}