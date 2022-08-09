
#include "main.h"

int main()
{
    sockaddr_can can_addr0;
    sockaddr_can can_addr1;
    auto can0 = new Can("vcan0", &can_addr0);
    if (can0->open_socket() < 0)
        return -1;

    auto can1 = new Can("vcan1", &can_addr1);
    if (can1->open_socket() < 0)
        return -1;

    char data[] = {10, 10, 10, 10, 10, 10, 10, 10};
    while (true)
    {
        for (int i = 0; i < 2000; i++)
        {
            if (primary_is_message_id(i))
            {
                can0->send(i, data, 8);
            }
            if (secondary_is_message_id(i))
            {
                can1->send(i, data, 8);
            }
        }
        usleep(100000);
    }

    return 0;
}