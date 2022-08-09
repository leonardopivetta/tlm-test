#include "console.h"

using namespace Debug;

Console::Console()
{
    header_color = GREEN;
    header_type  = BOLD;
    text_color   = DEFAULT;
    text_type    = NORMAL;

    start = time(0); // Open time
    // TODO: Implement time into console messages
}

/**
 * Class destructor. Close log file.
 */
Console::~Console()
{
    cout << endl;

    if (save_all_console_messages)
        console_messages.close();
}

/**
 * Close log file. Same functionality as class destructor.
 */
void Console::Close()
{
    cout << endl;

    if (save_all_console_messages)
        console_messages.close();
        save_all_console_messages = false;
}

/**
 *
 */
string Console::MakeTextColor(const int text_mode, const int terminal_mode, const int terminal_color, const string text)
{
    string result = NON_PRINT + to_string(text_mode) + COMBINED + to_string(terminal_mode) + to_string(terminal_color) + EOC + text + EOM;
    return result;
}

/**
 *
 */
string Console::MakeTextColor(const int text_mode, const int terminal_mode, const int terminal_color, const string text, const int background_color)
{
    string result = GetBgColorCode(background_color) + NON_PRINT + to_string(text_mode) + COMBINED + to_string(terminal_mode) + to_string(terminal_color) + EOC + text + EOM;
    return result;
}


string Console::GetBgColorCode(int color)
{
    string bg_color = "\x1B[4" + to_string(color);
    return bg_color;
}


void Console::SetBackgorundColor(int color)
{
    cout << "\x1B[4" << color << "m";
}


string Console::MakeNonPrintable(const string printable)
{
    string non_printable = NON_PRINT + printable + NON_PRINT;
    return non_printable;
}


void Console::ClearCurrentLine()
{
    cout << CLEAR_LINE;
}


double Console::get_dt()
{
    time_t now = time(0);
    double seconds = difftime(now, start);

    return seconds;
}


void Console::MoveCursorUp(const int lines)
{
    string cmd = std::string(NON_PRINT) + to_string(lines) + MOVE_UP + EOM;
    cout << cmd;
}


void Console::ClearWindow()
{
    string cmd = std::string(NON_PRINT) + CLEAR_DISPLAY + EOM;
    cout << cmd;
}


void Console::SetDefault(const int head_color, const int head_type, const int txt_color, const int txt_type)
{
    header_color = head_color;
    header_type  = head_type;
    text_color   = txt_color;
    text_type    = txt_type;
}


void Console::SaveAllMessages(string path)
{
    // If path is not available put the log file
    // near the executable
    if (path == "")
    {
        char cwd[100];
        string wd = getcwd(cwd, 100);
        path = wd;
    }

    save_all_console_messages = true;

    // Init console messages
    string _name = GetDatetimeAsString();
    string filename = path;

    if (path.back() == '/')
    {
        filename = path + _name + ".log";
    }

    try
    {
        console_messages.open(filename, std::ofstream::app);
    }
    catch (const std::ifstream::failure& e)
    {
        save_all_console_messages = false;
        ErrorMessage(e.what());
    }
    if(console_messages.is_open()){
        DebugMessage("Console messages are saved to " + filename);
    }
    else{
        ErrorMessage("Error opening file " + filename);
    }
}

// void Console::InitLogFile(const string path)
// {
//     string _name = GetDatetimeAsString();
//     string filename = path + "/" + _name + ".log";

//     if (path.back() == '/')
//     {
//         filename = path + _name + ".log";
//     }

//     try
//     {
//         console_log.open(filename);
//     }
//     catch (const ifstream::failure& e)
//     {
//         error_message(e.what());
//     }
//     //log(_name);
// }

void Console::Message(const string header,
                  const int head_color,
                  const string text,
                  const int txt_color)
{
    static string message;
    static string timestamp;
    timestamp = GetSystemTime();

    message = timestamp + " " + MakeTextColor(BOLD, TEXT, head_color, MakeHeader(header));
    message += SEPARATOR + MakeTextColor(NORMAL, TEXT, txt_color, text);

    cout << message << endl;

    if (save_all_console_messages)
        console_messages << timestamp << SEPARATOR << MakeHeader(header) << SEPARATOR << text << endl;
}


void Console::DebugMessage(const string header, const string text)
{
  Message(header, header_color, text, text_color);
}


void Console::DebugMessage(const string text)
{
    Message(INFO, header_color, text, text_color);
}


void Console::ErrorMessage(const string header, const string text)
{
    Message(header, RED, text, text_color);
}


void Console::ErrorMessage(const string text)
{
    Message(ERROR, RED, text, text_color);
}


void Console::WaitMessage(const string header, const string text)
{
    static string message;
    static string timestamp;
    timestamp = GetSystemTime();

    message  = GetSystemTime() + " " + MakeTextColor(BLINK, TEXT, header_color, MakeHeader(header));
    message += SEPARATOR + MakeTextColor(NORMAL, TEXT, text_color, text);

    cout << message << RESET_CURSOR << std::flush;

    if (save_all_console_messages)
        console_messages << timestamp << SEPARATOR << MakeHeader(header) << SEPARATOR << text << endl;
}


void Console::WaitMessage(const string text)
{
    WaitMessage(INFO, text);
}


void Console::MessageWithHeader(const string header, const string text, bool reprint)
{
    if (is_first_time)
    {
        string header_ = MakeTextColor(BOLD, TEXT, header_color, MakeHeader(INFO)) + SEPARATOR + MakeTextColor(BOLD, TEXT, text_color, header);
        cout << header_ << endl;
        is_first_time = false;
    }

    string message = MakeTextColor(NORMAL, TEXT, header_color, MakeHeader(INFO)) + SEPARATOR + MakeTextColor(NORMAL, TEXT, text_color, text);
    cout << message << endl;

    if (reprint)
        MoveCursorUp(1);

    if (save_all_console_messages)
    {
        string timestamp = GetSystemTime();
        console_messages << timestamp << SEPARATOR << MakeHeader(INFO) << SEPARATOR << text << endl;
    }
}


void Console::StatMessage(const string text)
{
    Message(STAT, BLUE, text, text_color);
}


void Console::WarnMessage(const string text)
{
    Message(WARN, YELLOW, text, text_color);
}


void Console::ClearTerminal(const int lines)
{
    string cmd = std::string(NON_PRINT) + to_string(lines) + MOVE_UP + EOM;
    cout << cmd;
}

/**
 * Ring system bell.
 *
 */
void Console::BellNotification()
{
    string _bell = BELL;
    cout << _bell << endl;
}


void Console::Test()
{
    const string text = "Now I'm here! Think I'll stay around around around...";
    DebugMessage(text);
}

string Console::GetDatetimeAsString()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d_%m_%Y__%H_%M_%S", timeinfo);
    std::string str(buffer);

    return str;
}

/**
 * @brief Get system time as string using
 * HH_MM_SS format.
 */
string Console::GetSystemTime()
{   
    const int len = 100;
    char buffer[len];
    struct timeval tv;
    tm * curr_tm;
    time_t curr_time;

    time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(buffer, len, "%d/%m/%Y %H:%M:%S", curr_tm);

    return string(buffer);
}

string Console::MakeHeader(string header){
  header.resize(4, ' ');
  return "[" + header + "]";
}