#include "inverter.h"

#include <math.h>

int16_t max_rpm = 7000.0f;
float max_current = 20.0f;
float max_motor_temp = 100.0f;
float max_inverter_temp = 100.0f;

inverter_data_t inverters_data[2];

void inverter_open_files(std::string folder, inverter_files_t* inverters_files){
  inverters_files[0].inverter_temp = fopen((folder + inverter_filename(0)).c_str(), "w");
  inverters_files[0].inverter_motor_temp = fopen((folder + inverter_filename(1)).c_str(), "w");
  inverters_files[0].inverter_speed = fopen((folder + inverter_filename(2)).c_str(), "w");
  inverters_files[1].inverter_temp = fopen((folder + inverter_filename(3)).c_str(), "w");
  inverters_files[1].inverter_motor_temp = fopen((folder + inverter_filename(4)).c_str(), "w");
  inverters_files[1].inverter_speed = fopen((folder + inverter_filename(5)).c_str(), "w");

  inverter_fields(inverters_files[0].inverter_temp, 0);
  inverter_fields(inverters_files[0].inverter_motor_temp, 1);
  inverter_fields(inverters_files[0].inverter_speed, 2);
  inverter_fields(inverters_files[1].inverter_temp, 3);
  inverter_fields(inverters_files[1].inverter_motor_temp, 4);
  inverter_fields(inverters_files[1].inverter_speed, 5);
  fflush(inverters_files[0].inverter_temp);
  fflush(inverters_files[0].inverter_motor_temp);
  fflush(inverters_files[0].inverter_speed);
  fflush(inverters_files[1].inverter_temp);
  fflush(inverters_files[1].inverter_motor_temp);
  fflush(inverters_files[1].inverter_speed);
}

void inverter_close_files(inverter_files_t* inverters_files){
  fflush(inverters_files[0].inverter_temp);
  fclose(inverters_files[0].inverter_temp);
  fflush(inverters_files[0].inverter_motor_temp);
  fclose(inverters_files[0].inverter_motor_temp);
  fflush(inverters_files[0].inverter_speed);
  fclose(inverters_files[0].inverter_speed);
  fflush(inverters_files[1].inverter_temp);
  fclose(inverters_files[1].inverter_temp);
  fflush(inverters_files[1].inverter_motor_temp);
  fclose(inverters_files[1].inverter_motor_temp);
  fflush(inverters_files[1].inverter_speed);
  fclose(inverters_files[1].inverter_speed);
}

std::string inverter_filename(int j){
  switch (j) {
    case 0:
      return "INVERTER_L_TEMP.csv";
    case 1:
      return "INVERTER_L_MOTOR_TEMP.csv";
    case 2:
      return "INVERTER_L_SPEED.csv";
    case 3:
      return "INVERTER_R_TEMP.csv";
    case 4:
      return "INVERTER_R_MOTOR_TEMP.csv";
    case 5:
      return "INVERTER_R_SPEED.csv";
  }
  return "";
}

int inverter_fields(FILE *buffer, int j) {
  switch (j) {
  case 0: case 3:
    fprintf(buffer, "_timestamp,inverter_temp\n");
    break;
  case 1: case 4:
    fprintf(buffer, "_timestamp,motor_temp\n");
    break;
  case 2: case 5:
    fprintf(buffer, "_timestamp,speed\n");
    break;
  }
  return 0;
}

float inline convert_motor_temp(primary_message_INV_L_RESPONSE *data) {
  int16_t m_temp = ((data->data_1 << 8) | data->data_0);
  float motor_temp = (float)((m_temp - 9393.9) / 55.1f);
  return motor_temp;
}

float inline convert_inverter_temp(primary_message_INV_L_RESPONSE *data) {
  uint16_t i_temp = ((data->data_1 << 8) | data->data_0);
  float igbt_temp = (float)((i_temp - 15797) / 112.1f);
  return igbt_temp;
}

int16_t inline convert_speed(primary_message_INV_L_RESPONSE *data) {
  int16_t raw_val = ((data->data_1 << 8) | data->data_0);
  int16_t rpm = max_rpm * raw_val / 32767;
  return rpm;
}

float inline rpm_to_rads(int16_t rpm) {
  return rpm * 2 * M_PI / 60;
}

bool inverter_to_string(primary_message_INV_L_RESPONSE *data, int id, inverter_files_t* inverters_files) {
  inverter_data_t& inv = inverters_data[CHECK_INVERTER_SIDE(id)];
  inverter_files_t& file = inverters_files[CHECK_INVERTER_SIDE(id)];
  switch (data->reg_id) {
  case 0x49: // motor temp
    if(file.inverter_motor_temp == NULL) return false;
    fprintf(file.inverter_motor_temp, "%" PRIu64 ",%f", inv.inverter_motor_temp._timestamp,
                  inv.inverter_motor_temp.motor_temp);
    fprintf(file.inverter_motor_temp, "\n");
    fflush(file.inverter_motor_temp);
    break;
  case 0x4A: // inverter temp
    if(file.inverter_temp == NULL) return false;
    fprintf(file.inverter_temp, "%" PRIu64 ",%f", inv.inverter_temp._timestamp,
                  inv.inverter_temp.temp);
    fprintf(file.inverter_temp, "\n");
    fflush(file.inverter_temp);
  case 0xA8: // speed
    if(file.inverter_speed == NULL) return false;
    fprintf(file.inverter_speed, "%" PRIu64 ",%f", inv.inverter_speed._timestamp,
                  inv.inverter_speed.speed);
    fprintf(file.inverter_speed, "\n");
    fflush(file.inverter_speed);
  default:
    return false;
    break;
  }
  return true;
}

bool parse_inverter(primary_message_INV_L_RESPONSE *data, int id){
  inverter_data_t& inv = inverters_data[CHECK_INVERTER_SIDE(id)];
  switch (data->reg_id) {
  case 0x49: // motor temp
    inv.inverter_motor_temp._timestamp = data->_timestamp;
    inv.inverter_motor_temp.motor_temp = convert_motor_temp(data);
    break;
  case 0x4A: // inverter temp
    inv.inverter_temp._timestamp = data->_timestamp;
    inv.inverter_temp.temp = convert_inverter_temp(data);
  case 0xA8: // speed
    inv.inverter_speed._timestamp = data->_timestamp;
    inv.inverter_speed.speed = rpm_to_rads(convert_speed(data));
  default:
    return false;
    break;
  }
  return true;
}

// --- PROTO IMPLEMENTATION ---

void inverter_serialize(void* data, int id, devices::InverterVec* inverter){
  primary_message_INV_L_RESPONSE* inv_data = (primary_message_INV_L_RESPONSE*)data;
  inverter_data_t& inv = inverters_data[CHECK_INVERTER_SIDE(id)];
  switch (inv_data->reg_id) {
  case 0x49:{ // motor temp
    auto new_el = inverter->add_inverter_motor_temp();
    new_el->set_timestamp(inv.inverter_motor_temp._timestamp);
    new_el->set_motor_temp(inv.inverter_motor_temp.motor_temp);
    break;
  }
  case 0x4A:{ // inverter temp
    auto new_el = inverter->add_inverter_temp();
    new_el->set_timestamp(inv.inverter_temp._timestamp);
    new_el->set_temperature(inv.inverter_temp.temp);
    break;
  }
  case 0xA8:{ // speed
    auto new_el = inverter->add_inverter_speed();
    new_el->set_timestamp(inv.inverter_speed._timestamp);
    new_el->set_speed(inv.inverter_speed.speed);
    break;
  }
  }
}

void inverter_deserialize(inverter_proto_t* data, devices::InverterVec* inverter){
  for(int i = 0; i < inverter->inverter_motor_temp_size(); i++){
    static inverter_motor_temp_t instance;
    instance._timestamp = inverter->inverter_motor_temp(i).timestamp();
    instance.motor_temp = inverter->inverter_motor_temp(i).motor_temp();
    data->motor_temp.push(instance);
  }
  for(int i = 0; i < inverter->inverter_temp_size(); i++){
    static inverter_temp_t instance;
    instance._timestamp = inverter->inverter_temp(i).timestamp();
    instance.temp = inverter->inverter_temp(i).temperature();
    data->temp.push(instance);
  }
  for(int i = 0; i < inverter->inverter_speed_size(); i++){
    static inverter_speed_t instance;
    instance._timestamp = inverter->inverter_speed(i).timestamp();
    instance.speed = inverter->inverter_speed(i).speed();
    data->speed.push(instance);
  }
}