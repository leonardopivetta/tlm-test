#pragma once

#include <setjmp.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>

#include <filesystem>

#include "libharu/include/hpdf.h"

#include "telemetry_config.h"

#include "gnuplot-iostream.h"

using namespace std;
using namespace gnuplotio;
namespace fs = std::filesystem;

void error_handler(HPDF_STATUS error_no,
                   HPDF_STATUS detail_no,
                   void *user_data);
void show_description(HPDF_Page page,
                      float x,
                      float y,
                      const char *text);

template <class T1>
vector<T1> upsample(const vector<T1> &x, const int p)
{
  long int N = x.size();
  vector<T1> y;
  y.reserve(p * N);
  for (long int n = 0; n < N; n++)
    y[p * n] = x[n];
  return y;
}

template <class T1>
vector<T1> downsample(const vector<T1> &x, const int q)
{
  vector<T1> y;
  int N = int(floor(1.0 * x.size() / q));
  y.reserve(N);
  for (long int n = 0; n < N; n++)
    y[n] = x[n * q];
  return y;
}

struct MapElement
{
  string primary;
  string secondary;
};

struct PlotElement
{
  MapElement sensor;
  int y_id;
  string y_scale;
};

struct CurrentFont
{
  string font_name;
  HPDF_Font font;
  int size;
};

struct Delta_ST
{
  double val0 = 0.0;
  double val1 = 0.0;
  bool setted = false;
};

struct Point_ST
{
  double x;
  double y;
};

class Report
{
public:
  Report()
  {
    id = instances;
    instances++;
  };

  void AddDeviceSample(Chimera *chim, Device *device);
  void Generate(const string &path, const session_config &stat);
  void Clean(int);
  void Filter(const vector<double> &in, vector<double> *out, int window = 10);
  void DownSample(const vector<double> &in, vector<double> *out, int count);

private:
  unordered_map<string, unordered_map<string, vector<double>>> sensor_data;

  unordered_map<string, CurrentFont> c_fonts;

  double first_timestamp = -1.0;
  double last_timestamp = 0.0;

  double max_speed = 0.0;
  double max_g = 0.0;
  double max_brake_pressure = 0.0;
  Delta_ST min_voltage_drop;
  Delta_ST total_voltage_drop;
  Delta_ST distance_travelled;

  double lat_0 = -1.0;
  double long_0 = -1.0;
  double last_alt = -1.0;

  static int instances;
  int id;

private:
  void lla2xyz(const double &lat, const double &lng, const double &alt, const double &lat0, const double &lng0, double &, double &, double &);

  string _Odometers(const string &fname);
  string _Pedals(const string &fname);
  string _IMU(const string &fname);
  string _SteerAccelGyro(const string &fname);
  string _PedalsSpeedAccel(const string &fname);
  string _BMS_HV(const string &fname);
  string _VoltCurrentSpeed(const string &fname);
  string _GpsEncoderSpeed(const string &fname);
  string _GPS(const string &fname);
  string _GPSDirection(const string &fname);

  bool CheckSize(const vector<MapElement> &sensors, size_t &minsize);
  void PlaceImage(HPDF_Doc &pdf, HPDF_Page &page, const string &fname);
};