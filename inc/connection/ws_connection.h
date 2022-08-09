#ifndef __WS_CONECTION__
#define __WS_CONECTION__

#include <set>
#include <mutex>
#include <string>
#include <unordered_set>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "connection.h"

using namespace std;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::client<websocketpp::config::asio_client> client;

enum ConnectionState_
{
	NONE,
	CONNECTING,
	CONNECTED,
	FAIL,
	CLOSED
};

#define WS_LOGIN GenericMessage("client", "{\"identifier\":\"client\"}")
#define WS_COLLECT_PING GenericMessage("server_collect_all_ping", "{\"type\": \"server_collect_all_ping\"}")
#define WS_GET_PING GenericMessage("server_get_all_ping", "{\"type\": \"server_get_all_ping\"}")
#define WS_TEL_START GenericMessage("telemetry_start", "{\"type\": \"telemetry_start\"}")
#define WS_TEL_STOP GenericMessage("telemetry_stop", "{\"type\": \"telemetry_stop\"}")
#define WS_TEL_KILL GenericMessage("telemetry_kill", "{\"type\": \"telemetry_kill\"}")
#define WS_TEL_RESET GenericMessage("telemetry_reset", "{\"type\": \"telemetry_reset\"}")
#define WS_TEL_GET_CONFIG GenericMessage("telemetry_get_config", "{\"type\":\"telemetry_get_config\"}")
#define WS_TEL_ACTION_ZIP GenericMessage("telemetry_action_zip_logs", "{\"type\":\"telemetry_action_zip_logs\"}")
#define WS_TEL_ACTION_ZIP_AND_MOVE GenericMessage("telemetry_action_zip_and_move", "{\"type\":\"telemetry_action_zip_and_move\"}")

class custom_ws_socket
{
public:
	client m_client;
	client::connection_ptr m_conn;
	websocketpp::connection_hdl m_hdl;
	websocketpp::lib::error_code ec;
};

class WSConnection : public Connection
{
public:
	WSConnection();
	~WSConnection();

	virtual void closeConnection();
	virtual thread *start();

	virtual int subscribe(const string &topic);
	virtual int unsubscribe(const string &topic);

private:
	set<string> topics;
	custom_ws_socket *socket;

	void m_onOpen(websocketpp::connection_hdl);
	void m_onClose(websocketpp::connection_hdl conn);
	void m_onFail(websocketpp::connection_hdl);
	void m_onMessage(client *cli, websocketpp::connection_hdl hdl, message_ptr msg);

	websocketpp::lib::thread *asio_thread = nullptr;
	websocketpp::lib::thread *telemetry_thread = nullptr;

	virtual void sendMessage(const GenericMessage &msg);
	virtual void receiveMessage(GenericMessage &msg);
};

#endif // __WS_CONECTION__