#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <iostream>

using namespace std;
int main(int argc, char const* argv[])
{
    int fd0;
    int fd1;

    // FIFO file path
    string gps0 = "/home/";
    gps0 += argv[1];


    // Creating the named file(FIFO)
    // mkfifo(<pathname>,<permission>)
    mkfifo(gps0.c_str(), 0666);

    fd0 = open(gps0.c_str(), O_RDONLY);
    string line = "";
    char ch;
    while (1)
    {
        line = "";
        ch = ' ';
        while(true){
            if(read(fd0, &ch, sizeof(char)) < 0){
              continue;
            }
            if(ch == '\n')
              break;
            line += ch;

        };
        cout << line << endl;
    }
    close(fd0);
    return 0;
}
