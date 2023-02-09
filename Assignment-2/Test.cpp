//  spawn a process which locks a file and try to write to a file using a while 1 loop.
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>



using namespace std;

int main() {
    int fd = open("temp.txt", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Error in open");
        return -1;
    }
    int status = flock(fd, LOCK_EX);
    if (status < 0) {
        perror("Error in flock");
        return -2;
    }
    else {
        cout << "File locked successfully.\n";
    }
    ofstream myfile;
    myfile.open("temp.txt");
    while (1) {
        myfile << "Writing to file" << endl;
        sleep(1);
    }
    myfile.close();
    return 0;
}