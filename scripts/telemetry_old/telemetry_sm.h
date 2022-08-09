#ifndef __TELEMETRY_SM__
#define __TELEMETRY_SM__
#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <filesystem>

#include <mutex>
#include <thread>
#include <condition_variable>

#include "can.h"
#include "gps.h"
#include "utils.h"
#include "loads.h"
#include "serial.h"
#include "gps_logger.h"
#include "inverter.h"
#include "file_transfer.h"

#include "devices.pb.h"

#ifdef WITH_CAMERA
#include "camera.h"
#endif

#include "ws_connection.h"
#include "zmq_connection.h"

#include "console.h"
#include "StateMachine/StateMachine.h"

#define JSON_LOG_FUNC(msg) CONSOLE.LogError(msg)
#include "telemetry_json.h"
#include "messages_json.h"
using namespace rapidjson;

extern "C"
{
#include "thirdparty/lapcounter/src/lapcounter.h"
}

#include "implementations.h"

// // #define primary_IDS_IMPLEMENTATION
// // #define secondary_IDS_IMPLEMENTATION
// // #define primary_NETWORK_IMPLEMENTATION
// // #define secondary_NETWORK_IMPLEMENTATION
// #define primary_MAPPING_IMPLEMENTATION
// #define secondary_MAPPING_IMPLEMENTATION
// // #define __TELEMETRY_CONFIG_IMPLEMENTATION__
// // #define __MESSAGES_IMPLEMENTATION__
// #define CANLIB_TIMESTAMP
// #include "thirdparty/can/lib/primary/c/ids.h"
// #include "thirdparty/can/lib/secondary/c/ids.h"
// #include "thirdparty/can/lib/primary/c/network.h"
// #include "thirdparty/can/lib/secondary/c/network.h"

// #include "thirdparty/can/proto/primary/cpp/mapping.h"
// #include "thirdparty/can/proto/secondary/cpp/mapping.h"

using namespace std;
using namespace std::chrono;

struct CAN_Stat_t
{
	double duration;
	uint64_t msg_count;
};
typedef struct CAN_Message_t
{
	string receiver_name;
	uint64_t timestamp;
	can_frame frame;
} CAN_Message;
typedef struct CAN_Socket_t
{
	Can *sock;
	string name;
	thread *thrd;
	sockaddr_can addr;
} CAN_Socket;
enum TelemetryError
{
	TEL_NONE,
	TEL_LOAD_CONFIG_TELEMETRY,
	TEL_LOAD_CONFIG_SESSION,
	TEL_LOG_FOLDER,
	TEL_CAN_SOCKET_OPEN
};
static const char *TelemetryErrorStr[]{
	"None",
	"Load telemetry config Error",
	"Load session config Error",
	"Log folder error",
	"Can failed opening socket"};

static string topics[]{
	"ping",
	"telemetry_stop",
	"telemetry_kill",
	"telemetry_reset",
	"telemetry_start",
	"telemetry_get_config",
	"telemetry_action_raw",
	"telemetry_set_sesh_config",
	"telemetry_set_tel_config",
	"telemetry_action_zip_logs",
	"telemetry_action_zip_and_move",
	"telemetry_ask_transaction"
	};

class TelemetrySM : public StateMachine
{
public:
	TelemetrySM();
	~TelemetrySM();

	void Init();
	void Run();
	void Stop();
	void Reset();

	TelemetryError GetError();

private:
	enum States
	{
		ST_UNINITIALIZED,
		ST_INIT,
		ST_IDLE,
		ST_RUN,
		ST_STOP,
		ST_ERROR,
		ST_MAX_STATES
	};
	string StatesStr[ST_MAX_STATES];

	/////////////////
	// LAP COUNTER //
	/////////////////
	lc_point_t point; // to pass to evaluation
	lc_counter_t *lp; // initialization with default settings
	lc_counter_t *lp_inclination;
	double previousX;
	double previousY;

	/////////////////

	string HOME_PATH;
	string CAN_DEVICE;
	string FOLDER_PATH;
	string CURRENT_LOG_FOLDER;

	std::fstream *dump_file;

	double zero_timestamp;
	unordered_map<string, CAN_Socket> sockets;
	queue<CAN_Message> messages_queue;
	condition_variable can_cv;
	mutex can_mutex;

	Connection *pub_connection;
	Connection *sub_connection;

	vector<Gps *> gps_class;
	vector<GpsLogger *> gps_loggers;

	primary_devices *primary_devs;
	secondary_devices *secondary_devs;

	primary::Pack primary_pack;
	secondary::Pack secondary_pack;
	devices::GpsVec gps_proto;

	FILE *inverter_files[INVERTER_MSGS]; // 6?
	FILE *primary_files[primary_MESSAGE_COUNT];
	FILE *secondary_files[secondary_MESSAGE_COUNT];
#ifdef WITH_CAMERA
	Camera camera;
#endif

	string action_string;

	// Threads
	mutex mtx;
	atomic<bool> kill_threads;

	thread *data_thread;
	thread *status_thread;
	thread *ws_conn_thread;
	thread *pub_connection_thread;
	thread *sub_connection_thread;
	thread *actions_thread;

	// JSON
	telemetry_config tel_conf;
	session_config sesh_config;
	stat_json can_stat;

	// WebSocket
	ConnectionState_ ws_conn_state = ConnectionState_::NONE;
	atomic<States> wsRequestState;

	// Maps
	unordered_map<string, uint32_t> msgs_counters;
	unordered_map<string, uint32_t> msgs_per_second;
	unordered_map<string, uint64_t> timers;

	TelemetryError currentError;

	unordered_map<int, int> transactions;
	FileTransfer::FileTransferManager ftm;

private:
	string GetDate();
	string GetTime();
	string CanMessage2Str(const can_frame &msg);

	void OpenCanSocket();
	void OpenLogFolder(const string &path);
	void CreateHeader(string &header);
	void CreateFolderName(string &folder);
	void LogCan(const CAN_Message &message);

	// CAN Receive
	void CanReceive(CAN_Socket *);

	// GPS
	void SetupGps();
	void OnGpsLine(int id, string line);

	// WebSocket
	void ConnectToWS();
	void OnOpen(const int &id);
	void OnClose(const int &id, const int &num);
	void OnError(const int &id, const int &num, const string &message);
	void SendWsData();
	void SendStatus();
	void OnMessage(const int &id, const GenericMessage &msg);

	// Actions
	void ActionThread();

	// State Machine
	void SetError(TelemetryError);
	void EmitError(TelemetryError);

	// Configs
	void LoadAllConfig();
	void SaveAllConfig();

	// Stats
	void SaveStat();

	// Protobuffer
	void ProtoSerialize(const int &can_network, const uint64_t &timestamp, const can_frame &msg, const int &dev_idx);

private:
	// Define the state machine state functions with event data type
	STATE_DECLARE(TelemetrySM, UninitializedImpl, NoEventData)
	STATE_DECLARE(TelemetrySM, InitImpl, NoEventData)
	STATE_DECLARE(TelemetrySM, IdleImpl, NoEventData)
	STATE_DECLARE(TelemetrySM, RunImpl, NoEventData)
	STATE_DECLARE(TelemetrySM, StopImpl, NoEventData)
	STATE_DECLARE(TelemetrySM, ErrorImpl, NoEventData)

	ENTRY_DECLARE(TelemetrySM, Deinitialize, NoEventData)
	ENTRY_DECLARE(TelemetrySM, ToRun, NoEventData)

	// State map to define state object order. Each state map entry defines a
	// state object.
	BEGIN_STATE_MAP_EX
	STATE_MAP_ENTRY_ALL_EX(&UninitializedImpl, nullptr, &Deinitialize, nullptr)
	STATE_MAP_ENTRY_EX(&InitImpl)
	STATE_MAP_ENTRY_EX(&IdleImpl)
	STATE_MAP_ENTRY_ALL_EX(&RunImpl, nullptr, &ToRun, nullptr)
	STATE_MAP_ENTRY_EX(&StopImpl)
	STATE_MAP_ENTRY_EX(&ErrorImpl)
	END_STATE_MAP_EX
};

#define TEL_ERROR_CHECK                         \
	if (GetError() != TelemetryError::TEL_NONE) \
		return;

#endif // __TELEMETRY_SM__