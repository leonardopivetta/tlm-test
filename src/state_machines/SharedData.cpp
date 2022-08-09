#include "state_machines/SharedData.h"

SharedData::SharedData() {
    if (getenv("HOME") != NULL)
        HOME_PATH = getenv("HOME");
    else
        CONSOLE.LogError("HOME environment variable is not set");
    CONSOLE.SaveAllMessages(HOME_PATH + "/telemetry_log.debug");

    currentError = TelemetryError::TEL_NONE;

    StatesStr[STATE_UNINITIALIZED] = "STATE_UNINITIALIZED";
    StatesStr[STATE_INIT] = "STATE_INIT";
    StatesStr[STATE_IDLE] = "STATE_IDLE";
    StatesStr[STATE_RUN] = "STATE_RUN";
    StatesStr[STATE_STOP] = "STATE_STOP";
    StatesStr[STATE_ERROR] = "STATE_ERROR";

    dump_file = NULL;
    data_thread = nullptr;
    status_thread = nullptr;
    pub_connection = nullptr;
    sub_connection = nullptr;
    ws_conn_thread = nullptr;
    actions_thread = nullptr;
    pub_connection_thread = nullptr;
    sub_connection_thread = nullptr;

    lp = nullptr;
    lp_inclination = nullptr;

    kill_threads.store(false);
    wsRequestState = NUM_STATES;

    primary_devs = primary_devices_new();
    secondary_devs = secondary_devices_new();
    for (int i = 0; i < primary_MESSAGE_COUNT; i++) primary_files[i] = NULL;
    for (int i = 0; i < secondary_MESSAGE_COUNT; i++) secondary_files[i] = NULL;

    zero_timestamp = get_timestamp_u();
}

SharedData::~SharedData() {
}

void SharedData::ActionThread() {
  string cmd_copy = "";
  while (kill_threads.load() == false)
  {
    usleep(1000);
    {
      unique_lock<mutex> lck(mtx);
      if (action_string == "")
        continue;
      cmd_copy = action_string;
    }

    try
    {
      CONSOLE.Log("Starting command:", cmd_copy);
      string status = "Action: " + cmd_copy + " ----> started";

      command_t command;
      command.command = cmd_copy;
      if(!execute_command(&command)){
        CONSOLE.LogWarn("Failed executing command:", cmd_copy);
      }
      usleep(100);

      string out;
      command_status status_msg;
      status_msg.command = cmd_copy;
      while(!command.finished){
        command.lock();
        status_msg.data = command.output;
        command.unlock();
        CONSOLE.Log(status_msg.data);
        pub_connection->setData(GenericMessage("action_result", StructToString(status_msg)));
        usleep(1000);
      }
      status_msg.data = command.output;
      CONSOLE.Log(status_msg.data);
      pub_connection->setData(GenericMessage("action_result", StructToString(status_msg)));
    }
    catch (exception e)
    {
      CONSOLE.LogError("Actions exception:", e.what());
    }

    action_string = "";
  }
}

void SharedData::KillThread(thread* thread, string name) {
    if(thread != nullptr) {
		if(thread->joinable()) {
			thread->join();
		}
		delete thread;
		thread = nullptr;
	}
	CONSOLE.Log("Stopped " + name + " thread");
}

SharedData::TelemetryError SharedData::GetError() {
    return currentError;
}

string SharedData::GetDate() {
    static time_t date;
    static struct tm ltm;

    time(&date);
    localtime_r(&date, &ltm);

    std::ostringstream ss;
    ss << std::put_time(&ltm, "%d_%m_%Y");

    return ss.str();
}

string SharedData::GetTime() {
    static time_t date;
    static struct tm ltm;

    time(&date);
    localtime_r(&date, &ltm);

    std::ostringstream ss;
    ss << std::put_time(&ltm, "%H:%M:%S");

    return ss.str();
}

void SharedData::CreateFolderName(string &out) {
    // Create a folder with current configurations
    stringstream subfolder;
    subfolder << sesh_config.Race;
    subfolder << " [";
    subfolder << sesh_config.Configuration << "]";

    string s = subfolder.str();
    std::replace(s.begin(), s.end(), '\\', ' ');
    std::replace(s.begin(), s.end(), '/', ' ');
    std::replace(s.begin(), s.end(), '\n', ' ');

    out = s;
}

void SharedData::SaveStat() {
    can_stat.Average_Frequency_Hz = double(can_stat.Messages) / can_stat.Duration_seconds;
    SaveStruct(can_stat, CURRENT_LOG_FOLDER + "/CAN_Stat.json");

    sesh_config.Canlib_Version = primary_IDS_VERSION;
    SaveStruct(sesh_config, CURRENT_LOG_FOLDER + "/Session.json");
}

void SharedData::CreateHeader(string &out) {
    out = "\r\n\n";
    out += "*** EAGLE-TRT\r\n";
    out += "*** Telemetry Log File\r\n";
    out += "*** Date: " + sesh_config.Date + "\r\n";
    out += "*** Time: " + sesh_config.Time + "\r\n";
    out += "\r\n";
    out += "*** Curcuit       .... " + sesh_config.Circuit + "\r\n";
    out += "*** Pilot         .... " + sesh_config.Pilot + "\r\n";
    out += "*** Race          .... " + sesh_config.Race + "\r\n";
    out += "*** Configuration .... " + sesh_config.Configuration;
    out += "\n\n\r";
}

// to fix
void SharedData::EmitError(TelemetryError error) {
    currentError = error;
    CONSOLE.LogError(TelemetryErrorStr[error]);
}

// Load all configuration files
// If the file doesn't exist create it
void SharedData::LoadAllConfig() {
    string path = HOME_PATH + "/fenice_telemetry_config.json";
    if(path_exists(path)) {
        if(LoadStruct(tel_conf, path)) {
            CONSOLE.Log("Loaded telemetry config");
        } else {
            EmitError(TEL_LOAD_CONFIG_TELEMETRY);
        }
    } else {
        CONSOLE.Log("Created: " + path);
        tel_conf.can_devices = {can_devices_o{"can1", "primary"}};
        tel_conf.generate_csv = true;
        tel_conf.connection_enabled = true;
        tel_conf.connection_send_sensor_data = true;
        tel_conf.connection_send_rate = 500;
        tel_conf.connection_downsample = true;
        tel_conf.connection_downsample_mps = 50;
        tel_conf.connection.ip = "telemetry-server.herokuapp.com";
        tel_conf.connection.port = "";
        tel_conf.connection.mode = "WEBSOCKET";
        SaveStruct(tel_conf, path);
    }

    path = HOME_PATH + "/fenice_session_config.json";
    if(path_exists(path)) {
        if(LoadStruct(sesh_config, path)) {
            CONSOLE.Log("Loaded session config");
        } else {
            EmitError(TEL_LOAD_CONFIG_SESSION);
        }
    } else {
        CONSOLE.Log("Created: " + path);
        SaveStruct(sesh_config, path);
    }
}

void SharedData::SaveAllConfig() {
    string path = " ";
    path = HOME_PATH + "/fenice_telemetry_config.json";
    CONSOLE.Log("Saving new tel config");
    SaveStruct(tel_conf, path);
    CONSOLE.Log("Done");
    CONSOLE.Log("Saving new sesh config");
    path = HOME_PATH + "/fenice_session_config.json";
    SaveStruct(sesh_config, path);
    CONSOLE.Log("Done");
}

string SharedData::CanMessage2Str(const can_frame &msg) {
    // Format message as ID#<payload>
    // Hexadecimal representation
    static char buff[22];
    static char id[3];
    static char val[2];

    get_hex(id, msg.can_id, 3);
    sprintf(buff, "%s#", id);
    for(int i = 0; i < msg.can_dlc; i++) {
        get_hex(val, msg.data[i], 2);
        strcat(buff, val);
    }

    return string(buff);
}

void SharedData::LogCan(const CAN_Message &message) {
    if(dump_file == NULL || !dump_file->is_open()) {
        CONSOLE.LogError("candump file not opened");
        return;
    }
    static string rec;
    rec = message.receiver_name;
    rec.resize(10, ' ');
    (*dump_file) << "(" << message.timestamp << ")" << " " << rec << " " << CanMessage2Str(message.frame) << "\n";
}

void SharedData::OpenCanSocket() {
    sockets.clear();
    for(auto dev : tel_conf.can_devices) {
        CONSOLE.Log("Opening Socket ", dev.sock);
        CAN_Socket &new_can = sockets[dev.name];
        new_can.name = dev.name;
        new_can.sock = new Can(dev.sock.c_str(), &new_can.addr);
        new_can.thrd = nullptr;
        if(new_can.sock->open_socket() < 0) {
            CONSOLE.LogWarn("Failed opening socket: ", dev.name, " ", dev.sock);
            EmitError(TEL_CAN_SOCKET_OPEN);
            return;
        }
        CONSOLE.Log("Opened Socket: ", dev.name);
        CONSOLE.Log("Starting CAN thread");
        new_can.thrd = new thread(&SharedData::CanReceive, this, &new_can);
        CONSOLE.Log("Started CAN thread");
    }
}

void SharedData::CanReceive(CAN_Socket *can) {
    can_frame frame;
    while(!kill_threads) {
        CAN_Message msg;
        msg.receiver_name = can->name;
        int ret = can->sock->receive(&frame);
        if(ret == -1 || ret == 0) {
            CONSOLE.LogWarn("Can receive ", can->name);
            continue;
        }
        msg.timestamp = get_timestamp_u();
        msg.frame = frame;
        if(currentState == STATE_IDLE || currentState == STATE_RUN) {
            unique_lock<mutex> lck(can_mutex);
            messages_queue.push(msg);
            can_cv.notify_all();
        }
    }
}


void SharedData::OpenLogFolder(const string &path) {
    CONSOLE.Log("Output Folder ", path);
    if(!path_exists(path)) {
        CONSOLE.LogWarn("Creating log folder");
        if(!create_directory(path))
            EmitError(TEL_LOG_FOLDER);
    }
}

void SharedData::SetupGps() {
    CONSOLE.Log("Initializing gps instances");
    // Setup of all GPS devices
    for(size_t i = 0; i < tel_conf.gps_devices.size(); i++) {
        string dev = tel_conf.gps_devices[i].addr;
        string mode = tel_conf.gps_devices[i].mode;
        bool enabled = tel_conf.gps_devices[i].enabled;
        if(dev == "") continue;

        CONSOLE.Log("Initializing ", dev, mode, enabled);
        gps_class.push_back(new Gps(dev));
        GpsLogger *gps = new GpsLogger(i, dev);
        CONSOLE.Log("Instanced logger and dataclass");

        gps->SetOutFName("gps_" + to_string(gps->GetId()));
        msgs_counters["gps_" + to_string(gps->GetId())] = 0;
        msgs_per_second["gps_" + to_string(gps->GetId())] = 0;
        if(mode == "file") {
            gps->SetMode(MODE_FILE);
        } else {
            gps->SetMode(MODE_PORT);
        }
        CONSOLE.Log("Setted modes and filename");
        /////////////////
        // LAP COUNTER //
        /////////////////
        // lp = lc_init(NULL); // initialization with default settings
        // lp_inclination = lc_init(NULL);
        // previousX = -1;
        // previousY = -1;

        gps->SetCallback(bind(&SharedData::OnGpsLine, this, std::placeholders::_1, std::placeholders::_2));

        gps_loggers.push_back(gps);
        CONSOLE.Log("Setted callbacks");
    }
    CONSOLE.Log("Done");
}

void SharedData::StartGps() {
	CONSOLE.Log("Starting gps loggers");
	for(size_t i = 0; i < gps_loggers.size(); i++)
		if(tel_conf.gps_devices[i].enabled)
			gps_loggers[i]->Start();
	CONSOLE.Log("Done");
}

// Callback, fires every time a line from a GPS is received
void SharedData::OnGpsLine(int id, string line) {
    static ParseError ret;
    static Gps *gps;
    gps = nullptr;

    for(auto el : gps_class)
        if(el->get_id() == id) gps = el;
    if(gps == nullptr) return;

    // Parsing GPS data
    ret = ParseError::NoMatch;
    try {
        ret = parse_gps(gps, get_timestamp_u(), line);
    } catch(std::exception e) {
        CONSOLE.LogError("GPS parse error: ", line);
        return;
    }


    // If parsing was successfull
    if(ret == ParseError::ParseOk) {
        {
            unique_lock<mutex> lck(mtx);
            if(currentState == STATE_RUN && tel_conf.generate_csv) {
                (*gps->file) << gps->get_string(",") + "\n" << flush;
            }
        }

        devices::Gps *msg = gps_proto.add_gps();
        msg->set_timestamp(gps->data.timestamp);
        msg->set_msg_type(gps->data.msg_type);
        msg->set_time(gps->data.time);
        msg->set_latitude(gps->data.latitude);
        msg->set_longitude(gps->data.longitude);
        msg->set_altitude(gps->data.altitude);
        msg->set_fix(gps->data.fix);
        msg->set_satellites(gps->data.satellites);
        msg->set_fix_state(gps->data.fix_state);
        msg->set_age_of_correction(gps->data.age_of_correction);
        msg->set_course_over_ground_degrees(gps->data.course_over_ground_degrees);
        msg->set_course_over_ground_degrees_magnetic(gps->data.course_over_ground_degrees_magnetic);
        msg->set_speed_kmh(gps->data.speed_kmh);
        msg->set_mode(gps->data.mode);
        msg->set_position_diluition_precision(gps->data.position_diluition_precision);
        msg->set_horizontal_diluition_precision(gps->data.horizontal_diluition_precision);
        msg->set_vertical_diluition_precision(gps->data.vertical_diluition_precision);
        msg->set_heading_valid(gps->data.heading_valid);
        msg->set_heading_vehicle(gps->data.heading_vehicle);
        msg->set_heading_motion(gps->data.heading_motion);
        msg->set_heading_accuracy_estimate(gps->data.heading_accuracy_estimate);
        msg->set_speed_accuracy(gps->data.speed_accuracy);

        // in this message the coordinates are updated
        if(gps->data.msg_type == "GGA"){
            static uint8_t data[secondary_SIZE_GPS_COORDS];
            secondary_serialize_GPS_COORDS(data, gps->data.latitude, gps->data.longitude);
            sockets["secondary"].sock->send(secondary_ID_GPS_COORDS, (char*)data, secondary_SIZE_GPS_COORDS);
        }else if(gps->data.msg_type == "VTG"){
            static uint8_t data[secondary_SIZE_GPS_SPEED];
            secondary_serialize_GPS_SPEED(data, gps->data.speed_kmh);
            sockets["secondary"].sock->send(secondary_ID_GPS_SPEED, (char*)data, secondary_SIZE_GPS_SPEED);
        }

        // lapCounter
        // point.x = gps->data.latitude;
        // point.y = gps->data.longitude;

        // if(!(point.x == previousX && point.y == previousY)) {
        //   previousX = point.x;
        //   previousY = point.y;

        //   if(point.x == 0 && point.y == 0){
        //     // return
        //   }

        //   if(lc_eval_point(lp, &point, lp_inclination)){
        //     // è passato un giro
        //     // PHIL: arriva al volante?
        //   }
        // }

        msgs_counters["gps_" + to_string(id)]++;
    } else {
        // cout << ret << " " << line << endl;
    }
}

void SharedData::ConnectToWS() {
    pub_connection->addOnOpen(bind(&SharedData::OnOpen, this, std::placeholders::_1));
    sub_connection->addOnOpen(bind(&SharedData::OnOpen, this, std::placeholders::_1));
    pub_connection->addOnClose(bind(&SharedData::OnClose, this, std::placeholders::_1, std::placeholders::_2));
    sub_connection->addOnClose(bind(&SharedData::OnClose, this, std::placeholders::_1, std::placeholders::_2));
    pub_connection->addOnError(bind(&SharedData::OnError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    sub_connection->addOnError(bind(&SharedData::OnError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // sends sensors data only if connected
    data_thread = new thread(&SharedData::SendWsData, this);
    status_thread = new thread(&SharedData::SendStatus, this);
    while(kill_threads.load() == false) {
        usleep(1000000);
        if(!tel_conf.connection_enabled) continue;
        if(ws_conn_state == ConnectionState_::CONNECTED ||
            ws_conn_state == ConnectionState_::CONNECTING)
            continue;

        sub_connection->addOnMessage(bind(&SharedData::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        if(tel_conf.connection.mode == "ZMQ") {
            pub_connection->init(tel_conf.connection.ip, tel_conf.connection.port, ZmqConnection::PUB);
            sub_connection->init(tel_conf.connection.ip, tel_conf.connection.port, ZmqConnection::SUB);
        }
        if(tel_conf.connection.mode == "WEBSOCKET") {
            pub_connection->init(tel_conf.connection.ip, tel_conf.connection.port, 0);
            sub_connection->init(tel_conf.connection.ip, tel_conf.connection.port, 0);
        }
        pub_connection_thread = pub_connection->start();
        if(pub_connection_thread == nullptr)
            CONSOLE.ErrorMessage("PUB Failed connecting to server: " +
                                 tel_conf.connection.ip);
        if(sub_connection != pub_connection) {
            sub_connection_thread = sub_connection->start();
            if(sub_connection_thread == nullptr)
                CONSOLE.ErrorMessage("SUB Failed connecting to server: " +
                                     tel_conf.connection.ip);
        }
        // Login as telemetry
        pub_connection->clearData();
        if(pub_connection->GetConnectionType() == "WEBSOCKET")
            pub_connection->setData(GenericMessage(" ", "{\"identifier\":\"telemetry\"}"));

        for(const auto &topic : topics) {
            sub_connection->subscribe(topic);
        }
        ws_conn_state = ConnectionState_::CONNECTING;
    }
    CONSOLE.LogError("KILLED");
}

void SharedData::OnOpen(const int &id) {
    CONSOLE.Log("Connection opened");
    ws_conn_state = ConnectionState_::CONNECTED;
}

void SharedData::OnClose(const int &id, const int &num) {
    CONSOLE.LogError("Connection Closed <", num, ">");
    ws_conn_state = ConnectionState_::CLOSED;
}

void SharedData::OnError(const int &id, const int &num, const string &err) {
    CONSOLE.LogError("Connection Error <", num, ">", "message: ", err);
    ws_conn_state = ConnectionState_::FAIL;
}

void SharedData::SendWsData() {
    string primary_serialized;
    string secondary_serialized;
    string gps_serialized;
    bool primary_ok;
    bool secondary_ok;
    bool gps_ok;
    bool inv_ok;

    car_data car_data_msg;

    while(kill_threads.load() == false) {
        while(ws_conn_state != ConnectionState_::CONNECTED) {
            usleep(500000);
        }

        while(ws_conn_state == ConnectionState_::CONNECTED) {
            if(kill_threads.load() == true)
                break;
            usleep(1000 * tel_conf.connection_send_rate);

            if(!tel_conf.connection_send_sensor_data)
                continue;

            {
                unique_lock<mutex> lck(mtx);
                primary_ok = primary_pack.SerializeToString(&car_data_msg.primary);
                secondary_ok = secondary_pack.SerializeToString(&car_data_msg.secondary);
                gps_ok = gps_proto.SerializeToString(&car_data_msg.gps);
                inv_ok = inverter_proto.SerializeToString(&car_data_msg.inverters);

                if(!primary_ok && !secondary_ok && !gps_ok && !inv_ok)
                    continue;
                
                car_data_msg.timestamp = get_timestamp_u();
                pub_connection->setData(GenericMessage("update_data", StructToString(car_data_msg)));
                
                primary_pack.Clear();
                secondary_pack.Clear();
                gps_proto.Clear();
                inverter_proto.Clear();
                car_data_msg.primary.clear();
                car_data_msg.secondary.clear();
                car_data_msg.gps.clear();
                car_data_msg.inverters.clear();
            }
        }
    }
}

void SharedData::SendStatus() {
    uint8_t msg_data[8];
    int is_in_run;
    while(kill_threads.load() == false) {
        if(currentState == STATE_RUN) {
            is_in_run = 1;
        } else {
            is_in_run = 0;
        }
        primary_serialize_TLM_STATUS(msg_data, is_in_run ? primary_Toggle_ON : primary_Toggle_OFF);

        if(sockets.find("primary") != sockets.end()) {
            sockets["primary"].sock->send(primary_ID_TLM_STATUS, (char *)msg_data, primary_SIZE_TLM_STATUS);
        }
        primary_serialize_TLM_VERSION(msg_data, 12, primary_IDS_VERSION);

        if(sockets.find("primary") != sockets.end()) {
            sockets["primary"].sock->send(primary_ID_TLM_VERSION, (char *)msg_data, primary_SIZE_TLM_VERSION);
        }

        string str;
        for(auto el : msgs_counters) {
            msgs_per_second[el.first] = msgs_counters[el.first];
            msgs_counters[el.first] = 0;
            str += "{" + el.first + ", " + to_string(msgs_per_second[el.first]) + "} ";
        }
        string state = CONSOLE.MakeTextColor(BOLD, TEXT, BLUE, StatesStr[currentState]);
        CONSOLE.Log(state, "MSGS per second: " + str);

        if(tel_conf.connection_enabled && ws_conn_state == ConnectionState_::CONNECTED) {
            telemetry_status tel_status_struct;
            tel_status_struct.type = "telemetry_status";
            tel_status_struct.timestamp = get_timestamp_u();
            tel_status_struct.zero_timestamp = zero_timestamp;
            tel_status_struct.data = is_in_run;
            tel_status_struct.msgs_per_second.clear();
            for(auto [dev, count] : msgs_per_second) {
                tel_status_struct.msgs_per_second.push_back(msgs_per_second_o{dev, count});
            }
#ifdef WITH_CAMERA
            tel_status_struct.camera_status = camera.StatesStr[camera.GetCurrentState()];
            tel_status_struct.camera_error = CamErrorStr[camera.GetError()];
#else
            tel_status_struct.camera_status = "NONE";
            tel_status_struct.camera_error = "CAMERA NOT SUPPORTED";
#endif
            tel_status_struct.cpu_total_load = cpu_total_load_value();
            tel_status_struct.cpu_process_load = cpu_process_load_value();
            tel_status_struct.mem_process_load = mem_process_load_value();
            pub_connection->setData(GenericMessage("telemetry_status", StructToString(tel_status_struct)));
        }
        usleep(1000000);
    }
}

// some improvement here (?)
void SharedData::OnMessage(const int &id, const GenericMessage &msg) {
    Document req;
    StringBuffer sb;
    Writer<StringBuffer> w(sb);
    rapidjson::Document::AllocatorType &alloc = req.GetAllocator();

    Document helper_doc;
    Document ret;
    StringBuffer sb2;
    Writer<StringBuffer> w2(sb2);
    rapidjson::Document::AllocatorType &alloc2 = ret.GetAllocator();

    static string data;
    static string topic;
    static ParseResult ok;
    topic = msg.topic;
    data = msg.payload;
    ok = req.Parse(data.c_str(), data.size());

    if(!ok) return;

    static file_ask_transaction ask;
    static file_response_transaction resp;

    if(topic == "telemetry_set_sesh_config") {
        CONSOLE.Log("Received new session config");
        session_config buffer;
        if(StringToStruct(data, buffer)){
            sesh_config = buffer;
            SaveAllConfig();
        }else{
            CONSOLE.LogWarn("Session config parse failed");
        }
    } else if(topic == "telemetry_set_tel_config") {
        CONSOLE.Log("Received new telemetry config");
        telemetry_config buffer;
        if(StringToStruct(data, buffer)){
            tel_conf = buffer;
            SaveAllConfig();
        }else{
            CONSOLE.LogWarn("Telemetry config parse failed");
        }
    } else if(topic == "ping") {
        CONSOLE.DebugMessage("Requested ping");
        ret.SetObject();
        ret.AddMember("type", Value().SetString("server_answer_ping"), alloc2);
        ret.AddMember("time", (get_timestamp_u() - req["time"].GetDouble()), alloc2);
        ret.Accept(w2);
        pub_connection->setData(GenericMessage("server_answer_ping", sb2.GetString()));
    } else if(topic == "telemetry_get_config") {
        CONSOLE.DebugMessage("Requested configs");

        get_telemetry_config get_msg;
        get_msg.type = "telemetry_config";
        get_msg.session_config = StructToString(sesh_config);
        get_msg.telemetry_config = StructToString(tel_conf);
        pub_connection->setData(GenericMessage("telemetry_config", StructToString(get_msg)));
    } else if(topic == "telemetry_kill") {
        CONSOLE.Log("Requested Kill (from ws)");
        exit(2);
    } else if(topic == "telemetry_reset") {
        CONSOLE.Log("Requested Reset (from ws)");
        wsRequestState = STATE_UNINITIALIZED;
    } else if(topic == "telemetry_start") {
        CONSOLE.Log("Requested Start (from ws)");
        wsRequestState = STATE_RUN;
    } else if(topic == "telemetry_stop") {
        CONSOLE.Log("Requested Stop (from ws)");
        wsRequestState = STATE_STOP;
    } else if(topic == "telemetry_action_raw") {
        if(req.HasMember("data")) {
            CONSOLE.Log("Requested action:", req["data"].GetString());
            unique_lock<mutex> lck(mtx);
            action_string = req["data"].GetString();
        }
    }else if(topic == "telemetry_ask_transaction"){
        if(StringToStruct(data, ask))
        {
            CONSOLE.Log("Requested file transaction:", ask.identifier, " ", ask.transaction_hash);
            resp.identifier = ask.identifier;
            resp.transaction_hash = ask.transaction_hash;
            resp.transaction_topic = ask.transaction_hash;
            sub_connection->subscribe(resp.transaction_topic);
            sub_connection->subscribe(resp.transaction_topic+"_end");
            sub_connection->subscribe(resp.transaction_topic+"_begin");
            pub_connection->setData(GenericMessage("file_response_transaction", StructToString(resp)));
        }
    }else if(topic == resp.transaction_topic){
        static file_chunk chunk;
        if(StringToStruct(msg.payload, chunk)){
            auto it = transactions.find(chunk.transaction_hash);
            if(it != transactions.end()){
                if(!ftm.receive(it->second, msg)){
                    CONSOLE.LogWarn("FileTranferManager did not find transaction");
                }else{
                    CONSOLE.Log("<",chunk.transaction_hash,"> received chunk",chunk.chunk_n,"of",chunk.chunk_total,":",(float)chunk.chunk_n / chunk.chunk_total);
                }
            }else{
                CONSOLE.LogWarn("Transaction id unrecognized", chunk.transaction_hash);
            }
        }
    }else if(topic == resp.transaction_topic+"_end"){
        static file_end_transaction tr_end;
        if(StringToStruct(msg.payload, tr_end)){
        auto it = transactions.find(tr_end.transaction_hash);
        if(it != transactions.end()){
            if(ftm.end_receive(it->second)){
                transactions.erase(it);
                CONSOLE.Log("<",tr_end.transaction_hash,"> -> End");
                sub_connection->unsubscribe(resp.transaction_topic);
            }else{
                CONSOLE.LogWarn("<",tr_end.transaction_hash,"> -> FAILED end receive");
            }
        }else{
            CONSOLE.LogWarn("End transaction failed");
        }
    }
    }else if(topic == resp.transaction_topic+"_begin"){
        static file_begin_transaction begin;
        if(StringToStruct(data, begin)){
            FileTransfer::FileTransferManager::transaction_t transaction(
                begin.filename,
                begin.dest_path,
                begin.total_chunks,
                [](const int &id, TRANSACTION_EVENT event){
                CONSOLE.LogWarn("Transaction event:", id, TRANSACTION_EVENT_STRING[event]);
                }
            );
            int id = ftm.init_receive(transaction);
            if(id != -1){
                transactions[begin.transaction_hash] = id;
            }else{
                CONSOLE.LogWarn("Transaction initialization failed");
            }
        }
    }
}

void SharedData::ProtoSerialize(const int &can_network, const uint64_t &timestamp, const can_frame &msg, const int &dev_idx) {
    // Serialize with protobuf if websocket is enabled
    if(tel_conf.connection_enabled == false || tel_conf.connection_send_sensor_data == false)
        return;

    if(tel_conf.connection_downsample){
        if(can_network == 0){
            if((1.0e6 / tel_conf.connection_downsample_mps) < (timestamp - timers["primary_" + to_string(dev_idx)])){
                timers["primary_" + to_string(dev_idx)] = timestamp;
                return;
            }
        }
        else if(can_network == 1){
            if((1.0e6 / tel_conf.connection_downsample_mps) < (timestamp - timers["secondary_" + to_string(dev_idx)])){
                timers["secondary_" + to_string(dev_idx)] = timestamp;
                return;
            }
        }
    }

    unique_lock<mutex> lck(mtx);
    if(can_network == 0) {
        primary_proto_serialize_from_id(msg.can_id, &primary_pack, primary_devs);
        if(msg.can_id == primary_ID_INV_L_RESPONSE){
            inverter_serialize(
                (*primary_devs)[primary_INDEX_INV_L_RESPONSE].message_raw,
                msg.can_id,
                inverter_proto.mutable_inverter_l()
            );
        }else if(msg.can_id == primary_ID_INV_R_RESPONSE){
            inverter_serialize(
                (*primary_devs)[primary_INDEX_INV_R_RESPONSE].message_raw,
                msg.can_id,
                inverter_proto.mutable_inverter_r()
            );
        }
    } else if(can_network == 1) {
        secondary_proto_serialize_from_id(msg.can_id, &secondary_pack, secondary_devs);
    }
}
