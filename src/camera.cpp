#include "camera.h"

Camera::Camera() :
	StateMachine(ST_MAX_STATES)
{
	SetError(CAM_NONE);

	StatesStr[ST_NONE] = "ST_NONE";
	StatesStr[ST_INIT] = "ST_INIT";
	StatesStr[ST_IDLE] = "ST_IDLE";
	StatesStr[ST_RUN] = "ST_RUN";
	StatesStr[ST_STOP] = "ST_STOP";
	StatesStr[ST_ERROR] = "ST_ERROR";
}

void Camera::Init(CamInitData* data)
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (ST_INIT)							// ST_NONE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_INIT
		TRANSITION_MAP_ENTRY (ST_INIT)							// ST_IDLE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_RUN
		TRANSITION_MAP_ENTRY (ST_INIT)				      // ST_STOP
		TRANSITION_MAP_ENTRY (ST_INIT)				      // ST_ERROR
	END_TRANSITION_MAP(data)
}

void Camera::Run(CamRunData* data)
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_NONE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_INIT
		TRANSITION_MAP_ENTRY (ST_RUN)								// ST_IDLE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_RUN
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_STOP
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_ERROR
	END_TRANSITION_MAP(data)
}

void Camera::Stop(){
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_NONE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_INIT
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_IDLE
		TRANSITION_MAP_ENTRY (ST_STOP)				      // ST_RUN
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_STOP
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_ERROR
	END_TRANSITION_MAP(&no_data)
}

// Define state NONE function
STATE_DEFINE(Camera, NoneImpl, NoEventData)
{
	currentError = CamError::CAM_NONE;
}

// Define state IDLE function
STATE_DEFINE(Camera, IdleImpl, NoEventData)
{
	CONSOLE.Log("Camera Idling");
}

// Define state INIT function
STATE_DEFINE(Camera, InitImpl, CamInitData)
{
	currentError = CamError::CAM_NONE;

	if(raspi_cam != nullptr)  
		delete raspi_cam;

  raspi_cam = new raspicam::RaspiCam();

  raspi_cam->setFormat(raspicam::RASPICAM_FORMAT_RGB);
  raspi_cam->setExposure(raspicam::RASPICAM_EXPOSURE_AUTO);
  raspi_cam->setAWB(raspicam::RASPICAM_AWB_AUTO);
  raspi_cam->setImageEffect(raspicam::RASPICAM_IMAGE_EFFECT_NONE);
  raspi_cam->setVideoStabilization(true);

  raspi_cam->setWidth(data->width);
  raspi_cam->setHeight(data->height);
  raspi_cam->setFrameRate(data->framerate);


  //Open raspi_cam 
  CONSOLE.Log("Opening raspi_cam");
#ifdef __arm__
	if ( !raspi_cam->open(false)) {
		error_data.error = CamError::CAM_OPENING_CAM;
		InternalEvent(ST_ERROR, &error_data);
		return;
	}
  CONSOLE.Log("Done");
#else
	error_data.error = CamError::CAM_NOT_RASPI;
	InternalEvent(ST_ERROR, &error_data);
	return;
#endif


  //wait a while until camera stabilizes
	CONSOLE.Log("Sleeping");
  usleep(1000000);
	CONSOLE.Log("Done");

	size_t framesize = raspi_cam->getImageTypeSize ( raspicam::RASPICAM_FORMAT_RGB );
	img_data = new unsigned char[framesize];
	img = new cv::Mat(data->height, data->width, CV_8UC3);

	cam_config_data.framerate = data->framerate;
	cam_config_data.framesize = cv::Size(data->width, data->height);

	InternalEvent(ST_IDLE, &no_data);
}

// Define state RUN function
STATE_DEFINE(Camera, RunImpl, CamRunData)
{
	CONSOLE.Log("Initializing VideoWriter");
	
	if(!writer.open(data->filename, cv::VideoWriter::fourcc('M','J','P','G'), cam_config_data.framerate, cam_config_data.framesize, true))
	{
		error_data.error = CamError::CAM_OPENING_FILE;
		InternalEvent(ST_ERROR, &error_data);
		return;
	}
  CONSOLE.Log("Done");

	cam_config_data.fname = data->filename;

	save_thread = new thread(&Camera::SaveLoop, this);
}

// Define state STOP function
STATE_DEFINE(Camera, StopImpl, NoEventData)
{
	if(save_thread != nullptr)
	{
		CONSOLE.Log("Joining Threads");
		save_thread->join();
		delete save_thread;
	}

	writer.release();

	InternalEvent(ST_IDLE, &no_data);
}

// Define state ERROR function
STATE_DEFINE(Camera, ErrorImpl, ErrorData)
{
	SetError(data->error);
	CONSOLE.LogError(CamErrorStr[data->error]);

	if(save_thread != nullptr)
	{
		CONSOLE.Log("Joining Threads");
		save_thread->join();
	}
}

CamError Camera::GetError()
{
	CamError err = currentError;
	return err;
}

void Camera::SetError(const CamError& err)
{
	currentError = err;
}


void Camera::SaveLoop()
{
	usleep(1000);
	raspi_cam->startCapture();
	while (true)
	{
		if(GetCurrentState() != ST_RUN)
			break;
		
		if(raspi_cam == nullptr)
		{
			error_data.error = CamError::CAM_NOT_INITIALIZED;
			InternalEvent(ST_ERROR, &error_data);
			return;
		}

		double t0 = get_timestamp_u();

		raspi_cam->grab();
		// extract the image in rgb format
		raspi_cam->retrieve ( img_data ); // get camera image

		img->data = img_data;
		if(!img->empty())
		{
			cv::cvtColor(*img, *img, cv::COLOR_BGR2RGB);
			writer.write(*img);
		}
		else
		{
			CONSOLE.LogWarn("Empty Image");
		}

		double t1 = get_timestamp_u();

		usleep((1.0 / cam_config_data.framerate) - (t1 - t0));
	}
}