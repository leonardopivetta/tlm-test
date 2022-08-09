#pragma once
/*
* Cristiano Strobbe - Feb 2020
* Cool console debug messages
*
* Changelog:
* v1.0 - Simple header files for cool terminal messages.
* v1.1 - Added wait message and clear window functionality.
* v2.0 - No more single header file. Added message system time.
         Fix class style.
*/
#include <string.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <ctime>
#include <math.h>
#include <sys/time.h> // gettimeofday

#include <mutex>

using namespace std;



static std::string NON_PRINT =      "\e[";       // Escape character
static std::string EOM =            "\e[0m";     // End of message, clear all the settings used befores
static std::string COMBINED =       ";" ;        // \e[CMD1;CMD2;CMD3 ...
static std::string EOC =            "m";         // End of attribute
static std::string CLEAR_DISPLAY =  "\e[2J";     // Clear display and reset cursor
static std::string CLEAR_LINE =     "\e[1K";     // Clear line and reset cursor
static std::string RESET_CURSOR =   "\r";        // Reset cursor position
static std::string VERTICAL_TAB =   "\v";        // Move cursor up (??)
static std::string BELL =           "\a";        // System bell
static std::string MOVE_UP =        "A";         // \e[<N>A
static std::string MOVE_DW =        "B";         // \e[<N>B
static std::string MOVE_FW =        "C";         // \e[<N>C
static std::string MOVE_BW =        "D";         // \e[<N>D

// Messages state options
static std::string INFO =           "Info";
static std::string STAT =           "Stat";
static std::string WARN =           "Warn";
static std::string ERROR =          "Err ";
static std::string SYS =            "Sys ";

// What you want to color?
static int TEXT =       3;
static int BACKGROUND = 4;
static int LIGHTER =    9;

/* Text/Foreground
In order to colorise the part of the terminal that you choose
(text, background, ...) you first choose the code number refered
to the part of the temrinal that you want to print and then
put the color code.
Example: I want blue background so the code will be:
BACKGROUND + BLUE = 44
*/
static int BLACK =   0;
static int RED =     1;
static int GREEN =   2;
static int YELLOW =  3;
static int BLUE =    4;
static int MAGENTA = 5;
static int CYAN =    6;
static int GRAY =    7;
//int XXX =   8;
static int DEFAULT = 9;

// Text formatting
static int NORMAL =     0;
static int BOLD =       1;
static int DIM =        2;
//static int XXX =        3;
static int UNDERLINED = 4;
static int BLINK =      5;
static int INVERTED =   6;
static int HIDDEN =     7;

static std::string SEPARATOR = " ";

namespace Debug
{

  class Console
  {
    private:
      /**
       * @brief Construct a new Console object
       *
       */
      Console();
    public:

      static Console& Get()
      {
        static Console instance;
        return instance;
      }

      /**
      * @brief Destroy the Console:: Console object
      *
      */
      ~Console();

      /**
      * @brief Close log file. Same functionality as class destructor.
      *
      */
      void Close();

      // TODO: Put carriage return at the begin of the line
      void SetDefault(const int head_color, const int head_type, const int txt_color, const int txt_type);

      void SaveAllMessages(string path="");

      // // TODO(@cstrobbe):
      // void InitLogFile(const string path);

      // TODO(@cstrobbe):
      // void close_log(){}


      void DebugMessage(const string header,
                        const int color,
                        const int mode,
                        const string message,
                        const int msg_color,
                        const int msg_mode){}

      /**
       * @brief
       *
       * @param header
       * @param head_color
       * @param text
       * @param txt_color
       */
      void Message(const string header,
                        const int head_color,
                        const string text,
                        const int txt_color);

      /**
       * @brief
       *
       * @param header
       * @param text
       */
      void DebugMessage(const string header,
                        const string text);

      void DebugMessage(const string text);

      // void DebugMessage(const string text);


      void ErrorMessage(const string header, const string text);

      void ErrorMessage(const string text);



      void WaitMessage(const string header, const string text);

      void WaitMessage(const string text);



      void MessageWithHeader(const string header, const string text, bool reprint=true);

      void StatMessage(const string text);

      void WarnMessage(const string text);

      /**
       * @brief Ring system bell.
       *
       */
      void BellNotification();

      void Test();

            /**
       * @brief
       *
       * @param text_mode
       * @param terminal_mode
       * @param terminal_color
       * @param text
       * @return string
       */
      string MakeTextColor(const int text_mode,
                            const int terminal_mode,
                            const int terminal_color,
                            const string text);

      /**
       * @brief Get the Datetime As String object
       *
       * @return string
       */
      string GetDatetimeAsString();

      /**
       * @brief Get the System Time object
       *
       * @return string
       */
      string GetSystemTime();

      // void LogToFile(const string text, bool print_message=false);

      template<typename... Args>
      void Log(Args... args)
      {
        unique_lock<mutex> lck(mtx);
        DebugMessage(LogImpl(args...));
      }

      template<typename... Args>
      void LogStatus(Args... args)
      {
        unique_lock<mutex> lck(mtx);
        StatMessage(LogImpl(args...));
      }

      template<typename... Args>
      void LogWarn(Args... args)
      {
        unique_lock<mutex> lck(mtx);
        WarnMessage(LogImpl(args...));
      }

      template<typename... Args>
      void LogError(Args... args)
      {
        unique_lock<mutex> lck(mtx);
        ErrorMessage(LogImpl(args...));
      }

    private:

        int header_color = YELLOW;
        int header_type  = BOLD;
        int text_color   = DEFAULT;
        int text_type    = NORMAL;
        bool save_all_console_messages { false };
        time_t start;
        bool is_first_time { true };

        mutex mtx;

        std::ofstream console_messages; // Store all console messages

        /**
         * @brief
         *
         * @param text_mode
         * @param terminal_mode
         * @param terminal_color
         * @param text
         * @param background_color
         * @return string
         */
        string MakeTextColor(const int text_mode,
                             const int terminal_mode,
                             const int terminal_color,
                             const string text,
                             const int background_color);

        string MakeHeader(string header);

        string GetBgColorCode(int color = DEFAULT);

        void SetBackgorundColor(int color = DEFAULT);

        string MakeNonPrintable(const string printable);

        template<typename T>
        string LogImpl(T arg)
        {
          stringstream ss;
          ss << arg << " ";
          return ss.str();
        }

        template<typename T, typename... Args>
        string LogImpl(T arg1, Args... args)
        {
          stringstream ss;
          ss << LogImpl(arg1);
          ss << LogImpl(args...);
          return ss.str();
        }

        // TO BE FIXED
        void ClearCurrentLine();

        /**
         * @brief
         *
         * @return double
         */
        double get_dt();

        /**
         * @brief
         *
         * @param lines
         */
        void MoveCursorUp(const int lines);

        /**
         * @brief
         *
         */
        void ClearWindow();

        /**
         * @brief
         *
         * @param lines
         */
        void ClearTerminal(const int lines);

  };

}; // namespace debug

#define CONSOLE Debug::Console::Get()

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)