#include "serial.h"

serial::serial(string device)
{
  this->device = device;
}
serial::~serial()
{
  close(fd);
}

string serial::read_line(char separator)
{
  string line = "";

  char ch;
  while (true)
  {
    if (get_char(ch) <= 0)
    {
      break;
    }
    if (ch == separator)
      break;
    line += ch;
  };
  return line;
}

void serial::close_port()
{
  close(fd);
}

bool serial::file_exists()
{
  return std::filesystem::exists(device.c_str());
}

int serial::open_file()
{
  if (!file_exists())
    return -1;
  fd = open(device.c_str(), O_RDONLY);
  if (fd < 0)
    return -1;
  return 1;
}

int serial::open_port()
{
  if (!file_exists())
    return -1;

  fd = open(device.c_str(), O_RDWR);
  // Handle in case of error
  if (fd < 0)
    return -1;

  struct termios tty;

  // Read in existing settings and handle errors
  if (tcgetattr(fd, &tty) != 0)
  {
    cout << "Error tcgetattr" << endl;
    return -1;
  }

  // Setting baud rate
  cfsetispeed(&tty, B460800);
  cfsetospeed(&tty, B460800);

  tty.c_cflag &= ~PARENB;        // disable parity bit
  tty.c_cflag &= ~CSTOPB;        // clear stop field
  tty.c_cflag |= CS8;            // 8 data bits per byte
  tty.c_cflag &= ~CRTSCTS;       // disable TRS/CTS hardware flow control
  tty.c_cflag |= CREAD | CLOCAL; // turn on READ and ignore control lines, setting CLOCAL allows us to read data
  // local modes
  tty.c_lflag &= ~ICANON; // disable canonical mode, in canonical mode input data is received line by line, usually undesired when dealing with serial ports
  tty.c_lflag &= ~ECHO;   // if this bit (ECHO) is set, sent characters will be echoed back.
  tty.c_lflag &= ~ECHOE;
  tty.c_lflag &= ~ECHONL;
  tty.c_lflag &= ~ISIG; // when the ISIG bit is set, INTR,QUIT and SUSP characters are interpreted. we don't want this with a serial port
  // the c_iflag member of the termios struct contains low-level settings for input processing. the c_iflag member is an int
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // clearing IXON,IXOFF,IXANY disable software flow control
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // clearing all of this bits disable any special handling of received bytes, i want raw data
  // output modes (c_oflag). the c_oflag member of the termios struct contain low level settings for output processing, we want to disable any special handling of output chars/bytes
  tty.c_oflag &= ~OPOST; // prevent special interpretation of output bytes
  tty.c_oflag &= ~ONLCR; // prevent conversion of newline to carriage return/line feed
  // setting VTIME VMIN
  tty.c_cc[VTIME] = 10; // read() will block until either any amount of data is received or the timeout ocurs
  tty.c_cc[VMIN] = 0;

  // After changing settings we need to save the tty termios struct, also error checking
  if (tcsetattr(fd, TCSANOW, &tty) != 0)
  {
    cout << "Error tcsetattr" << endl;
    return -1;
  }

  return 0;
}
