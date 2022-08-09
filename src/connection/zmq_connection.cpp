#include "zmq_connection.h"
#include "zhelpers.hpp"

using namespace std;

ZmqConnection::ZmqConnection() : Connection()
{
    connection_type = "ZMQ";
}

ZmqConnection::~ZmqConnection()
{
    delete socket->socket;
    delete socket->context;
}

// it will connect to the server using the selected mode
thread *ZmqConnection::start()
{
    thread *t = nullptr;
    if (openMode != SUB && openMode != PUB)
        return t;

    socket = new custom_zmq_socket();
    socket->context = new zmq::context_t(1);
    socket->socket = new zmq::socket_t(*socket->context, openMode);

    string server = "tcp://" + address + ":" + port;

    try
    {
        socket->socket->bind(server);
    }
    catch (zmq::error_t e)
    {
        if (e.num() == 98)
        {
            try
            {
                socket->socket->connect(server);
            }
            catch (zmq::error_t e)
            {
                if (onError)
                    onError(id, e.num(), e.what());
                if (onClose)
                    onClose(id, e.num());
            }
        }
        else
        {
            if (onError)
                onError(id, e.num(), e.what());
            if (onClose)
                onClose(id, e.num());
        }
    }
    open = true;
    done = false;

    if (onOpen)
        onOpen(id);

    if (openMode == PUB)
        return new thread(&ZmqConnection::sendLoop, this);
    else
        return new thread(&ZmqConnection::receiveLoop, this);
}

int ZmqConnection::subscribe(const string &topic)
{
    try
    {
        socket->socket->setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
    }
    catch (zmq::error_t e)
    {
        if (onError)
            onError(id, e.num(), e.what());
        return -1;
    }
    subscription_count++;
    return 0;
}

int ZmqConnection::unsubscribe(const string &topic)
{
    try
    {
        socket->socket->setsockopt(ZMQ_UNSUBSCRIBE, topic.c_str(), topic.size());
    }
    catch (zmq::error_t e)
    {
        if (onError)
            onError(id, e.num(), e.what());
        return -1;
    }
    if (subscription_count > 0)
        subscription_count--;
    return 0;
}

// before ending the program remember to close the connection
void ZmqConnection::closeConnection()
{
    socket->socket->close();
    socket->context->close();

    if (onClose)
    {
        onClose(id, 0); // 0 means no error
    }

    done = true;
    cv.notify_all();
}

// passing the topic and the message it will send it
void ZmqConnection::sendMessage(const GenericMessage &msg)
{
    if (socket->socket == nullptr)
        return;
    int rc;
    rc = s_sendmore(*(socket->socket), msg.topic);
    if (rc < 0)
    {
        if (onError)
        {
            onError(id, rc, "Error seding topic");
        }
    }

    rc = s_send(*(socket->socket), msg.payload);

    if (rc < 0)
    {
        if (onError)
        {
            onError(id, rc, "Error sending payload");
        }
    }
}

void ZmqConnection::receiveMessage(GenericMessage &msg)
{
    if (socket->socket == nullptr)
        return;
    try
    {
        msg.topic = s_recv(*socket->socket);
        msg.payload = s_recv(*socket->socket);
    }
    catch (zmq::error_t e)
    {
        if (onError)
        {
            onError(id, e.num(), e.what());
        }
    }
}