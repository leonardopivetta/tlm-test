#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>
#include <stdio.h>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <algorithm>
#include <regex>


#include <filesystem>

using namespace std;
using namespace std::chrono;
using namespace filesystem;

struct message
{
  int id;
  int size;
  uint8_t data[8];
  std::string sender;
  uint64_t timestamp;
};
struct gps_message
{
  uint64_t timestamp;
  string message;
};

enum UtilsParseMessageResult
{
  PARSE_MESSAGE_OK,
  PARSE_MESSAGE_ID,
  PARSE_MESSAGE_SIZE,
  PARSE_MESSAGE_DATA,
  PARSE_MESSAGE_SENDER,
  PARSE_MESSAGE_TIMESTAMP,
  PARSE_MESSAGE_SHORT
};

inline void remove_leading_spaces(string& str){
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::bind1st(std::not_equal_to<char>(), '\t')));
};

bool check_candump_format(string path);
void get_lines(string filename, vector<string> *lines);

UtilsParseMessageResult parse_can_line(string str, message *msg);
UtilsParseMessageResult parse_gps_line(string str, gps_message *msg);

/**
 * Gets current timestamp in seconds
 */
uint64_t get_timestamp_u();

vector<string> get_all_files(string path, string extension = "*");

vector<string> get_gps_from_files(vector<string> files);
vector<string> get_files_with_word(vector<string> files, string word);
vector<string> get_candump_from_files(vector<string> files);

string get_parent_dir(string path);

string remove_extension(string path);

void mkdir(string path);

bool path_exists(string path);

string get_colored(string text, int color, int style = 1);

vector<string> split(string str, char separator);

// returns -1 if all fields are filled
// returns index of first empty field
int empty_fields(const vector<string> &vec);

// indeces to be checked
// returns -1 if all fields are filled
// returns index of first empty field
int empty_fields(const vector<string> &vec, const vector<int> &indeces);

/**
 * Returns a string whith int expressed as Hexadecimal
 * Capital letters
 *
 * @param num number to be converted
 * @param zeros length of the final string (num = 4 => 0000A)
 * return string
 */
string get_hex(int num, int zeros);

void get_hex(char *buff, const int &num, const int &zeros);



/*
  * command command to be executed
  * output output of the command
  * cmd_thread thread in which the command is executed
  * mutex to protect the output
*/
struct command_t{
  string command;
  string output;
  bool finished;

  mutex mtx;
  thread* cmd_thread;

  command_t(){
    finished = false;
    cmd_thread = NULL;
  };
  void lock(){mtx.lock();};
  void unlock(){mtx.unlock();};
};
/* Nonblocking function to execute a shell command
  * @return true if command was launched successfully, false otherwise
  */
bool execute_command(command_t* command);

#endif // UTILS_H
