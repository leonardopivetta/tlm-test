#include <iostream>

#include "utils.h"
#include "serial.h"
#include "gps.h"

using namespace std;

bool read_gps_line(const serial &ser, string &line)
{
    static string buff;
    while (true)
    {
        static char c;
        if (ser.get_char(c) <= 0)
            return false;
        if (c == '$')
        {
            line = buff;
            line[line.size() - 1] = ' ';
            buff = "$";
            break;
        }
        buff += c;
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

int main()
{
    Gps gps("gps1");
    serial ser("/dev/ttyACM0");

    ser.open_port();

    string line;
    while (true)
    {
        read_gps_line(ser, line);

        if (line == "")
            continue;

        int code = parse_gps(&gps, get_timestamp_u(), line);
        if (code == ParseError::ParseOk)
        {
            if (gps.data.msg_type == "UBX")
                cout << gps.get_readable() << endl;
        }
        else
        {
            if (line[0] != '$')
                for (char c : line)
                    cout << hex << int(c) << " ";
            cout << endl;
        }
    }

    return 0;
}