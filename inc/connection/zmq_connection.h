#ifndef __ZMQ_CONNECTION__
#define __ZMQ_CONNECTION__

#include <thread>
#include <string>

#include "connection.h"
#include "zhelpers.hpp"

class custom_zmq_socket
{
public:
    zmq::context_t *context;
    zmq::socket_t *socket;
};

class ZmqConnection : public Connection
{
public:
    ZmqConnection();
    ~ZmqConnection();

    virtual void closeConnection();
    virtual thread *start();

    virtual int subscribe(const string &topic);
    virtual int unsubscribe(const string &topic);

private:
    custom_zmq_socket *socket;

    virtual void sendMessage(const GenericMessage &msg);
    virtual void receiveMessage(GenericMessage &msg);
};

#endif
