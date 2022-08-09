#include "report.h"

jmp_buf env;

int Report::instances = 0;

void error_handler  (HPDF_STATUS   error_no,
                HPDF_STATUS   detail_no,
                void         *user_data)
{
    printf ("HPDF ERROR: error_no=%04X, detail_no=%u   %04X\n", (HPDF_UINT)error_no,
                (HPDF_UINT)detail_no, (HPDF_UINT)detail_no);
    throw runtime_error("error");
    longjmp(env, 1);
}

void
show_description (HPDF_Page    page,
                  float        x,
                  float        y,
                  const char  *text)
{
    char buf[255];

    HPDF_Page_MoveTo (page, x, y - 10);
    HPDF_Page_LineTo (page, x, y + 10);
    HPDF_Page_MoveTo (page, x - 10, y);
    HPDF_Page_LineTo (page, x + 10, y);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetFontAndSize (page, HPDF_Page_GetCurrentFont (page), 8);
    HPDF_Page_SetRGBFill (page, 0, 0, 0);

    HPDF_Page_BeginText (page);

#ifdef __WIN32__
    _snprintf(buf, 255, "(x=%d,y=%d)", (int)x, (int)y);
#else
    snprintf(buf, 255, "(x=%d,y=%d)", (int)x, (int)y);
#endif /* __WIN32__ */
    HPDF_Page_MoveTextPos (page, x - HPDF_Page_TextWidth (page, buf) - 5,
            y - 10);
    HPDF_Page_ShowText (page, buf);
    HPDF_Page_EndText (page);

    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, x - 20, y - 25);
    HPDF_Page_ShowText (page, text);
    HPDF_Page_EndText (page);
}

void Report::AddDeviceSample(Chimera* chim, Device* device)
{
  static int discard_distance_count = 0;
  static int discard_voltage_count = 0;
  if (device == chim->accel){
    Imu* accel = (Imu*) device;
    if(first_timestamp < 0.0)
      first_timestamp = accel->timestamp;
    auto tim = accel->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["accel"]["timestamp"].push_back(tim);
    sensor_data["accel"]["x"].push_back(accel->x);
    sensor_data["accel"]["y"].push_back(accel->y);
    sensor_data["accel"]["z"].push_back(accel->z);
    max_g = max(max_g, max(fabs(accel->x), fabs(accel->y)));
  }else if(device == chim->gyro){
    Imu* gyro = (Imu*) device;
    if(first_timestamp < 0.0)
      first_timestamp = gyro->timestamp;
    auto tim = gyro->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["gyro"]["timestamp"].push_back(tim);
    sensor_data["gyro"]["x"].push_back(gyro->x);
    sensor_data["gyro"]["y"].push_back(gyro->y);
    sensor_data["gyro"]["z"].push_back(gyro->z);
  }else if(device == chim->encoder_left){
    Encoder* enc = (Encoder*) device;
    if(first_timestamp < 0.0)
      first_timestamp = enc->timestamp;
    auto tim = enc->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["encoder_l"]["timestamp"].push_back(tim);
    sensor_data["encoder_l"]["rads"].push_back(enc->rads);
    sensor_data["encoder_l"]["km"].push_back(enc->km);
    sensor_data["encoder_l"]["rotations"].push_back(enc->rotations);
    sensor_data["encoder_l"]["kmh"].push_back((enc->rads* 0.395/2.0)*3.6);
    max_speed = max(max_speed, fabs(enc->rads));
    if(distance_travelled.setted)
    {
      distance_travelled.val1 = enc->km;
    }
    else
    {
      if(discard_distance_count >= 10)
      {
        distance_travelled.setted = true;
        distance_travelled.val0 = enc->km;
      }
      else
      {
        discard_distance_count ++;
      }
    }
  }else if(device == chim->encoder_right){
    Encoder* enc = (Encoder*) device;
    if(first_timestamp < 0.0)
      first_timestamp = enc->timestamp;
    auto tim = enc->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["encoder_r"]["timestamp"].push_back(tim);
    sensor_data["encoder_r"]["rads"].push_back(enc->rads);
    sensor_data["encoder_r"]["km"].push_back(enc->km);
    sensor_data["encoder_r"]["rotations"].push_back(enc->rotations);
    sensor_data["encoder_r"]["kmh"].push_back((enc->rads* 0.395/2.0)*3.6);
    max_speed = max(max_speed, fabs(enc->rads));
  }else if(device == chim->inverter_right){
    Inverter* inv = (Inverter*) device;
    if(first_timestamp < 0.0)
      first_timestamp = inv->timestamp;
    auto tim = inv->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["inverter_r"]["timestamp"].push_back(tim);
    sensor_data["inverter_r"]["temperature"].push_back(inv->temperature);
    sensor_data["inverter_r"]["motor_temp"].push_back(inv->motor_temp);
    sensor_data["inverter_r"]["torque"].push_back(inv->torque);
    sensor_data["inverter_r"]["speed"].push_back(-inv->speed);
  }else if(device == chim->inverter_left){
    Inverter* inv = (Inverter*) device;
    if(first_timestamp < 0.0)
      first_timestamp = inv->timestamp;
    auto tim = inv->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["inverter_l"]["timestamp"].push_back(tim);
    sensor_data["inverter_l"]["temperature"].push_back(inv->temperature);
    sensor_data["inverter_l"]["motor_temp"].push_back(inv->motor_temp);
    sensor_data["inverter_l"]["torque"].push_back(inv->torque);
    sensor_data["inverter_l"]["speed"].push_back(inv->speed);
  }else if(device == chim->bms_lv){
    Bms* bms = (Bms*)device;
    if(first_timestamp < 0.0)
      first_timestamp = bms->timestamp;
    auto tim = bms->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["bms_lv"]["timestamp"].push_back(tim);
    sensor_data["bms_lv"]["temperature"].push_back(bms->temperature);
    sensor_data["bms_lv"]["max_temperature"].push_back(bms->max_temperature);
    sensor_data["bms_lv"]["voltage"].push_back(bms->voltage);
  }else if(device == chim->bms_hv){
    Bms* bms = (Bms*)device;
    if(first_timestamp < 0.0)
      first_timestamp = bms->timestamp;
    auto tim = bms->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["bms_hv"]["timestamp"].push_back(tim);
    sensor_data["bms_hv"]["temperature"].push_back(bms->temperature);
    sensor_data["bms_hv"]["max_temperature"].push_back(bms->max_temperature);
    sensor_data["bms_hv"]["min_temperature"].push_back(bms->min_temperature);
    sensor_data["bms_hv"]["current"].push_back(bms->current);
    sensor_data["bms_hv"]["voltage"].push_back(bms->voltage);
    sensor_data["bms_hv"]["max_voltage"].push_back(bms->max_voltage);
    sensor_data["bms_hv"]["min_voltage"].push_back(bms->min_voltage);
    sensor_data["bms_hv"]["power"].push_back(bms->power);
    if(min_voltage_drop.setted)
    {
      min_voltage_drop.val1 = bms->min_voltage;
    }
    else
    {
      if(discard_voltage_count >= 10)
      {
        min_voltage_drop.setted = true;
        min_voltage_drop.val0 = bms->min_voltage;
      }
      else
      {
        discard_voltage_count ++;
      }
    }
    if(total_voltage_drop.setted)
    {
      total_voltage_drop.val1 = bms->voltage;
    }
    else
    {
      if(discard_voltage_count >= 10)
      {
        total_voltage_drop.setted = true;
        total_voltage_drop.val0 = bms->voltage;
      }
    }
  }else if(device == chim->pedal){
    Pedals* pedals = (Pedals*)device;
    if(first_timestamp < 0.0)
      first_timestamp = pedals->timestamp;
    auto tim = pedals->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["pedals"]["timestamp"].push_back(tim);
    sensor_data["pedals"]["throttle1"].push_back(pedals->throttle1);
    sensor_data["pedals"]["throttle2"].push_back(pedals->throttle2);
    sensor_data["pedals"]["brake_front"].push_back(pedals->brake_front);
    sensor_data["pedals"]["brake_rear"].push_back(pedals->brake_rear);
    max_brake_pressure = max(max_brake_pressure, max(pedals->brake_front, pedals->brake_rear));
  }else if(device == chim->steer){
    Steer* steer = (Steer*)device;
    if(first_timestamp < 0.0)
      first_timestamp = steer->timestamp;
    auto tim = steer->timestamp - first_timestamp;
    last_timestamp = max(last_timestamp, tim);
    sensor_data["steer"]["timestamp"].push_back(tim);
    sensor_data["steer"]["angle"].push_back(steer->angle);
  }else if(device == chim->ecu_state){
    
  }else if(device == chim->bms_hv_state){
    
  }else if(device == chim->steering_wheel_state){
    
  }else if(device == chim->ecu){
    
  }else if(device == chim->gps1){
    Gps* gps = (Gps*)device;
    if(gps->altitude != 0)
    {
      last_alt = gps->altitude;
      sensor_data["gps1"]["alt"].push_back(gps->altitude);
    }
    if(gps->latitude != 0 && gps->longitude != 0)
    {
      if(lat_0 < 0.0)
        lat_0 = gps->latitude;
      if(long_0 < 0.0)
        long_0 = gps->longitude;

      double x, y, z;
      lla2xyz(gps->latitude,
              gps->longitude,
              last_alt,
              lat_0,
              long_0,
              x,
              y,
              z);
      auto tim = gps->timestamp - first_timestamp;
      last_timestamp = max(last_timestamp, tim);
      sensor_data["gps1"]["latlong_timestamp"].push_back(tim);
      sensor_data["gps1"]["latitude"].push_back(x);
      sensor_data["gps1"]["longitude"].push_back(y);
    }
    if(gps->speed_kmh != 0.0)
    {
      auto tim = gps->timestamp - first_timestamp;
      last_timestamp = max(last_timestamp, tim);
      sensor_data["gps1"]["speed_timestamp"].push_back(tim);
      sensor_data["gps1"]["kmh"].push_back(gps->speed_kmh);
    }
    
  }else if(device == chim->gps2){
    Gps* gps = (Gps*)device;
    if(gps->altitude != 0)
    {
      last_alt = gps->altitude;
      sensor_data["gps2"]["alt"].push_back(gps->altitude);
    }
    if(gps->latitude != 0 && gps->longitude != 0 && last_alt != -1.0)
    {
      if(lat_0 < 0.0)
        lat_0 = gps->latitude;
      if(long_0 < 0.0)
        long_0 = gps->longitude;

      double x, y, z;
      lla2xyz(gps->latitude,
              gps->longitude,
              last_alt,
              lat_0,
              long_0,
              x,
              y,
              z);
      auto tim = gps->timestamp - first_timestamp;
      last_timestamp = max(last_timestamp, tim);
      sensor_data["gps2"]["latlong_timestamp"].push_back(tim);
      sensor_data["gps2"]["latitude"].push_back(x);
      sensor_data["gps2"]["longitude"].push_back(y);
    }
    if(gps->speed_kmh != 0.0)
    {
      auto tim = gps->timestamp - first_timestamp;
      last_timestamp = max(last_timestamp, tim);
      sensor_data["gps2"]["speed_timestamp"].push_back(tim);
      sensor_data["gps2"]["kmh"].push_back(gps->speed_kmh);
    }
  }
}

void Report::Filter(const vector<double> &in, vector<double>* out, int window)
{
  double mean;
  out->reserve(in.size()-window);
  for(int i = 0; i < in.size()-window; i++)
  {
    mean = 0;
    for(int j = 0; j < window; j++)
      mean += in[i+j];
    mean /= window;
    out->push_back(mean);
  }
}

void Report::Clean(int count)
{
  for(const auto& element : sensor_data)
  {
    for(const auto& subelement : sensor_data[element.first])
    {
      int pos = subelement.first.find("latitude");
      if(pos != string::npos)
        continue;
      pos = subelement.first.find("longitude");
      if(pos != string::npos)
        continue;
      vector<double> out;
      DownSample(sensor_data[element.first][subelement.first], &out, count);
      sensor_data[element.first][subelement.first].clear();
      Filter(out, &sensor_data[element.first][subelement.first]);
    }
  }
}


bool Report::CheckSize(const vector<MapElement>& sensors, size_t& minsize)
{
  bool ret = false;
  minsize = sensor_data[sensors[0].primary][sensors[0].secondary].size();
  for(auto el : sensors)
  {
    if(minsize != sensor_data[el.primary][el.secondary].size())
    {
      minsize = min(minsize, sensor_data[el.primary][el.secondary].size());
      ret = true;
    }
  }
  return ret;
}


string Report::_Odometers(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set title \"Odometers\"\n";

  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"rad/s\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"rad/s\"\n";
  gp << "show y2label\n";
  gp << "set y2tics nomirror\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'encoder left rad/s', ";
  gp << "'-' with lines axis x1y1 title 'encoder right rad/s', ";
  gp << "'-' with lines axis x1y1 title 'inverter left rad/s', ";
  gp << "'-' with lines axis x1y1 title 'inverter right rad/s'\n";

  gp.send1d(make_pair(sensor_data["encoder_l"]["timestamp"], sensor_data["encoder_l"]["rads"]));
  gp.send1d(make_pair(sensor_data["encoder_r"]["timestamp"], sensor_data["encoder_r"]["rads"]));
  gp.send1d(make_pair(sensor_data["inverter_l"]["timestamp"], sensor_data["inverter_l"]["speed"]));
  gp.send1d(make_pair(sensor_data["inverter_r"]["timestamp"], sensor_data["inverter_r"]["speed"]));

  return "graph"+fname+".png";
}

string Report::_Pedals(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set my2tics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set title \"Pedals\"\n";


  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"throttle %\"\n";
  gp << "set y2label \"pressure bar\"\n";
  gp << "set y2range [0:50]\n";
  gp << "set y2tics nomirror\n";
  gp << "show ylabel\n";
  gp << "show y2label\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'pedals throttle', ";
  gp << "'-' with lines axis x1y2 title 'pedals brake front', ";
  gp << "'-' with lines axis x1y2 title 'pedals brake rear'\n";

  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["throttle1"]));
  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["brake_front"]));
  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["brake_rear"]));

  return "graph"+fname+".png";
}

string Report::_IMU(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set multiplot layout 2, 1 title \"IMU\" tc rgb \"red\"\n";


  // Accel
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"g\"\n";
  gp << "show ylabel\n";
  gp << "set y2tics nomirror\n";
  gp << "set title \"Accel\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'accel x', ";
  gp << "'-' with lines axis x1y1 title 'accel y', ";
  gp << "'-' with lines axis x1y1 title 'accel z'\n";

  gp.send1d(make_pair(sensor_data["accel"]["timestamp"], sensor_data["accel"]["x"]));
  gp.send1d(make_pair(sensor_data["accel"]["timestamp"], sensor_data["accel"]["y"]));
  gp.send1d(make_pair(sensor_data["accel"]["timestamp"], sensor_data["accel"]["z"]));


  // Gyro
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"deg/s \"\n";
  gp << "show ylabel\n";
  gp << "set y2tics nomirror\n";
  gp << "set title \"Gyro\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'gyro x', ";
  gp << "'-' with lines axis x1y1 title 'gyro y', ";
  gp << "'-' with lines axis x1y1 title 'gyro z'\n";

  gp.send1d(make_pair(sensor_data["gyro"]["timestamp"], sensor_data["gyro"]["x"]));
  gp.send1d(make_pair(sensor_data["gyro"]["timestamp"], sensor_data["gyro"]["y"]));
  gp.send1d(make_pair(sensor_data["gyro"]["timestamp"], sensor_data["gyro"]["z"]));

  return "graph"+fname+".png";
}

string Report::_SteerAccelGyro(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set multiplot layout 2, 1 title \"Steer Accel Gyro\" tc rgb \"red\"\n";

  // Steer
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"deg °\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \" \"\n";
  gp << "set y2tics nomirror\n";
  gp << "show y2label\n";
  gp << "set title \"Steer\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'steer angle'\n";
  
  gp.send1d(make_pair(sensor_data["steer"]["timestamp"], sensor_data["steer"]["angle"]));


  // Accel Gyro
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"g \"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"deg/s \"\n";
  gp << "show y2label\n";
  gp << "set title \"Accel Gyro\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'accel x', ";
  gp << "'-' with lines axis x1y2 title 'gyro z'\n";

  gp.send1d(make_pair(sensor_data["accel"]["timestamp"], sensor_data["accel"]["x"]));
  gp.send1d(make_pair(sensor_data["gyro"]["timestamp"], sensor_data["gyro"]["z"]));

  return "graph"+fname+".png";
}

string Report::_PedalsSpeedAccel(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set multiplot layout 2, 1 title \"Throttle Brake Speed Accel\" tc rgb \"red\"\n";

  // Steer
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Throttle [%]\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"Pressure [bar]\"\n";
  gp << "set y2tics nomirror\n";
  gp << "show y2label\n";
  gp << "set title \"Throttle Brake\"\n";


  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'throttle', ";
  gp << "'-' with lines axis x1y2 title 'brake front'\n";
  
  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["throttle1"]));
  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["brake_front"]));


  // Accel Gyro
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"g \"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"   \"\n";
  gp << "show y2label\n";
  gp << "set title \"Accel \"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'accel y'\n";

  gp.send1d(make_pair(sensor_data["accel"]["timestamp"], sensor_data["accel"]["y"]));

  return "graph"+fname+".png";
}

string Report::_GPS(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set title \"GPS\"\n";


  gp << "set xlabel \"meters\"\n";
  gp << "set ylabel \"meters\"\n";
  gp << "show ylabel\n";

  gp << "plot ";
  gp << "'-' with points ps 1 pt 7 lc rgb \"blue\" axis x1y1 title 'GPS 1', ";
  gp << "'-' with points ps 1 pt 7 lc rgb \"red\" axis x1y1 title 'GPS 2'\n";

  gp.send1d(make_pair(sensor_data["gps1"]["latitude"], sensor_data["gps1"]["longitude"]));
  gp.send1d(make_pair(sensor_data["gps2"]["latitude"], sensor_data["gps2"]["longitude"]));

  return "graph"+fname+".png";
}

string Report::_GPSDirection(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  int num_points;
  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set title \"GPS\"\n";


  gp << "set xlabel \"meters\"\n";
  gp << "set ylabel \"meters\"\n";
  gp << "show ylabel\n";

  gp << "plot ";
  if(sensor_data["gps1"]["latitude"].size() > 0)
    gp << "'-' with vectors lc rgb \"blue\" axis x1y1 title 'GPS 1'";
  if(sensor_data["gps2"]["latitude"].size() > 0)
    gp << ", '-' with vectors lc rgb \"red\" axis x1y1 title 'GPS 2'";
  gp << "\n";

  if(sensor_data["gps1"]["latitude"].size() > 0)
  {
    vector<double> lat1, long1, dx1, dy1;
    num_points = sensor_data["gps1"]["latitude"].size()/2;
    DownSample(sensor_data["gps1"]["latitude"], &lat1, num_points);
    DownSample(sensor_data["gps1"]["longitude"],&long1, num_points);
    dx1.reserve(num_points);
    dy1.reserve(num_points);
    dx1.push_back(0.0);
    dy1.push_back(0.0);
    for(int i = 1; i < lat1.size(); i++)
    {
      dx1.push_back((lat1[i] - lat1[i-1])/2.0 );
      dy1.push_back((long1[i] - long1[i-1])/2.0);
    }
    gp.send1d(make_tuple(lat1, long1, dx1, dy1));
  }

  if(sensor_data["gps2"]["latitude"].size() > 0)
  {
    vector<double> lat2, long2, dx2, dy2;
    num_points = sensor_data["gps2"]["latitude"].size()/2;
    DownSample(sensor_data["gps2"]["latitude"], &lat2, num_points);
    DownSample(sensor_data["gps2"]["longitude"],&long2, num_points);

    dx2.reserve(num_points);
    dy2.reserve(num_points);
    dx2.push_back(0.0);
    dy2.push_back(0.0);
    for(int i = 1; i < lat2.size(); i++)
    {
      dx2.push_back(lat2[i] - lat2[i-1]);
      dy2.push_back(long2[i] - long2[i-1]);
    }

    gp.send1d(make_tuple(lat2, long2, dx2, dy2));
  }

  return "graph"+fname+".png";
}

string Report::_BMS_HV(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set multiplot layout 2, 1 title \"BMS HV\" tc rgb \"red\"\n";

  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Volts [V]\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"Current [A]\"\n";
  gp << "set y2tics nomirror\n";
  gp << "show y2label\n";
  gp << "set title \"Volt Current\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'voltage', ";
  gp << "'-' with lines axis x1y2 title 'current'\n";
  
  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["voltage"]));
  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["current"]));

  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Voltage\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"  \"\n";
  gp << "show y2label\n";
  gp << "set title \"Min-Max cell voltages\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'min voltage', ";
  gp << "'-' with lines axis x1y1 title 'max voltage'\n";

  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["min_voltage"]));
  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["max_voltage"]));

  return "graph"+fname+".png";
}

string Report::_VoltCurrentSpeed(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set multiplot layout 2, 1 title \"Throttle Speed Volts Current\" tc rgb \"red\"\n";

  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Throttle %\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"Speed [rad/s]\"\n";
  gp << "set y2tics nomirror\n";
  gp << "show y2label\n";
  gp << "set title \"Throttle Speed\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'Throttle', ";
  gp << "'-' with lines axis x1y2 title 'Speed'\n";
  
  gp.send1d(make_pair(sensor_data["pedals"]["timestamp"], sensor_data["pedals"]["throttle1"]));
  gp.send1d(make_pair(sensor_data["inverter_l"]["timestamp"], sensor_data["inverter_l"]["speed"]));


  // Accel Gyro
  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Voltage\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"Current\"\n";
  gp << "show y2label\n";
  gp << "set title \"Voltage Current\"\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  gp << "plot ";
  gp << "'-' with lines axis x1y1 title 'Voltage', ";
  gp << "'-' with lines axis x1y2 title 'Current'\n";

  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["voltage"]));
  gp.send1d(make_pair(sensor_data["bms_hv"]["timestamp"], sensor_data["bms_hv"]["current"]));

  return "graph"+fname+".png";
}

string Report::_GpsEncoderSpeed(const string& fname)
{
  Gnuplot gp;
  // Gnuplot gp("tee plot.gnu | gnuplot -persist");

  // Plot setup
  gp << "set terminal png size 1920,1080\n";
  gp << "set output 'graph" << fname << ".png'\n";
  gp << "set mxtics 5\n"; // Sets a tick on x axis every 5 units
  gp << "set mytics 5\n"; // Sets a tick on y axis every 5 units
  gp << "set grid\n";     // Enables grid
  gp << "set title \"Gps Odometers speeds\"\n";

  gp << "set xlabel \"Seconds\"\n";
  gp << "set ylabel \"Km/h\"\n";
  gp << "show ylabel\n";
  gp << "set y2label \"  \"\n";
  gp << "show y2label\n";
  gp << "set y2tics nomirror\n";

  gp << "set xrange [0:"<<int(last_timestamp) << "]\n";

  stringstream ss;
  ss << "plot ";
  ss << "'-' with lines axis x1y1 title 'encoder left Km/h', ";
  ss << "'-' with lines axis x1y1 title 'encoder right Km/h'";
  if(sensor_data["gps1"]["kmh"].size() > 0)
    ss << ", '-' with lines axis x1y1 title 'gps1 Km/h'";
  if(sensor_data["gps2"]["kmh"].size() > 0)
    ss << ", '-' with lines axis x1y1 title 'gps2 Km/h'";
  ss << "\n";
  gp << ss.str();

  gp.send1d(make_pair(sensor_data["encoder_l"]["timestamp"], sensor_data["encoder_l"]["kmh"]));
  gp.send1d(make_pair(sensor_data["encoder_r"]["timestamp"], sensor_data["encoder_r"]["kmh"]));
  if(sensor_data["gps1"]["kmh"].size() > 0)
    gp.send1d(make_pair(sensor_data["gps1"]["speed_timestamp"], sensor_data["gps1"]["kmh"]));
  if(sensor_data["gps2"]["kmh"].size() > 0)
    gp.send1d(make_pair(sensor_data["gps2"]["speed_timestamp"], sensor_data["gps2"]["kmh"]));

  return "graph"+fname+".png";
}





void Report::PlaceImage(HPDF_Doc& pdf, HPDF_Page& page, const string& fname)
{
  /* load image file. */
  auto image = HPDF_LoadPngImageFromFile (pdf, fname.c_str());

  double iw = HPDF_Image_GetWidth (image);
  double ih = HPDF_Image_GetHeight (image);
  float ratio = iw / ih;
  iw = HPDF_Page_GetWidth(page);
  ih = iw/ratio;

  double x = (HPDF_Page_GetWidth(page) - iw)/2;
  if(x < 0)
    x = 0;
  double y = (HPDF_Page_GetHeight(page) - ih)/2.0;

  /* Draw image to the canvas. (normal-mode with actual size.)*/
  HPDF_Page_DrawImage (page, image, x, y, iw, ih);
}


void Report::Generate(const string& path, const session_config& stat)
{
  HPDF_Doc  pdf;
  HPDF_Page page;
  string fname = path;
  HPDF_Destination dst;
  HPDF_Image image;

  double x;
  double y;

  double iw;
  double ih;

  pdf = HPDF_New (error_handler, NULL);

  if (!pdf) {
      cout << "error: cannot create PdfDoc object" << endl;
      return;
  }


  /* error-handler */
  if (setjmp(env)) {
      HPDF_Free (pdf);
      return;
  }

  HPDF_SetCompressionMode (pdf, HPDF_COMP_NONE);

  // Loading fonts
  c_fonts["main"].font_name = "Helvetica";
  if(fs::exists("../assets/fonts/Lato-Regular.ttf"))
    c_fonts["main"].font_name = HPDF_LoadTTFontFromFile (pdf, "../assets/fonts/Lato-Regular.ttf", HPDF_TRUE);
  else if(fs::exists("assets/fonts/Lato-Regular.ttf"))
    c_fonts["main"].font_name = HPDF_LoadTTFontFromFile (pdf, "assets/fonts/Lato-Regular.ttf", HPDF_TRUE);
  c_fonts["main"].font = HPDF_GetFont (pdf, c_fonts["main"].font_name.c_str(), NULL);

  c_fonts["main-bold"].font_name = "Helvetica";
  if(fs::exists("../assets/fonts/Lato-Bold.ttf"))
    c_fonts["main-bold"].font_name = HPDF_LoadTTFontFromFile (pdf, "../assets/fonts/Lato-Bold.ttf", HPDF_TRUE);
  else if(fs::exists("assets/fonts/Lato-Bold.ttf"))
    c_fonts["main-bold"].font_name = HPDF_LoadTTFontFromFile (pdf, "assets/fonts/Lato-Bold.ttf", HPDF_TRUE);
  c_fonts["main-bold"].font = HPDF_GetFont (pdf, c_fonts["main-bold"].font_name.c_str(), NULL);


  // First Page
  {
    page = HPDF_AddPage (pdf);

    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);

    dst = HPDF_Page_CreateDestination (page);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
    HPDF_SetOpenAction(pdf, dst);

    string png = "";
    if(fs::exists("../assets/images/LogoAndText_1.png"))
      png = "../assets/images/LogoAndText_1.png";
    else if(fs::exists("assets/images/LogoAndText_1.png"))
      png = "assets/images/LogoAndText_1.png";

    if(png != "")
    {
      image = HPDF_LoadPngImageFromFile (pdf, png.c_str());

      iw = HPDF_Image_GetWidth (image);
      ih = HPDF_Image_GetHeight (image);
      float ratio = iw / ih;
      iw = HPDF_Page_GetWidth(page)/2.0;
      ih = iw/ratio;

      x = (HPDF_Page_GetWidth(page) - iw)/2;
      if(x < 0)
        x = 0;
      y = HPDF_Page_GetHeight(page) - ih - ih/2.0;

      HPDF_Page_DrawImage (page, image, x, y, iw, ih);
    }

    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 42);
    HPDF_Page_SetRGBFill (page, 0.1,0.1,0.15);
    
    string title = "Telemetry Report";
    auto t_width = HPDF_Font_TextWidth(c_fonts["main"].font, (HPDF_BYTE*)title.c_str(), title.size());
    // 42 is the font size
    t_width.width *= 42.0 / 1000.0;
    x = (HPDF_Page_GetWidth(page) - t_width.width)/2.0;
    y -= 64;
    HPDF_Page_MoveTextPos (page, x, y);

    HPDF_Page_ShowText (page, title.c_str());
    HPDF_Page_EndText (page);

    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 24);
    HPDF_Page_SetRGBFill (page, 0.1,0.1,0.15);
    
    string text = "Plots and basic information from race session.";
    t_width = HPDF_Font_TextWidth(c_fonts["main"].font, (HPDF_BYTE*)text.c_str(), text.size());
    // 24 is the font size
    t_width.width *= 24.0 / 1000.0;
    x = (HPDF_Page_GetWidth(page) - t_width.width)/2.0;
    y -= 64;
    HPDF_Page_MoveTextPos (page, x, y);
    HPDF_Page_ShowText (page, text.c_str());
    HPDF_Page_EndText (page);


    y -= 96;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 18);
    HPDF_Page_TextOut(page, 120, y, "Date: ");
    HPDF_Page_TextOut(page, 360, y, stat.Date.c_str());
    HPDF_Page_EndText(page);
    y -= 32;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 18);
    HPDF_Page_TextOut(page, 120, y, "Circuit: ");
    HPDF_Page_TextOut(page, 360, y, stat.Circuit.c_str());
    HPDF_Page_EndText(page);
    y -= 32;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 18);
    HPDF_Page_TextOut(page, 120, y, "Pilot: ");
    HPDF_Page_TextOut(page, 360, y, stat.Pilot.c_str());
    HPDF_Page_EndText(page);
    y -= 32;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 18);
    HPDF_Page_TextOut(page, 120, y, "Race: ");
    HPDF_Page_TextOut(page, 360, y, stat.Race.c_str());
    HPDF_Page_EndText(page);
    y -= 32;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 18);
    HPDF_Page_TextOut(page, 120, y, "Configuration: ");
    HPDF_Page_TextOut(page, 360, y, stat.Configuration.c_str());
    HPDF_Page_EndText(page);

    {
      stringstream ss;
      HPDF_Page page = HPDF_AddPage (pdf);
      HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);
      y = HPDF_Page_GetHeight(page);
      y -= 120;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 24);
      HPDF_Page_TextOut(page, 120, y, "Race stats");
      HPDF_Page_EndText(page);
      y -= 64;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Distance travelled [m]: ");
      HPDF_Page_TextOut(page, 360, y, to_string(int(distance_travelled.val1 - distance_travelled.val0)).c_str());
      HPDF_Page_EndText(page);
      y -= 32;
      ss << std::fixed << setprecision(3) << (max_speed * 0.395/2.0)*3.6;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Max Speed [km/h]: ");
      HPDF_Page_TextOut(page, 360, y, ss.str().c_str());
      HPDF_Page_EndText(page);
      y -= 32;
      ss.str("");
      ss << max_g;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Max acceleration (both x and y) [g]: ");
      HPDF_Page_TextOut(page, 360, y, ss.str().c_str());
      HPDF_Page_EndText(page);
      y -= 32;
      ss.str("");
      ss << max_brake_pressure;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Max brake pressure [bar]: ");
      HPDF_Page_TextOut(page, 360, y, ss.str().c_str());
      HPDF_Page_EndText(page);
      y -= 32;
      ss.str("");
      ss << total_voltage_drop.val0 - total_voltage_drop.val1;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Voltage drop of total pack [V]: ");
      HPDF_Page_TextOut(page, 360, y, ss.str().c_str());
      HPDF_Page_EndText(page);
      y -= 32;
      ss.str("");
      ss << min_voltage_drop.val0 - min_voltage_drop.val1;
      HPDF_Page_BeginText(page);
      HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 15);
      HPDF_Page_TextOut(page, 120, y, "Voltage drop of min cell [V]: ");
      HPDF_Page_TextOut(page, 360, y, ss.str().c_str());
      HPDF_Page_EndText(page);
    }
  }

  HPDF_Page_SetFontAndSize (page, c_fonts["main"].font, 24);

  vector<string> image_names;

  image_names.push_back(_Odometers       (to_string(id)+"Odometers"       ));
  image_names.push_back(_GpsEncoderSpeed (to_string(id)+"GpsEncoderSpeed" ));
  image_names.push_back(_Pedals          (to_string(id)+"Pedals"          ));
  image_names.push_back(_IMU             (to_string(id)+"IMU"             ));
  image_names.push_back(_SteerAccelGyro  (to_string(id)+"SteerAccelGyro"  ));
  image_names.push_back(_PedalsSpeedAccel(to_string(id)+"PedalsSpeedAccel"));
  image_names.push_back(_BMS_HV          (to_string(id)+"BMS_HV"          ));
  image_names.push_back(_VoltCurrentSpeed(to_string(id)+"VoltCurrentSpeed"));
  image_names.push_back(_GPS             (to_string(id)+"GPS"             ));
  // image_names.push_back(_GPSDirection    (to_string(id)+"GPSDirection"    ));


  for(auto fname : image_names)
  {
    HPDF_Page page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);
    dst = HPDF_Page_CreateDestination (page);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
    PlaceImage(pdf, page, fname);
    fs::remove(fname);
  }




  /* save the document to a file */
  HPDF_SaveToFile (pdf, fname.c_str());
  HPDF_SaveToFile (pdf, "Last Report.pdf");

  /* clean up */
  HPDF_Free (pdf);
}

void Report::DownSample(const vector<double>& in, vector<double>* out, int count)
{
  if(in.size() > count)
  {
    float factor = float(in.size()) / float(count);
    if(factor >= 2)
    {
      out->clear();
      out->reserve(count);
      for(int k = 0; k < count; k++)
      {
        out->push_back(in[k*factor]);
      }
    }
    else
    {
      *out = in;
      int diff = in.size() - count;
      for(int k = 0; k < diff; k++)
      {
        out->erase(out->begin() + (out->size()/diff)*k);
      }
    }
  }
  else
    *out = in;
}

void Report::lla2xyz(const double& lat,
                      const double& lng,
                      const double& alt,
                      const double& lat0,
                      const double& lng0,
                      double& x,
                      double& y,
                      double& z)
{
  double f = 0.00335281066474748071;
  double earth_radius = 6378137.0;

  double Lat_p = lat * M_PI / 180.0;
  double Lon_p = lng * M_PI / 180.0;
  double Alt_p = alt;

  // Reference location (lat, lon), from degrees to radians
  double Lat_o = lat_0 * M_PI / 180.0;
  double Lon_o = long_0 * M_PI / 180.0;
  
  double psio = psio * M_PI / 180.0;  // from degrees to radians

  double dLat = Lat_p - Lat_o;
  double dLon = Lon_p - Lon_o;

  double ff = (2.0 * f) - (f * f);  // Can be precomputed

  double sinLat = sin(Lat_o);

  // Radius of curvature in the prime vertical
  double Rn = earth_radius / sqrt(1 - (ff * (sinLat * sinLat)));

  // Radius of curvature in the meridian
  double Rm = Rn * ((1 - ff) / (1 - (ff * (sinLat * sinLat))));

  double dNorth = (dLat) / atan2(1, Rm);
  double dEast = (dLon) / atan2(1, (Rn * cos(Lat_o)));

  // Rotate matrice clockwise
  x = (dNorth * cos(psio)) + (dEast * sin(psio));
  y = (-dNorth * sin(psio)) + (dEast * cos(psio));
  double href = 0.0;
  z = -Alt_p - href;
}