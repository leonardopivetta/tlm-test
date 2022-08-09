#include <stdio.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <signal.h>

#include <cstdio>

#include "serial.h"

using namespace std;

std::mutex mtx;
std::condition_variable cv;

string shared_string;

int N = 6;
string BASENAME = "/home/gps";

void writer(string fname)
{
  int fd;
  mkfifo(fname.c_str(), 0666);
  fd = open(fname.c_str(), O_WRONLY);
  while (1)
  {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk); // wait for notify by main thread

    // Copy values from the shared string to the fifo file
    if(write(fd, shared_string.c_str(), shared_string.size()) == -1)
    {
      close(fd);
      mkfifo(fname.c_str(), 0666);
      fd = open(fname.c_str(), O_WRONLY);
    }
  }
  close(fd);
}

int main()
{
  // Ignore when the pipe closes (from a reader that disconnects)
  signal(SIGPIPE, SIG_IGN);

  {
    int counter = 0;
    while( remove((BASENAME + to_string(counter)).c_str() ) == 0 ){
      counter ++;
      cout << "Deleted: " << BASENAME << counter << endl;
    }
    cout << "Removed " << counter << " previous ports" << endl;
  }

  // serialport
  string port = "/dev/ttyACM0";
  serial s(port);

  uint32_t count = 0;
  while (s.open_port() < 0){
    cout << " -> "<< to_string(count) << " Failed opening " << port << "\r" << flush;
    count ++;
    usleep(1000000);
  }
  cout << "Opened port: " << port << endl;

  // Start one thread for each fifo file (pipe)
  string filename = "";
  for (int i = 0; i < N; i++)
  {
    filename = BASENAME + to_string(i);
    thread *t = new thread(writer, filename);
  }

  string line;
  while (1)
  {
    // Reading line from serialport
    line = s.read_line('\n') + "\n";
    {
      std::lock_guard<std::mutex> lk(mtx);
      shared_string = line;
    }
    // Notify threads of new data available
    cv.notify_all();
  }
  return 0;
}
