#include "utils.h"

bool check_candump_format(string path)
{
  ifstream file(path);
  if(!file.is_open())
  {
    printf("Error opening file %s\n", path.c_str());
    return false;
  }
  file.seekg(0, std::ios::end);
  if(file.tellg() < 100)
  {
    printf("File too small to be a candump file\n");
    return false;
  }
  file.seekg(0, std::ios::beg);

  string line;

  // Reading the first lines that contains header
  for(int i = 0; i < 15; i++){
    if(!getline(file, line)){
      printf("Not enough lines\n");
      return false;
    }
  }

  message msg;
  UtilsParseMessageResult result;
  for(int i = 0; i < 20; i++){
    if(!getline(file, line)){
      printf("Not enough lines\n");
      return false;
    }
    result = parse_can_line(line, &msg);
    if(result != PARSE_MESSAGE_OK){
      printf("Error parsing message %d\n", (int)result);
      return false;
    }
  }
  return true;
}

UtilsParseMessageResult parse_can_line(string str, message *msg)
{
  static string timestamp_str;
  static size_t pos;

  if(str.size() <= 10)
    return PARSE_MESSAGE_SHORT;
  if(str[0] == '(')
    str = str.substr(1);

  // ------ timestamp ------ //
  pos = str.find(')');
  if(pos == string::npos)
    pos = str.find(' ');
  if(pos == string::npos)
    pos = str.find('\t');
  
  timestamp_str = str.substr(0, pos);
  str = str.substr(pos + 1);
  try{
    msg->timestamp = stoull(timestamp_str);
  }catch(std::invalid_argument const& ex){
    printf("Invalid timestamp: %s\n", timestamp_str.c_str());
    return PARSE_MESSAGE_TIMESTAMP;
  }
  // remove leading spaces
  remove_leading_spaces(str);

  // ------ sender ------ //
  pos = str.find(' ');
  if(pos == string::npos)
    pos = str.find('\t');
  if(pos == string::npos)
    return PARSE_MESSAGE_SENDER;
  
  msg->sender = str.substr(0, pos);
  str = str.substr(pos + 1);
  // remove leading spaces
  remove_leading_spaces(str);

  // ------ ID ------ //
  pos = str.find('#');
  if(pos == string::npos)
    return PARSE_MESSAGE_ID;
  
  msg->id = stoi(str.substr(0, pos), NULL, 16);
  str = str.substr(pos + 1);

  // ------ data ------ //
  msg->size = str.size() / 2;
  if(msg->size > 8 && msg->size % 2 != 0)
    return PARSE_MESSAGE_SIZE;
  
  for(int i = 0; i < msg->size-1; i++)
    msg->data[i] = stoi(str.substr(i*2, 2), NULL, 16);
  return PARSE_MESSAGE_OK;
}

UtilsParseMessageResult parse_gps_line(string str, gps_message *msg)
{
  static size_t pos;
  static string timestamp_str;

  if(str.size() < 10)
    return PARSE_MESSAGE_SHORT;
  
  if(str[0] == '(')
    str = str.substr(1);

  // ------ timestamp ------ //
  pos = str.find(')');
  if(pos == string::npos)
    pos = str.find(' ');
  if(pos == string::npos)
    pos = str.find('\t');
  
  timestamp_str = str.substr(0, pos);
  str = str.substr(pos + 1);
  try{
    msg->timestamp = stoull(timestamp_str);
  }catch(std::invalid_argument const& ex){
    printf("Invalid timestamp: %s\n", timestamp_str.c_str());
    return PARSE_MESSAGE_TIMESTAMP;
  }
  // remove leading spaces
  remove_leading_spaces(str);
  msg->message = str;
  return PARSE_MESSAGE_OK;
}

void get_lines(string filename, vector<string> *lines)
{
  FILE *f = fopen(filename.c_str(), "r");

  char *line = NULL;
  size_t size = 0;
  lines->clear();
  while (getline(&line, &size, f) != -1)
  {
    lines->push_back(line);
  }
  fclose(f);
}

vector<string> get_all_files(string path, string extension)
{
  std::vector<string> files;
  bool use_extension = extension != "*";

  if (!exists(path) || !is_directory(path))
    throw runtime_error("Directory non existent!");

  for (auto const &entry : recursive_directory_iterator(path))
  {
    if (is_regular_file(entry))
    {
      if (!use_extension)
        files.push_back(entry.path().string());
      if (use_extension && entry.path().extension() == extension)
        files.push_back(entry.path().string());
    }
  }

  return files;
}

vector<string> get_gps_from_files(vector<string> files)
{
  return get_files_with_word(files, "gps");
}
vector<string> get_candump_from_files(vector<string> files)
{
  return get_files_with_word(files, "dump.log");
}
vector<string> get_files_with_word(vector<string> files, string word)
{
  vector<string> new_vec;
  for (int i = 0; i < files.size(); i++)
  {
    size_t pos = files[i].find(word);
    if (pos != string::npos)
    {
      new_vec.push_back(files.at(i));
    }
  }
  return new_vec;
}

string get_parent_dir(string path)
{
  return std::filesystem::path(path).parent_path().string();
}

string remove_extension(string path)
{
  path = std::filesystem::path(path).filename().string();
  size_t lastindex = path.find_last_of(".");
  if (lastindex != string::npos)
    return path.substr(0, lastindex);
  return "";
}

void mkdir(string path)
{
  std::filesystem::path p = path;
  if (!exists(p))
    create_directory(p);
}

bool path_exists(string path)
{
  return exists(std::filesystem::path(path));
}

string get_colored(string text, int color, int style)
{
  return "\e[" + to_string(style) + ";3" + to_string(color) + "m" + text + "\e[0m";
}

vector<string> split(string str, char separator)
{
  vector<string> ret;
  string bff = "";
  for (int i = 0; i < str.size(); i++)
  {
    if (str[i] == separator)
    {
      ret.push_back(bff);
      bff = "";
    }
    else
    {
      bff += str[i];
    }
  }
  ret.push_back(bff);
  return ret;
}

int empty_fields(const vector<string> &vec)
{
  for (int i = 0; i < vec.size(); i++)
    if (vec[i] == "")
      return i;
  return -1;
}

int empty_fields(const vector<string> &vec, const vector<int> &indeces)
{
  for (int i : indeces)
  {
    if (i >= 0 && i < vec.size())
    {
      if (vec[i] == "")
        return i;
    }
  }
  return -1;
}

uint64_t get_timestamp_u()
{
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}
string get_hex(int num, int zeros)
{
  stringstream ss;
  ss << setw(zeros) << uppercase << setfill('0') << hex << num;
  return ss.str();
}
void get_hex(char *buff, const int &num, const int &zeros)
{
  sprintf(buff, ("%0" + to_string(zeros) + "X").c_str(), num);
}


bool execute_command(command_t* command){
  FILE* pipe = popen(command->command.c_str(), "r");
  if (!pipe)
    return false;
  command->cmd_thread = new thread([](FILE* pipe, command_t* cmd) {
    char buffer[32];
    try{
      while (fgets(buffer, 32, pipe) != NULL){
        cmd->lock();
        cmd->output += buffer;
        cmd->unlock();
      }
    } catch (...) {}
    pclose(pipe);
    cmd->lock();
    cmd->finished = true;
    cmd->unlock();
  }, pipe, command);
  if(command->cmd_thread == nullptr)
    return false;
  return true;
}