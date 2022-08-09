#pragma once

#include <ctime>
#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <iostream>
#include <exception>
#include <unordered_map>

#include <mutex>
#include <thread>
#include <condition_variable>

#include <raspicam/raspicam.h>
#include <opencv4/opencv2/opencv.hpp>



#include "utils.h"
#include "console.h"
#include "state_machines/class_implementation/StateMachine.h"


using namespace std;
using namespace std::chrono;




/**
 * This is a state machine:
 * The process is to init the camera (setting framerate and framesize)
 *  then the SM should automatically change state to IDLE waiting for a run command.
 * When calling Run() function (which requires filename of the output video)
 *  the SM lauches a thread that starts saving the raspicam video.
 * To stop logging call Stop().
 * If errors occurs during one of the previous steps the SM goes into ERROR state.
 * The error can be retrieved using GetError().
 * To be able to restart after an error the Init() function MUST be called.
 * The SM cannot change state to IDLE or RUN while being in ERROR state.
 * 
 * After Init is called if the SM does not change state to IDLE an error occurred.
**/

enum CamError
{
	CAM_NONE,
	CAM_OPENING_CAM,
	CAM_NOT_RASPI,
	CAM_OPENING_FILE,
	CAM_NOT_INITIALIZED,
	CAM_GENERAL_ERROR
};

static string CamErrorStr[]
{
	"NONE",
	"OPENING CAMERA",
	"NOT ON RASPI",
	"OPENING FILE",
	"CAM NOT INITIALIZED",
	"GENERAL ERROR"
};


class CamInitData : public EventData
{
public:
  uint framerate;
	uint width;
	uint height;
};

class CamRunData : public EventData
{
public:
  string filename;
};

class ErrorData : public EventData
{
public:
  CamError error;
};

class Camera : public StateMachine
{
public:
  Camera();

	// Init the raspicam
  void Init(CamInitData*);
	// Starts saving the video
  void Run(CamRunData*);
	// Stops savig the video
  void Stop();


	// returns the current error
	CamError GetError();

private:
	// State enumeration order must match the order of state method entries
	// in the state map.
	enum States
	{
		ST_NONE,
		ST_INIT,
		ST_IDLE,
		ST_RUN,
		ST_STOP,
		ST_ERROR,
		ST_MAX_STATES
	};

	NoEventData no_data;
	ErrorData error_data;
	CamError currentError;
	cv::VideoWriter writer;

	mutex mtx;
	condition_variable event;
	thread* save_thread = nullptr;

	cv::Mat* img;
	unsigned char *img_data;

	raspicam::RaspiCam* raspi_cam = nullptr;

	struct CamConfigData
	{
		uint framerate;
		string fname;
		cv::Size framesize;
	};

	CamConfigData cam_config_data;
public:
	string StatesStr[ST_MAX_STATES];

private:
	void SetError(const CamError&);

	void SaveLoop();

private:

	// Define the state machine state functions with event data type
	STATE_DECLARE(Camera, 	NoneImpl,			NoEventData)
	STATE_DECLARE(Camera, 	InitImpl,			CamInitData)
	STATE_DECLARE(Camera, 	IdleImpl,			NoEventData)
	STATE_DECLARE(Camera, 	RunImpl,			CamRunData)
	STATE_DECLARE(Camera, 	StopImpl,			NoEventData)
	STATE_DECLARE(Camera, 	ErrorImpl,		ErrorData)

	// State map to define state object order. Each state map entry defines a
	// state object.
	BEGIN_STATE_MAP
		STATE_MAP_ENTRY(&NoneImpl)
		STATE_MAP_ENTRY(&InitImpl)
		STATE_MAP_ENTRY(&IdleImpl)
		STATE_MAP_ENTRY(&RunImpl)
		STATE_MAP_ENTRY(&StopImpl)
		STATE_MAP_ENTRY(&ErrorImpl)
	END_STATE_MAP

};