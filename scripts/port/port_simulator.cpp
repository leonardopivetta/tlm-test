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

#include "utils.h"
#include "serial.h"

using namespace std;
std::mutex mtx;
std::condition_variable cv;

string shared_string;

const int N = 6;
string BASENAME = "/home/gps";

vector<thread*> threads;
vector<bool> new_data;

vector<string> split(string str, char separator){
  vector<string> ret;
  string bff = "";
  for(int i = 0; i < str.size(); i ++){
    if(str[i] == separator){
      ret.push_back(bff);
      bff = "";
    }
    else{
      bff += str[i];
    }
  }
  ret.push_back(bff);
  return ret;
}


void writer(int id, string fname)
{
  int fd;
  mkfifo(fname.c_str(), 0666);
  fd = open(fname.c_str(), O_WRONLY);
  while (1)
  {
    std::unique_lock<std::mutex> lk(mtx);
    while(new_data[id] == false)
      cv.wait(lk); // wait for notify by main thread

    // Copy values from the shared string to the fifo file
    write(fd, shared_string.c_str(), shared_string.size());
    new_data[id] = false;
  }
  close(fd);
}

int main()
{
  // Ignore when the pipe closes (from a reader that disconnects)
  signal(SIGPIPE, SIG_IGN);

  threads.reserve(N);
  new_data.reserve(N);

  {
    int counter = 0;
    while( remove((BASENAME + to_string(counter)).c_str() ) == 0 ){
      counter ++;
      cout << "Deleted: " << BASENAME << counter << endl;
    }
    cout << "Removed " << counter << " previous ports" << endl;
  }

  // Start one thread for each fifo file (pipe)
  string filename = "";
  for (int i = 0; i < N; i++)
  {
    new_data[i] = false;
    filename = BASENAME + to_string(i);
    threads.push_back(new thread(writer, i, filename));
  }

  string line;
  stringstream s;
  std::ifstream file("/home/filippo/COM3_211118_141650_5Hz.ubx");
  s << file.rdbuf();
  vector<string> lines = split(s.str(), '\n');

  while (1)
  {
    for(const string& line : lines)
    {
      {
        std::lock_guard<std::mutex> lk(mtx);
        shared_string = line + "\n";
      }
      // Notify threads of new data available
      for(int i = 0; i < N; i++)
        new_data[i] = true;
      cv.notify_all();
      usleep(1000);
    }
  }
  return 0;
}
