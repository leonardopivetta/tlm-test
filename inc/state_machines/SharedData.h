#ifndef _SHARED_DATA_H_
#define _SHARED_DATA_H_


using namespace std;

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

#include "loads.h"
#include "utils.h"

#include "console.h"

#include "can.h"
#include "gps.h"
#include "utils.h"
#include "loads.h"
#include "serial.h"
#include "gps_logger.h"
#include "file_transfer.h"

#include "devices.pb.h"

#ifdef WITH_CAMERA
#include "camera.h"
#endif

#include "ws_connection.h"
#include "zmq_connection.h"

#define JSON_LOG_FUNC(msg) CONSOLE.LogError(msg)
#include "json-loader/telemetry_json.h"
#include "json-loader/messages_json.h"
using namespace rapidjson;

extern "C" {
#include "thirdparty/lapcounter/src/lapcounter.h"
}
#define CANLIB_TIMESTAMP
#define primary_IDS_IMPLEMENTATION
#define secondary_IDS_IMPLEMENTATION
#define primary_NETWORK_IMPLEMENTATION
#define secondary_NETWORK_IMPLEMENTATION
#define primary_MAPPING_IMPLEMENTATION
#define secondary_MAPPING_IMPLEMENTATION

#define __APP_JSON_IMPLEMENTATION__
#define __MESSAGES_JSON_IMPLEMENTATION__
#define __TELEMETRY_JSON_IMPLEMENTATION__

#include "thirdparty/can/lib/primary/c/ids.h"
#include "thirdparty/can/lib/secondary/c/ids.h"
#include "thirdparty/can/lib/primary/c/network.h"
#include "thirdparty/can/lib/secondary/c/network.h"
#include "thirdparty/can/proto/primary/cpp/mapping.h"
#include "thirdparty/can/proto/secondary/cpp/mapping.h"

#define INVERTER_PROTO_IMPLEMENTATION
#include "inverter.h"

// List of states
typedef enum {
    STATE_UNINITIALIZED = 0,
    STATE_INIT,
    STATE_IDLE,
    STATE_RUN,
    STATE_STOP,
    STATE_ERROR,
    NUM_STATES,
    NO_CHANGE
} state_t;

class SharedData {
	public:
		SharedData();
		~SharedData();

		/////////////
		// STRUCTS //
		/////////////
		struct CAN_Stat_t {
			uint64_t duration;
			uint64_t msg_count;
		};

		typedef struct CAN_Message_t {
			string receiver_name;
			uint64_t timestamp;
			can_frame frame;
		} CAN_Message;

		typedef struct CAN_Socket_t {
			Can *sock;
			string name;
			thread *thrd;
			sockaddr_can addr;
		} CAN_Socket;

		///////////
		// ENUMS //
		///////////

		enum TelemetryError {
			TEL_NONE,
			TEL_LOAD_CONFIG_TELEMETRY,
			TEL_LOAD_CONFIG_SESSION,
			TEL_LOG_FOLDER,
			TEL_CAN_SOCKET_OPEN
		};

		///////////////
		// VARIABLES //
		///////////////

		string StatesStr[NUM_STATES];
		state_t currentState;

		string HOME_PATH;
		string CAN_DEVICE;
		string FOLDER_PATH;
		string CURRENT_LOG_FOLDER;

		std::fstream *dump_file;

		uint64_t zero_timestamp;
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
		devices::Inverter inverter_proto;

		inverter_files_t inverter_files[2];
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
		atomic<state_t> wsRequestState;

		// Maps
		unordered_map<string, uint64_t> msgs_counters;
		unordered_map<string, uint64_t> msgs_per_second;
		unordered_map<string, uint64_t> timers;

		TelemetryError currentError;

		/////////////////
		// LAP COUNTER //
		/////////////////
		lc_point_t point; // to pass to evaluation
		lc_counter_t *lp; // initialization with default settings
		lc_counter_t *lp_inclination;
		double previousX;
		double previousY;

		unordered_map<int, int> transactions;
		FileTransfer::FileTransferManager ftm;

		///////////////
		// FUNCTIONS //
		///////////////

		void ActionThread();
		void KillThread(thread *thread, string name);

		string GetDate();
		string GetTime();
		TelemetryError GetError();

		void SaveStat();

		// Configs
		
		void LoadAllConfig();
		void SaveAllConfig();

		void CreateFolderName(string &out);
		void CreateHeader(string &out);
		void EmitError(TelemetryError error);
		void OpenLogFolder(const string &path);

		// GPS
		void SetupGps();
		void StartGps();
		void OnGpsLine(int id, string line);

		// CAN
		string CanMessage2Str(const can_frame &msg);
		void CanReceive(CAN_Socket *can);
		void LogCan(const CAN_Message &message);
		void OpenCanSocket();

        // WEBSOCKET
		void ConnectToWS();
        void OnOpen(const int &id);
        void OnClose(const int &id, const int &num);
        void OnError(const int &id, const int &num, const string &message);
        void SendWsData();
        void SendStatus();
    	void OnMessage(const int &id, const GenericMessage &msg);

		// Protobuffer
		void ProtoSerialize(const int &can_network, const uint64_t &timestamp, const can_frame &msg, const int &dev_idx);
		void ClearProto();
};

static const char* TelemetryErrorStr[] {
	"None",
	"Load telemetry config Error",
	"Load session config Error",
	"Log folder error",
	"Can failed opening socket"
};

static string topics[] {
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

#endif // _SHARED_DATA_H_