using namespace std;

#include "connection.h"
#include <iostream>

int Connection::instance_count = 0;

Connection::Connection()
{
    id = instance_count;
    instance_count++;
    open = false;
    done = false;
    new_data = false;
    max_queue_size = 40;
    subscription_count = 0;
}

Connection::~Connection()
{
}

int Connection::getId()
{
    return id;
}

// passing address, port and mode it will save it in the class
void Connection::init(const string &address, const string &port, const int &openMode)
{
    this->address = address;
    this->port = port;
    this->openMode = openMode;
}

// this loop check if there are new data to send
void Connection::sendLoop()
{
    while (!open)
        usleep(1000);
    while (!done)
    {
        unique_lock<mutex> lck(mtx);
        while (!new_data && !done)
            cv.wait(lck);

        // Adding this because code can be stuck waiting for cv
        // when actually the socket is being closed
        if (done)
            break;

        
        this->sendMessage(buff_send.front());
        buff_send.pop();

        if (buff_send.empty())
            new_data = false;
    }
}   

// this loop check if there are new data to receive
// the receive function is blocking, so it will wait for new data
void Connection::receiveLoop()
{
    while (!open)
        usleep(10000);
    while (!done)
    {
        if (subscription_count == 0)
        {
            usleep(1000);
            continue;
        }
        GenericMessage msg;

        this->receiveMessage(msg);

        if (onMessage)
        {
            onMessage(id, msg);
        }
    }
}

void Connection::clearData()
{
    lock_guard<mutex> guard(mtx);

    while (!buff_send.empty())
        buff_send.pop();
    new_data = false;
}

void Connection::setData(const GenericMessage &msg)
{
    lock_guard<mutex> guard(mtx);

    buff_send.push(msg);

    if (buff_send.size() > max_queue_size)
    {
        buff_send.pop();
        if (onError)
            onError(id, 0, "Queue is full, dropping message");
    }

    new_data = true;

    cv.notify_all();
}

void Connection::stop()
{
    {
        lock_guard<mutex> guard(mtx);
        done = true;
        open = false;
    }

    cv.notify_all();

    if (onClose)
    {
        onClose(id, 1000);
    }
}

void Connection::reset()
{
    {
        lock_guard<mutex> guard(mtx);
        done = false;
        open = false;
        new_data = false;
    }
    clearData();
}

////////////////////////////////////////////////////////////////////
//////////////////////////    CALLBACKS   //////////////////////////
////////////////////////////////////////////////////////////////////

void Connection::addOnOpen(function<void(const int &id)> clbk)
{
    onOpen = clbk;
}
void Connection::addOnClose(function<void(const int &id, const int &)> clbk)
{
    onClose = clbk;
}
void Connection::addOnError(function<void(const int &id, const int &, const string &msg)> clbk)
{
    onError = clbk;
}
void Connection::addOnMessage(function<void(const int &id, const GenericMessage &)> clbk)
{
    onMessage = clbk;
}