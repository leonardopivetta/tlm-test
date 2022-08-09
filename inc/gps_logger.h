#pragma once

#include <ctime>
#include <chrono>
#include <stdio.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <iostream>
#include <exception>
#include <functional>

#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "utils.h"
#include "serial.h"
#include "console.h"

#define JSON_LOG_FUNC(msg) CONSOLE.LogError(msg)
#include "json-loader/telemetry_json.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

using namespace std;
using namespace std::chrono;

#define MODE_PORT 0
#define MODE_FILE 1

struct GPS_Stat_t
{
  double delta_time;
  uint64_t msg_count;
};

bool read_gps_line(const serial &ser, string &line);

class GpsLogger
{
public:
  GpsLogger(int id, string device);

  void SetOutFName(const string &fname);
  void SetOutputFolder(const string &folder);
  void SetCallback(void (*f)(int, string));
  void SetCallback(std::function<void(int, string)> function);
  void SetMode(int mode = 0);

  void StartLogging();
  void StopLogging();
  void Start();
  void Stop();
  void Kill();
  void WaitForEnd();

  bool IsRunning();

  int GetId() { return this->id; };

private:
  void Run();
  int OpenDevice();

  void SaveStat();

  double GetTimestamp();

  std::ofstream *m_GPS;

  string m_FName;
  string m_Device;
  string m_Folder;
  string m_NewFolder;

  int m_Mode;
  serial *m_Serial = nullptr;
  thread *m_Thread = nullptr;

  bool m_LogginEnabled;
  bool m_Running;
  bool m_Kill;

  mutex mtx;
  mutex logger_mtx;
  condition_variable cv;
  bool m_StateChanged;

  // void (*m_OnNewLine)(int, string) = nullptr;
  std::function<void(int, string)> m_OnNewLine;

  stat_json stat;
  int id;
};
