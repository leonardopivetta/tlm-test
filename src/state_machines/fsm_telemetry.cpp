/******************************************************************************
Finite State Machine
Project: Telemetry
Description: The Finite State Machine that manage the Telemetry

Generated by gv_fsm ruby gem, see https://rubygems.org/gems/gv_fsm
gv_fsm version 0.3.3
Generation date: 2022-07-20 19:17:54 +0200
Generated from: smtest.dot
The finite state machine has:
  6 states
  5 transition functions
******************************************************************************/

#include "state_machines/fsm_telemetry.h"

// SEARCH FOR Your Code Here FOR CODE INSERTION POINTS!

// GLOBALS
// State human-readable names
const char *state_names[] = {"uninitialized", "init", "idle", "run", "stop", "error", "num_states", "nochange"};

// List of state functions
state_func_t *const state_table[NUM_STATES] = {
	do_uninitialized,  // in state uninitialized
	do_init,           // in state init
	do_idle,           // in state idle
	do_run,            // in state run
	do_stop,           // in state stop
	do_error,          // in state error
};

// Table of transition functions
transition_func_t *const transition_table[NUM_STATES][NUM_STATES] = {
	/* states:           uninitialized, init   , idle   , run    , stop   , error   */
	/* uninitialized */ {NULL   , to_init, NULL   , NULL   , NULL   , NULL   }, 
	/* init          */ {reset  , NULL   , to_idle, NULL   , NULL   , NULL   }, 
	/* idle          */ {reset  , to_init, NULL   , to_run , NULL   , NULL   }, 
	/* run           */ {NULL   , NULL   , NULL   , NULL   , to_stop, NULL   }, 
	/* stop          */ {reset  , to_init, NULL   , NULL   , NULL   , NULL   }, 
	/* error         */ {reset  , to_init, NULL   , NULL   , NULL   , NULL   }, 
};

/*  ____  _        _
 * / ___|| |_ __ _| |_ ___
 * \___ \| __/ _` | __/ _ \
 *  ___) | || (_| | ||  __/
 * |____/ \__\__,_|\__\___|
 *
 *   __                  _   _
 *  / _|_   _ _ __   ___| |_(_) ___  _ __  ___
 * | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */

// Function to be executed in state uninitialized
// valid return states: STATE_INIT
state_t do_uninitialized(state_data_t *data) {
	static state_t next_state;
	next_state = STATE_INIT;

	CONSOLE.LogStatus("[FSM] In state uninitialized");
	/* Your Code Here */

	switch(next_state) {
		case NO_CHANGE:
		case STATE_INIT:
			break;
		default:
			CONSOLE.LogWarn(
				   "[FSM] Cannot pass from uninitialized to",
				   state_names[next_state],
				   ", remaining in this state"
				   );
			next_state = NO_CHANGE;
	}
	return next_state;
}

// Function to be executed in state init
// valid return states: STATE_UNINITIALIZED, STATE_IDLE
state_t do_init(state_data_t *data) {
	static state_t next_state;
	next_state = STATE_UNINITIALIZED;

	CONSOLE.LogStatus("[FSM] In state init");
	
	/* Your Code Here */
	cpu_total_load_init();
	cpu_process_load_init();

	data->kill_threads.store(false);
	// Loading json configurations
	CONSOLE.Log("Loading all config");
	data->LoadAllConfig();
	TEL_ERROR_CHECK
	CONSOLE.Log("Done");

	CONSOLE.Log("Opening log folders");
	data->FOLDER_PATH = data->HOME_PATH + "/logs";
	data->OpenLogFolder(data->FOLDER_PATH);
	TEL_ERROR_CHECK
	CONSOLE.Log("Done");

	// FOLDER_PATH/<date>/<session>
	data->sesh_config.Date = data->GetDate();
	data->FOLDER_PATH += "/" + data->sesh_config.Date;
	if(!path_exists(data->FOLDER_PATH)) {
		create_directory(data->FOLDER_PATH);
	}

	CONSOLE.Log("Opening can socket");
	data->OpenCanSocket();
	TEL_ERROR_CHECK
	CONSOLE.Log("Done");

	CONSOLE.Log("Chimera and WS instances");
	if(data->tel_conf.connection.mode == "ZMQ") {
		data->pub_connection = new ZmqConnection();
		data->sub_connection = new ZmqConnection();
	} else if(data->tel_conf.connection.mode == "WEBSOCKET") {
		data->pub_connection = new WSConnection();
		data->sub_connection = data->pub_connection;
	}
	data->ws_conn_thread = new thread(&SharedData::ConnectToWS, data);
	data->actions_thread = new thread(&SharedData::ActionThread, data);
	CONSOLE.Log("DONE");
	TEL_ERROR_CHECK

	data->SetupGps();
	TEL_ERROR_CHECK

	if(data->tel_conf.camera_enable) {
#ifdef WITH_CAMERA
		CamInitData initData;
		initData.framerate = 24;
		initData.width = 320;
		initData.height = 240;
		CONSOLE.Log("Initializing Camera");
		data->camera.Init(&initData);
		CONSOLE.Log("Done");
#endif
	}

	next_state = STATE_IDLE;
	//InternalEvent(data->ST_IDLE);
	CONSOLE.LogStatus("INIT DONE");

	switch(next_state) {
	case NO_CHANGE:
	case STATE_UNINITIALIZED:
	case STATE_IDLE:
		break;
	default:
		CONSOLE.LogWarn(
			"[FSM] Cannot pass from init to",
			state_names[next_state],
			", remaining in this state");
		next_state = NO_CHANGE;
	}
	return next_state;
}

// Function to be executed in state idle
// valid return states: STATE_UNINITIALIZED, STATE_INIT, STATE_RUN
state_t do_idle(state_data_t *data) {
	static state_t next_state;
	next_state = NO_CHANGE;

	/* Your Code Here */
	static SharedData::CAN_Message message_q;
	static can_frame message;
	static uint64_t timestamp;

	static FILE *fout;
  	static int dev_idx;

	{
		unique_lock<mutex> lck(data->can_mutex);
		while(data->messages_queue.size() == 0) {
			data->can_cv.wait(lck);
		}
		message_q = data->messages_queue.front();
		data->messages_queue.pop();
    }

	timestamp = message_q.timestamp;
	data->can_stat.Messages++;
	data->msgs_counters[message_q.receiver_name]++;
	message = message_q.frame;

	if(message_q.receiver_name == "primary" && primary_is_message_id(message.can_id)) {
		dev_idx = primary_index_from_id(message.can_id);
		primary_deserialize_from_id(message.can_id, message.data, (*data->primary_devs)[dev_idx].message_raw, (*data->primary_devs)[dev_idx].message_conversion, timestamp);
		if(dev_idx == primary_INDEX_INV_L_RESPONSE){
			parse_inverter((primary_message_INV_L_RESPONSE *)(*data->primary_devs)[dev_idx].message_raw, message.can_id);
		}else if(dev_idx == primary_INDEX_INV_R_RESPONSE){
			parse_inverter((primary_message_INV_L_RESPONSE *)(*data->primary_devs)[dev_idx].message_raw, message.can_id);
		}
		switch(message.can_id) {
			case primary_ID_SET_TLM_STATUS:
				{
					primary_message_SET_TLM_STATUS *msg = ((primary_message_SET_TLM_STATUS *)(*data->primary_devs)[dev_idx].message_raw);
					if(msg->tlm_status == primary_Toggle_ON) {
						next_state = STATE_RUN;
					}
				}
			break;
			case primary_ID_SET_CAR_STATUS:
				{
					primary_message_SET_CAR_STATUS *msg = ((primary_message_SET_CAR_STATUS *)(*data->primary_devs)[dev_idx].message_raw);
					if(msg->car_status_set == primary_SetCarStatus_READY || msg->car_status_set == primary_SetCarStatus_DRIVE) {
						next_state = STATE_RUN;
					}
				}
				break;
		}
		data->ProtoSerialize(0, timestamp, message, dev_idx);
	}

	if(message_q.receiver_name == "secondary" && secondary_is_message_id(message.can_id)) {
		dev_idx = secondary_index_from_id(message.can_id);
      	secondary_deserialize_from_id(message.can_id, message.data, (*data->secondary_devs)[dev_idx].message_raw, (*data->secondary_devs)[dev_idx].message_conversion, timestamp);
      	data->ProtoSerialize(1, timestamp, message, dev_idx);
	}

	if(data->wsRequestState == STATE_UNINITIALIZED) {
		data->wsRequestState = NUM_STATES;
		next_state = STATE_UNINITIALIZED;
	}

	if(data->wsRequestState == STATE_RUN) {
		data->wsRequestState = NUM_STATES;
		next_state = STATE_RUN;
	}

	switch(next_state) {
		case NO_CHANGE:
		case STATE_UNINITIALIZED:
		case STATE_INIT:
		case STATE_RUN:
			break;
		default:
			CONSOLE.LogWarn(
				   "[FSM] Cannot pass from idle to",
				   state_names[next_state],
				   ", remaining in this state");
			next_state = NO_CHANGE;
	}
	return next_state;
}

// Function to be executed in state run
// valid return states: STATE_STOP
state_t do_run(state_data_t *data) {
	static state_t next_state;
	next_state = NO_CHANGE;

	data->zero_timestamp = get_timestamp_u();

	static can_frame message;
	static SharedData::CAN_Message message_q;
	static uint64_t timestamp;
	static FILE *csv_out;
	static int dev_idx = 0;
	static void *canlib_message;

	{
		unique_lock<mutex> lck(data->can_mutex);
		while(data->messages_queue.size() == 0) {
			data->can_cv.wait(lck);
		}
		message_q = data->messages_queue.front();
		data->messages_queue.pop();
    }

	timestamp = message_q.timestamp;
    data->can_stat.Messages++;
    data->msgs_counters[message_q.receiver_name]++;
    message = message_q.frame;

    data->LogCan(message_q);

	// Parse the message only if is needed
    // Parsed messages are for sending via websocket or to be logged in csv
    if(data->tel_conf.generate_csv || data->tel_conf.connection_enabled) {
        if(message_q.receiver_name == "primary" && primary_is_message_id(message.can_id)) {
            dev_idx = primary_index_from_id(message.can_id);
            primary_deserialize_from_id(message.can_id, message.data, (*data->primary_devs)[dev_idx].message_raw, (*data->primary_devs)[dev_idx].message_conversion, timestamp);
            if(data->tel_conf.generate_csv) {
                csv_out = data->primary_files[dev_idx];
                if((*data->primary_devs)[dev_idx].message_conversion == NULL) {
                    primary_to_string_file_from_id(message.can_id, (*data->primary_devs)[dev_idx].message_raw, csv_out);
				} else {
                    primary_to_string_file_from_id(message.can_id, (*data->primary_devs)[dev_idx].message_conversion, csv_out);
				}

                fprintf(csv_out, "\n");
                fflush(csv_out);

				if(dev_idx == primary_INDEX_INV_L_RESPONSE){
					parse_inverter((primary_message_INV_L_RESPONSE *)(*data->primary_devs)[dev_idx].message_raw, message.can_id);
				}else if(dev_idx == primary_INDEX_INV_R_RESPONSE){
					parse_inverter((primary_message_INV_L_RESPONSE *)(*data->primary_devs)[dev_idx].message_raw, message.can_id);
				}
                if(message.can_id == primary_ID_INV_L_RESPONSE) {
                    inverter_to_string((primary_message_INV_L_RESPONSE*)(*data->primary_devs)[primary_INDEX_INV_L_RESPONSE].message_raw, message.can_id, data->inverter_files);
                }else if(message.can_id == primary_ID_INV_R_RESPONSE){
                    inverter_to_string((primary_message_INV_L_RESPONSE*)(*data->primary_devs)[primary_INDEX_INV_R_RESPONSE].message_raw, message.can_id, data->inverter_files);
				}
            }

            data->ProtoSerialize(0, timestamp, message, dev_idx);
			switch(message.can_id) {
			case primary_ID_SET_TLM_STATUS:
				{
					primary_message_SET_TLM_STATUS *msg = ((primary_message_SET_TLM_STATUS *)(*data->primary_devs)[dev_idx].message_raw);
					if(msg->tlm_status == primary_Toggle_OFF) {
						next_state = STATE_STOP;
					}
				}
			break;
			case primary_ID_SET_CAR_STATUS:
				{
					primary_message_SET_CAR_STATUS *msg = ((primary_message_SET_CAR_STATUS *)(*data->primary_devs)[dev_idx].message_raw);
					if(msg->car_status_set == primary_SetCarStatus_IDLE) {
						next_state = STATE_STOP;
					}
				}
				break;
			}
        }
        if(message_q.receiver_name == "secondary" && secondary_is_message_id(message.can_id)) {
            dev_idx = secondary_index_from_id(message.can_id);
            secondary_deserialize_from_id(message.can_id, message.data, (*data->secondary_devs)[dev_idx].message_raw, (*data->secondary_devs)[dev_idx].message_conversion, timestamp);
            if(data->tel_conf.generate_csv) {
                csv_out = data->secondary_files[dev_idx];
                if((*data->secondary_devs)[dev_idx].message_conversion == NULL) {
                    secondary_to_string_file_from_id(message.can_id, (*data->secondary_devs)[dev_idx].message_raw, csv_out);
				} else {
                    secondary_to_string_file_from_id(message.can_id, (*data->secondary_devs)[dev_idx].message_conversion, csv_out);
				}
                fprintf(csv_out, "\n");
                fflush(csv_out);
            }
            data->ProtoSerialize(1, timestamp, message, dev_idx);
        }
    }

	// Save stat data each 10 seconds
	if(get_timestamp_u() - data->timers["stat"] > 10000000){
		data->SaveStat();
		data->timers["stat"] = get_timestamp_u();
	}

    // Stop message
    if(data->wsRequestState == STATE_STOP) {
        data->wsRequestState = NUM_STATES;
        next_state = STATE_STOP;
		CONSOLE.LogStatus(
			   "Connection request Transitioning from",
			   state_names[data->currentState],
			   "to",
			   state_names[next_state]);
    }

	switch(next_state) {
		case NO_CHANGE:
		case STATE_STOP:
			break;
		default:
			CONSOLE.LogWarn(
				   "[FSM] Cannot pass from run to",
				   state_names[next_state],
				   ", remaining in this state");
			next_state = NO_CHANGE;
	}
	return next_state;
}

// Function to be executed in state stop
// valid return states: STATE_UNINITIALIZED, STATE_INIT
state_t do_stop(state_data_t *data) {
	static state_t next_state;
	
	next_state = STATE_IDLE;

	//CONSOLE.LogStatus("[FSM] In state stop");

	/* Your Code Here */
	unique_lock<mutex> lck(data->mtx);
	// duration of the log
	data->can_stat.Duration_seconds = (get_timestamp_u() - data->can_stat.Duration_seconds)/1000000;

	CONSOLE.Log("Closing files");
	if(data->tel_conf.generate_csv) {
		// Close all csv files and the dump file
		// chimera->close_all_files();
		for(int i = 0; i < primary_MESSAGE_COUNT; i++) {
			if(data->primary_files[i] != NULL) {
				fclose(data->primary_files[i]);
				data->primary_files[i] = NULL;
			}
		}
		for(int i = 0; i < secondary_MESSAGE_COUNT; i++) {
			if(data->secondary_files[i] != NULL) {
				fclose(data->secondary_files[i]);
				data->secondary_files[i] = NULL;
			}
		}
		inverter_close_files(data->inverter_files);
	}
	data->dump_file->close();
	delete data->dump_file;
	CONSOLE.Log("Done");

	CONSOLE.Log("Restarting gps loggers");
	// Stop logging but continue reading port
	for(size_t i = 0; i < data->gps_loggers.size(); i++) {
		data->gps_loggers[i]->StopLogging();
		if(data->tel_conf.gps_devices[i].enabled) {
			data->gps_loggers[i]->Start();
		}
		data->gps_class[i]->filename = "";
		data->gps_class[i]->file->flush();
		data->gps_class[i]->file->close();
	}
	CONSOLE.Log("Done");

	if(data->tel_conf.camera_enable) {
#ifdef WITH_CAMERA
		CONSOLE.Log("Stopping Camera");
		data->camera.Stop();
		CONSOLE.Log("Done");
#endif
	}

	// Save stats of this log session
	CONSOLE.Log("Saving stat: ", data->CURRENT_LOG_FOLDER);
	data->SaveStat();
	CONSOLE.Log("Done");

	next_state = STATE_IDLE;
	CONSOLE.LogStatus("STOP DONE");

	switch(next_state) {
		case STATE_UNINITIALIZED:
		case NO_CHANGE:
		case STATE_IDLE:
			break;
		default:
			CONSOLE.LogWarn(
				   "[FSM] Cannot pass from stop to",
				   state_names[next_state],
				   ", remaining in this state");
			next_state = NO_CHANGE;
	}

	return next_state;
}

// Function to be executed in state error
// valid return states: STATE_UNINITIALIZED, STATE_INIT
state_t do_error(state_data_t *data) {
	static state_t next_state;
	next_state = NO_CHANGE;

	CONSOLE.LogStatus("[FSM] In state error");

	/* Your Code Here */
	CONSOLE.LogError("Error occurred");
  	CONSOLE.LogStatus("ERROR");

	next_state = STATE_UNINITIALIZED;

	switch(next_state) {
		case STATE_UNINITIALIZED:
		case NO_CHANGE:
		case STATE_INIT:
			break;
		default:
			CONSOLE.LogWarn(
				"[FSM] Cannot pass from error to",
				state_names[next_state],
				", remaining in this state");
			next_state = NO_CHANGE;
	}

	return next_state;
}

/*  _____                    _ _   _
 * |_   _| __ __ _ _ __  ___(_) |_(_) ___  _ __
 *   | || '__/ _` | '_ \/ __| | __| |/ _ \| '_ \
 *   | || | | (_| | | | \__ \ | |_| | (_) | | | |
 *   |_||_|  \__,_|_| |_|___/_|\__|_|\___/|_| |_|
 *
 *   __                  _   _
 *  / _|_   _ _ __   ___| |_(_) ___  _ __  ___
 * | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 * |  _| |_| | | | | (__| |_| | (_) | | | \__ \
 * |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 */

// This function is called in 4 transitions:
// 1. from uninitialized to init
// 2. from idle to init
// 3. from stop to init
// 4. from error to init
state_t to_init(state_data_t *data) {
	CONSOLE.LogStatus("[FSM] State transition to_init");

	/* Your Code Here */
	static state_t next_state;
	next_state = STATE_INIT;

	return next_state;
}

// This function is called in 4 transitions:
// 1. from init to uninitialized
// 2. from idle to uninitialized
// 3. from stop to uninitialized
// 4. from error to uninitialized
state_t reset(state_data_t *data) {
	CONSOLE.LogStatus("[FSM] State transition reset");

	/* Your Code Here */
	static state_t next_state;
	next_state = STATE_UNINITIALIZED;

	/////////////////////////
	// LAP COUNTER DESTROY //
	/////////////////////////
	// if(lp != nullptr) {
	//   lc_reset(lp); // reset object (removes everything but thresholds)
	//   lc_destroy(lp);
	// }
	// if(lp_inclination != nullptr) {
	//   lc_reset(lp_inclination);
	//   lc_destroy(lp_inclination);
	// }

	data->kill_threads.store(true);

	data->KillThread(data->actions_thread, "actions");
	data->KillThread(data->data_thread, "data");
	data->KillThread(data->status_thread, "status");
	data->KillThread(data->ws_conn_thread, "reconnection");

	if(data->pub_connection != nullptr) {
		data->pub_connection->clearData();
		data->pub_connection->closeConnection();
		data->KillThread(data->pub_connection_thread, "pub");
		delete data->pub_connection;
		data->pub_connection = nullptr;
	}

	if(data->sub_connection != nullptr) {
		data->sub_connection->clearData();
		data->sub_connection->closeConnection();
		data->KillThread(data->sub_connection_thread, "sub");
		delete data->sub_connection;
		data->sub_connection = nullptr;
	}
	CONSOLE.Log("Stopped connection");

	if(data->dump_file != NULL) {
		data->dump_file->close();
		data->dump_file = NULL;
	}
	CONSOLE.Log("Closed dump file");

	for(auto &el : data->sockets) {
		el.second.sock->close_socket();
		if(el.second.thrd != nullptr) {
			CONSOLE.Log("Joining CAN thread [", el.second.name, "]");
			el.second.thrd->join();
			CONSOLE.Log("Joined CAN thread [", el.second.name, "]");
		}
	}
	CONSOLE.Log("Closed can socket");

	for(int i = 0; i < data->gps_loggers.size(); i++) {
		if(data->gps_loggers[i] == nullptr) continue;
		cout << i << endl;
		data->gps_loggers[i]->Kill();
		data->gps_loggers[i]->WaitForEnd();
		data->gps_class[i]->file->close();
		delete data->gps_class[i];
		delete data->gps_loggers[i];
	}
	data->gps_loggers.resize(0);
	data->gps_class.resize(0);
	CONSOLE.Log("Closed gps loggers");

	// if(chimera != nullptr) {
	//   chimera->clear_serialized();
	//   chimera->close_all_files();
	//   delete chimera;
	//   chimera = nullptr;
	// }
	data->primary_pack.Clear();
	data->secondary_pack.Clear();
	if(data->tel_conf.generate_csv) {
		// Close all csv files and the dump file
		// chimera->close_all_files();
		for(int i = 0; i < primary_MESSAGE_COUNT; i++) {
			if(data->primary_files[i] != NULL) {
				fclose(data->primary_files[i]);
				data->primary_files[i] = NULL;
			}
		}
		for(int i = 0; i < secondary_MESSAGE_COUNT; i++) {
			if(data->secondary_files[i] != NULL) {
				fclose(data->secondary_files[i]);
				data->secondary_files[i] = NULL;
			}
		}
	}
	CONSOLE.Log("Deleted vehicle");

	data->msgs_counters.clear();
	data->msgs_per_second.clear();
	data->timers.clear();

	data->ws_conn_state = ConnectionState_::NONE;
	data->currentError = SharedData::TelemetryError::TEL_NONE;

	data->kill_threads.store(false);

	CONSOLE.Log("Cleared variables");

	CONSOLE.LogStatus("DEINITIALIZE DONE");

	return next_state;
}

// This function is called in 1 transition:
// 1. from init to idle
state_t to_idle(state_data_t *data) {
	CONSOLE.LogStatus("[FSM] State transition to_idle");

	/* Your Code Here */
	static state_t next_state;
	next_state = STATE_IDLE;

	data->StartGps();

	return next_state;
}

// This function is called in 1 transition:
// 1. from idle to run
state_t to_run(state_data_t *data) {
	CONSOLE.LogStatus("[FSM] State transition to_run");

	state_t next_state = STATE_RUN;
	data->zero_timestamp = get_timestamp_u();

	/* Your Code Here */
	data->sesh_config.Time = data->GetTime();
	
	static string folder;
	static string subfolder;
	data->CreateFolderName(subfolder);

	CONSOLE.Log("Creting new directory");
	// Adding incremental number at the end of foldername
	int folder_i = 1;
	do {
		folder = data->FOLDER_PATH + "/" + subfolder + " " + to_string(folder_i);
		folder_i++;
	} while(path_exists(folder));
	create_directory(folder);

	data->CURRENT_LOG_FOLDER = folder;
	CONSOLE.Log("Log folder: ", data->CURRENT_LOG_FOLDER);
	CONSOLE.Log("Done");

	CONSOLE.Log("Initializing loggers, and csv files");

	data->dump_file = new fstream((data->CURRENT_LOG_FOLDER + "/" + "candump.log").c_str(), std::fstream::out);
	if(!data->dump_file->is_open()) {
		CONSOLE.LogError("Error opening candump file!");
		data->EmitError(data->TEL_LOG_FOLDER);
		TEL_ERROR_CHECK
	}

	///////
	if(data->tel_conf.generate_csv) {
		if(!path_exists(data->CURRENT_LOG_FOLDER + "/Parsed"))
			create_directory(data->CURRENT_LOG_FOLDER + "/Parsed");
		if(!path_exists(data->CURRENT_LOG_FOLDER + "/Parsed/primary"))
			create_directory(data->CURRENT_LOG_FOLDER + "/Parsed/primary");
		if(!path_exists(data->CURRENT_LOG_FOLDER + "/Parsed/secondary"))
			create_directory(data->CURRENT_LOG_FOLDER + "/Parsed/secondary");

		static char buff[125];
		static string folder;
		for(int i = 0; i < primary_MESSAGE_COUNT; i++) {
			primary_message_name_from_id((*data->primary_devs)[i].id, buff);
			folder = (data->CURRENT_LOG_FOLDER + "/Parsed/primary/" + string(buff) + ".csv");
			data->primary_files[i] = fopen(folder.c_str(), "w");
			primary_fields_file_from_id((*data->primary_devs)[i].id, data->primary_files[i]);
			fprintf(data->primary_files[i], "\r\n");
			fflush(data->primary_files[i]);
		}

		for(int i = 0; i < secondary_MESSAGE_COUNT; i++) {
			secondary_message_name_from_id((*data->secondary_devs)[i].id, buff);
			folder = (data->CURRENT_LOG_FOLDER + "/Parsed/secondary/" + string(buff) + ".csv");
			data->secondary_files[i] = fopen(folder.c_str(), "w");
			secondary_fields_file_from_id((*data->secondary_devs)[i].id, data->secondary_files[i]);
			fprintf(data->secondary_files[i], "\r\n");
			fflush(data->secondary_files[i]);
		}

		folder = data->CURRENT_LOG_FOLDER + "/Parsed/primary/";
		inverter_open_files(folder, data->inverter_files);
	}
	CONSOLE.Log("CSV Done");

	for(size_t i = 0; i < data->gps_loggers.size(); i++) {
		if(!data->tel_conf.gps_devices[i].enabled) continue;
		data->gps_loggers[i]->SetOutputFolder(data->CURRENT_LOG_FOLDER);
		data->gps_class[i]->filename = folder + "/Parsed/GPS " + to_string(i) + ".csv";
		data->gps_class[i]->file = new fstream(data->gps_class[i]->filename, std::fstream::out);
	}
	CONSOLE.Log("Loggers DONE");

	data->can_stat.Messages = 0;
	data->can_stat.Duration_seconds = get_timestamp_u();
	data->timers["stat"] = get_timestamp_u();
	data->SaveStat();

	for(auto logger : data->gps_loggers) {
		logger->StartLogging();
	}

	if(data->tel_conf.camera_enable) {
#ifdef WITH_CAMERA
		CamRunData runData;
		runData.filename = data->CURRENT_LOG_FOLDER + "/" + "onboard.avi";
		CONSOLE.Log("Starting camera");
		data->camera.Run(&runData);
		CONSOLE.Log("Done");
#endif
	}
	CONSOLE.LogStatus("TO_RUN DONE");

	return next_state;
}

// This function is called in 1 transition:
// 1. from run to stop
state_t to_stop(state_data_t *data) {
	CONSOLE.LogStatus("[FSM] State transition to_stop");

	/* Your Code Here */
	static state_t next_state;
	next_state = STATE_STOP;

	return next_state;
}

/*  ____  _        _
 * / ___|| |_ __ _| |_ ___
 * \___ \| __/ _` | __/ _ \
 *  ___) | || (_| | ||  __/
 * |____/ \__\__,_|\__\___|
 *
 *
 *  _ __ ___   __ _ _ __   __ _  __ _  ___ _ __
 * | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '__|
 * | | | | | | (_| | | | | (_| | (_| |  __/ |
 * |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_|
 *                              |___/
 */

state_t run_state(state_t cur_state, state_data_t *data) {
	state_t new_state = state_table[cur_state](data);

	if(new_state == NO_CHANGE) {
		new_state = cur_state;
	}

	transition_func_t *transition = transition_table[cur_state][new_state];

	if(transition) {
		new_state = transition(data);
	}
	
	return new_state == NO_CHANGE ? cur_state : new_state;
};

#ifdef TEST_MAIN
#include <unistd.h>
int main() {
	state_t cur_state = STATE_UNINITIALIZED;
	openlog("SM", LOG_PID | LOG_PERROR, LOG_USER);
	CONSOLE.LogStatus("Starting SM");
	do {
		cur_state = run_state(cur_state, NULL);
		sleep(1);
	} while(1);
	return 0;
}
#endif
