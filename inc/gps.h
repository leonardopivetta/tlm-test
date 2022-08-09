#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "utils.h"
#include "devices.pb.h"

using namespace std;

static std::string FIX_STATE[7] =
    {
        "FIX NOT AVAILABLE OR INVALID",
        "GPS SPS MODE FIX VALID",
        "DIFFERENTIAL GPS SPS MODE, FIX VALID",
        "RTK Fixed", "RTK Float",
        "DEAD RECKONING MODE FIX VALID"};
static std::string FIX_MODE[3] =
    {
        "FIX NOT AVAILABLE",
        "2D",
        "3D"};

enum ParseError
{
  ParseOk,
  NoMatch,
  MessageEmpty,
  MessageLength,
  MessageUndefined,
  FieldError
};
static std::string ParseErrorString[] =
    {
        "ParseOk",
        "NoMatch",
        "MessageEmpty",
        "MessageLength",
        "MessageUndefined",
        "FieldError"};

struct GpsData
{
  uint64_t timestamp;
  std::string msg_type;
  std::string time;
  double latitude;
  double longitude;
  double altitude;
  uint32_t fix;
  uint32_t satellites;
  std::string fix_state;
  double age_of_correction;
  double course_over_ground_degrees;
  double course_over_ground_degrees_magnetic;
  double speed_kmh;
  std::string mode;
  double position_diluition_precision;
  double horizontal_diluition_precision;
  double vertical_diluition_precision;

  bool heading_valid;
  double heading_vehicle;
  double heading_motion;
  double heading_accuracy_estimate;
  double speed_accuracy;
};
class Gps
{
public:
  Gps(std::string name) : name(name)
  {
    id = id_;
    id_++;
  };

  int get_id() { return id; };
  std::string get_header(std::string separator);
  std::string get_string(std::string separator);
  std::string get_readable();

  void serialize(devices::Gps *device);

  void clear()
  {
    data.timestamp = 0.0;
    data.msg_type = "";
    data.time = "";
    data.latitude = 0.0;
    data.longitude = 0.0;
    data.altitude = 0.0;
    data.fix = 0;
    data.satellites = 0;
    data.fix_state = "";
    data.age_of_correction = 0.0;
    data.course_over_ground_degrees = 0.0;
    data.course_over_ground_degrees_magnetic = 0.0;
    data.speed_kmh = 0.0;
    data.mode = "";
    data.position_diluition_precision = 0.0;
    data.horizontal_diluition_precision = 0.0;
    data.vertical_diluition_precision = 0.0;

    data.heading_valid = false;
    data.heading_vehicle = 0.0;
    data.heading_motion = 0.0;
    data.heading_accuracy_estimate = 0.0;
    data.speed_accuracy = 0.0;
  }

  std::string name;
  std::fstream *file;
  std::string filename;

  GpsData data;

private:
  static int id_;
  int id;
};

// Page 145 of interface manual ublox
// header: 0xb5 0x62
// class : 0x01
// id    : 0x07
// bytes : 92
struct __attribute__((__packed__)) UBX_MSG_PVT
{
  uint32_t iTOW;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t valid;
  uint32_t tAcc;
  int32_t nano;
  uint8_t fixType;
  uint8_t flags;
  uint8_t flags2;
  uint8_t numSV;
  int32_t lon;
  int32_t lat;
  int32_t height;
  int32_t hMSL;
  uint32_t hAcc;
  uint32_t vAcc;
  int32_t velN;
  int32_t velE;
  int32_t velD;
  int32_t gSpeed;
  int32_t headMot;
  uint32_t sAcc;
  uint32_t headAcc;
  uint16_t pDOP;
  uint16_t flags3;
  uint32_t reserved;
  int32_t headVeh;
  int16_t magDec;
  uint16_t magAcc;
};

struct __attribute__((__packed__)) UBX_MSG_MATCH
{
  uint8_t msgClass;
  uint8_t msgID;
  uint16_t length;
};

//////

bool parse_ubx_line(const string &line, UBX_MSG_PVT &msg);

uint16_t reverse16(const uint16_t &in);
uint32_t reverse32(const uint32_t &in);

int16_t reversei16(const int16_t &in);
int32_t reversei32(const int32_t &in);

ParseError parse_gps(Gps *gps_, const uint64_t &timestamp, string &line);