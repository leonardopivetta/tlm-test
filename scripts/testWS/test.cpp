using namespace std;

#include <iostream>
#include <string>
#include <unistd.h>

#include "ws_connection.h"

void onMessage(const int &id, const GenericMessage &msg)
{
    cout << id << " RECV: " << msg.topic << " | " << msg.payload << endl;
}

void onError(const int &id, const int &code, const string &message)
{
    cout << id << " Error: " << code << " - " << message << endl;
}

void onClose(const int &id, const int &code)
{
    cout << id << " Connection closed: " << code << endl;
}

void onOpen(const int &id)
{
    cout << id << " Connection opened" << endl;
}

int main()
{
    WSConnection cli1;
    cli1.init("127.0.0.1", "3000", 0);
    WSConnection cli2;
    cli2.init("127.0.0.1", "3000", 0);

    // cli1.add_on_message(onMessage);
    cli1.addOnOpen(onOpen);
    cli1.addOnClose(onClose);
    cli1.addOnError(onError);

    cli2.addOnOpen(onOpen);
    cli2.addOnClose(onClose);
    cli2.addOnError(onError);
    cli2.addOnMessage(onMessage);

    cli2.subscribe("telemetry_status");

    thread *cli1_thread = cli1.start();
    thread *cli2_thread = cli2.start();

    while (true)
    {
        sleep(1);
        cli1.setData(GenericMessage("topic1", "Test Message"));
    }

    cli1.closeConnection();
    cli2.closeConnection();

    return 0;
}