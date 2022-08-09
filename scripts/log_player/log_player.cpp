#include "log_player.h"
#include <cstdlib>

string GPS_BASENAME = "/home/gps";
string HOME_PATH;
unordered_map<string, CAN_Socket_t> can_sockets;

void writer(string fname);
void send_can(vector<string>& can_lines);
void send_gps(vector<string>& gps_lines);

int main()
{
  srand((unsigned) time(NULL));

  if(getenv("HOME") == NULL)
  {
    printf("HOME environment variable not set\n");
    return -1;
  }
  HOME_PATH = getenv("HOME");
  GPS_BASENAME = HOME_PATH + "/Desktop/gps_";

  telemetry_config tel_conf;
  if(!LoadStruct(tel_conf, HOME_PATH + "/fenice_telemetry_config.json")){
    printf("Error loading telemetry config\n");
    return -1;
  }else{
    if(tel_conf.can_devices.size() == 0){
      tel_conf.can_devices.push_back(can_devices_o{.sock="vcan1", .name="primary"});
      tel_conf.can_devices.push_back(can_devices_o{.sock="vcan0", .name="secondary"});
    }
  }

  for(auto can_conf : tel_conf.can_devices){
    CAN_Socket_t& can_sock = can_sockets[can_conf.name];
    can_sock.sock = new Can(can_conf.sock.c_str(), &can_sock.addr);
    if(can_sock.sock->open_socket() < 0){
      printf("Error opening socket %s\n", can_conf.sock.c_str());
      return -1;
    }
  }
  usleep(1000000);

  signal(SIGPIPE, SIG_IGN);

  Browse b;
  b.SetMaxSelections(1);
  b.SetSelectionType(SelectionType::sel_folder);
  auto selected_paths = b.Start();

  if (selected_paths.size() == 0)
  {
    cout << "No file selected... exiting" << endl;
    return -1;
  }
  
  if(!std::filesystem::exists(selected_paths[0] + "/candump.log")){
    cout << "No candump.log found in selected folder... exiting" << endl;
    return -1;
  }
  if(!check_candump_format(selected_paths[0] + "/candump.log")){
    cout << "Selected file is not a candump file... exiting" << endl;
    return -1;
  }
  cout << "Selected file is a valid candump file" << endl;

  vector<string> can_lines, gps_lines;
  // getting CAN lines
  get_lines(selected_paths[0] + "/candump.log", &can_lines);

  // getting GPS lines
  if(!std::filesystem::exists(selected_paths[0] + "/gps_0.log")){
    cout << "No gps_0.log found in selected folder" << endl;
  }else{
    get_lines(selected_paths[0] + "/gps_0.log", &gps_lines);
  }

  thread can_sender(send_can, std::ref(can_lines));
  thread gps_sender(send_gps, std::ref(gps_lines));

  can_sender.join();
  gps_sender.join();
  return 0;
}

void send_can(vector<string>& can_lines){
  message msg;
  UtilsParseMessageResult result;
  while (true)
  {
    uint64_t prev_timestamp = 0;
    for (int i = 20; i < can_lines.size(); i++)
    {
      result = parse_can_line(can_lines[i], &msg);
      if (result != UtilsParseMessageResult::PARSE_MESSAGE_OK){
        printf("Error parsing message code<%d>: %s\n", (int)result, can_lines[i].c_str());
        continue;
      }

      if(can_sockets.find(msg.sender) == can_sockets.end()){
        printf("Unknown sender: %s\n", msg.sender.c_str());
        continue;
      }

      can_sockets[msg.sender].sock->send(msg.id, (char *)msg.data, msg.size);

      if (prev_timestamp > 0)
      {
        usleep((msg.timestamp - prev_timestamp));
      }

      prev_timestamp = msg.timestamp;
    }
  }
}

void send_gps(vector<string>& gps_lines){
  int counter = 0;
  while (remove((GPS_BASENAME + to_string(counter)).c_str()) == 0)
  {
    counter++;
    cout << "Deleted: " << GPS_BASENAME << counter << endl;
  }
  cout << "Removed " << counter << " previous ports" << endl;
  string filename = "";
  for (int i = 0; i < 1; i++)
  {
    filename = GPS_BASENAME + to_string(i);
    thread *t = new thread(writer, filename);
  }

  while(true){
    uint64_t prev_timestamp = 0;
    gps_message msg;
    UtilsParseMessageResult result;
    for (int i = 20; i < gps_lines.size(); i++){
      result = parse_gps_line(gps_lines[i], &msg);
      if (result != UtilsParseMessageResult::PARSE_MESSAGE_OK){
        printf("Error parsing message code<%d>: %s\n", (int)result, gps_lines[i].c_str());
        continue;
      }
      cout << msg.message << endl;
      {
        std::unique_lock<std::mutex> lk(mtx);
        cv.notify_all();
        shared_string = msg.message;
      }
      if (prev_timestamp > 0)
      {
        usleep((msg.timestamp - prev_timestamp));
      }
      prev_timestamp = msg.timestamp;
    }
  }
}

void writer(string fname)
{
  int fd;
  mkfifo(fname.c_str(), 0666);
  fd = open(fname.c_str(), O_WRONLY);
  while (1)
  {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk); // wait for notify by main thread

    cout << shared_string << endl;

    // Copy values from the shared string to the fifo file
    if (write(fd, shared_string.c_str(), shared_string.size()) == -1)
    {
      close(fd);
      mkfifo(fname.c_str(), 0666);
      fd = open(fname.c_str(), O_WRONLY);
    }
  }
  close(fd);
}