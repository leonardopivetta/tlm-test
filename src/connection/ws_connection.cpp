#include "ws_connection.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

using namespace std;
using namespace rapidjson;

WSConnection::WSConnection() : Connection()
{
    socket = new custom_ws_socket();

    // set up access channels to only log interesting things
    socket->m_client.clear_access_channels(websocketpp::log::alevel::all);
    // socket->m_client.set_access_channels(websocketpp::log::alevel::connect);
    // socket->m_client.set_access_channels(websocketpp::log::alevel::disconnect);
    // socket->m_client.set_access_channels(websocketpp::log::alevel::app);

    // Initialize the Asio transport policy
    socket->m_client.init_asio();

    // Bind the handlers we are using
    socket->m_client.set_open_handler(bind(&WSConnection::m_onOpen, this, _1));
    socket->m_client.set_close_handler(bind(&WSConnection::m_onClose, this, _1));
    socket->m_client.set_fail_handler(bind(&WSConnection::m_onFail, this, _1));

    // bind with the on_message
    socket->m_client.set_message_handler(
        bind(bind(&WSConnection::m_onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
             &socket->m_client, ::_1, ::_2));

    connection_type = "WEBSOCKET";
}

// it will connect to the server using the selected mode
thread *WSConnection::start()
{
    done = false;

    stringstream server;
    if (address.size() <= 4)
    {
        if (onError)
            onError(id, 0, "Missing socket address");
        return nullptr;
    }

    if (address[0] == 'w' && address[1] == 's')
        server << address;
    else
        server << "ws://" << address;
    if (port != "")
        server << ":" << port;

    socket->m_conn = socket->m_client.get_connection(server.str(), socket->ec);

    if (socket->ec)
        return nullptr;

    socket->m_hdl = socket->m_conn->get_handle();
    socket->m_client.connect(socket->m_conn);

    socket->m_client.get_io_service().reset();
    asio_thread = new websocketpp::lib::thread(&client::run, &socket->m_client);
    telemetry_thread = new websocketpp::lib::thread(&WSConnection::sendLoop, this);

    return telemetry_thread;
}

void WSConnection::m_onOpen(websocketpp::connection_hdl)
{
    {
        lock_guard<mutex> guard(mtx);
        open = true;
    }

    if (onOpen)
        onOpen(id);
}

void WSConnection::m_onClose(websocketpp::connection_hdl conn)
{
    onClose(id, 0);
}

void WSConnection::m_onFail(websocketpp::connection_hdl)
{
    if (onError)
        onError(id, 0, "Failed");
}

WSConnection::~WSConnection()
{
    delete socket;
}

void WSConnection::closeConnection()
{
    if (open)
    {
        // the code was 1000
        socket->m_client.close(socket->m_hdl, 0, "Closed by user");
        socket->m_hdl.reset();
        socket->m_client.reset();
    }
    reset();
    done = true;
    cv.notify_all();
}

void WSConnection::m_onMessage(client *cli, websocketpp::connection_hdl hdl, message_ptr msg)
{
    static GenericMessage message;
    if (onMessage)
    {
        message.topic = msg->get_payload().c_str();
        if (topics.find(message.topic) == topics.end())
            return;
        message.payload = msg->get_payload().c_str() + message.topic.size() + 1;
        onMessage(id, message);
    }
}

void WSConnection::sendMessage(const GenericMessage &msg)
{
    string msg_str = string(msg.topic.c_str() + '\000', msg.topic.size() + 1) + msg.payload;
    socket->m_client.send(socket->m_hdl, msg_str, websocketpp::frame::opcode::binary, socket->ec);
    if (socket->ec && onError)
    {
        onError(id, socket->ec.value(), socket->ec.message());
    }
}

void WSConnection::receiveMessage(GenericMessage &msg_)
{
}

int WSConnection::subscribe(const string &topic)
{
    topics.insert(topic);
    subscription_count++;
    return 0;
}
int WSConnection::unsubscribe(const string &topic)
{
    topics.erase(topic);
    if (subscription_count > 0)
        subscription_count--;
    return 0;
}