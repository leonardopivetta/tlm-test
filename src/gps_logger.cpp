#include "gps_logger.h"

bool read_gps_line(const serial &ser, string &line)
{
  static string buff;
  static char c;
  while (true)
  {
    if (ser.get_char(c) <= 0)
      return false;
    // start of nmea line
    if (c == '$')
    {
      line = buff;
      line[line.size() - 1] = ' ';
      buff = "$";
      break;
    }
    buff += c;
    // finish of ubx line
    if (buff[buff.size() - 2] == '\xb5' || buff[buff.size() - 1] == '\x62')
    {
      line = buff;
      line.erase(line.size() - 2, 2);
      buff = "\xb5\x62";
      break;
    }
  }
  return true;
}

GpsLogger::GpsLogger(int id_, string device)
{
  m_Device = device;

  m_FName = "gps";
  m_Mode = MODE_PORT;
  m_Kill = false;
  m_LogginEnabled = false;
  m_Running = false;
  m_StateChanged = false;

  id = id_;

  m_Thread = new thread(&GpsLogger::Run, this);
}

void GpsLogger::SetOutFName(const string &fname)
{
  m_FName = fname;
}

void GpsLogger::SetMode(int mode)
{
  m_Mode = mode;
}

void GpsLogger::SetOutputFolder(const string &folder)
{
  m_Folder = folder;
}

void GpsLogger::SetCallback(void (*clbk)(int, string))
{
  m_OnNewLine = clbk;
}
void GpsLogger::SetCallback(std::function<void(int, string)> clbk)
{
  m_OnNewLine = clbk;
}

void GpsLogger::StartLogging()
{
  unique_lock<mutex> lck(logger_mtx);

  CONSOLE.Log("GPS", id, "Start Logging");

  m_GPS = new std::ofstream(m_Folder + "/" + m_FName + ".log");

  if (!m_GPS->is_open())
    CONSOLE.LogError("GPS", id, "Error opening .log file", m_Folder + "/" + m_FName + ".log");

  stat.Duration_seconds = GetTimestamp();
  stat.Messages = 0;

  m_LogginEnabled = true;
  m_Running = true;
  m_StateChanged = true;
  cv.notify_all();
  CONSOLE.Log("GPS", id, "Done");
}

void GpsLogger::StopLogging()
{
  unique_lock<mutex> lck(logger_mtx);

  stat.Duration_seconds = GetTimestamp() - stat.Duration_seconds;
  m_StateChanged = false;
  m_GPS->close();
  delete m_GPS;
  SaveStat();

  m_LogginEnabled = false;
  m_Running = false;
  m_StateChanged = true;
  cv.notify_all();
}

void GpsLogger::Start()
{
  m_Running = true;
  m_StateChanged = true;
  cv.notify_all();
}

void GpsLogger::Stop()
{
  m_Running = false;
  m_StateChanged = true;
  cv.notify_all();
}

void GpsLogger::Kill()
{
  m_Kill = true;
  m_Running = false;
  m_StateChanged = true;
  cv.notify_all();
}

void GpsLogger::WaitForEnd()
{
  cv.notify_all();
  m_Thread->join();
}

int GpsLogger::OpenDevice()
{
  CONSOLE.Log("GPS", id, "Opening device mode:", m_Mode);
  if (m_Serial == nullptr)
    m_Serial = new serial(m_Device);

  int ret = 0;
  if (m_Mode == MODE_PORT)
    ret = m_Serial->open_port();
  else
    ret = m_Serial->open_file();

  if (ret < 0)
  {
    CONSOLE.LogError("GPS", id, "Failed opening", m_Device);
    return 0;
  }
  else
  {
    CONSOLE.Log("GPS", id, "Opened ", m_Device);
    return 1;
  }
}

void GpsLogger::Run()
{
  string line = "", buff = "";
  int line_fail_count = 0;
  int file_fail_count = 0;

  CONSOLE.Log("GPS", id, "Run thread");

  while (!m_Kill)
  {
    unique_lock<mutex> lck(mtx);
    while (!m_Running && !m_Kill)
      cv.wait(lck);

    m_StateChanged = false;
    if (m_Kill)
      break;

    if (m_Running)
    {
      if (!OpenDevice())
      {
        m_Running = false;
        continue;
      }
    }

    m_StateChanged = false;
    bool parse_result = false;
    CONSOLE.Log("GPS", id, "Running");
    while (m_Running)
    {
      try
      {
        parse_result = read_gps_line(*m_Serial, line);
      }
      catch (std::exception e)
      {
        CONSOLE.LogError("GPS", id, "Error", e.what());
        parse_result = false;
      }
      if(!parse_result){
        line_fail_count++;
        if (line_fail_count >= 20)
        {
          CONSOLE.LogError("GPS", id, "Failed 20 times readline");
          m_Running = false;
          line_fail_count = 0;
          break;
        }
        continue;
      }

      if (m_LogginEnabled)
      {
        unique_lock<mutex> lck(logger_mtx);
        try
        {
          if (line.find('\n') == string::npos)
            line += '\n';
          if (m_GPS->is_open())
            (*m_GPS) << '(' << get_timestamp_u() << ')' << line << flush;
        }
        catch (std::exception e)
        {
          CONSOLE.LogError("GPS", id, "Error writing to file:", e.what());
          file_fail_count++;
          if (file_fail_count >= 20)
          {
            CONSOLE.LogError("GPS", id, "Failed 20 times writing to file");
            m_Running = false;
            file_fail_count = 0;
            break;
          }
        }
        stat.Messages++;
      }

      // Callback on new line
      if (m_OnNewLine)
        m_OnNewLine(id, line);

      if (m_StateChanged)
        break;
    }

    if (!m_Running)
    {
      CONSOLE.Log("GPS", id, "closing port");
      m_Serial->close_port();
      CONSOLE.Log("GPS", id, "Done");
    }
  }
}

double GpsLogger::GetTimestamp()
{
  return duration_cast<duration<double, milli>>(system_clock::now().time_since_epoch()).count() / 1000;
}

void GpsLogger::SaveStat()
{
  if (stat.Messages == 0 || !path_exists(m_Folder))
    return;
  CONSOLE.Log("GPS", id, "Saving stat");
  stat.Average_Frequency_Hz = float(stat.Messages) / stat.Duration_seconds;
  SaveStruct(stat, m_Folder + "/" + m_FName + ".json");
  CONSOLE.Log("GPS", id, "Done");
}
