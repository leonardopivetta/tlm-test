#ifndef INVERTER_H
#define INVERTER_H

#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <string.h>

#define CANLIB_TIMESTAMP
#include "can/lib/primary/c/network.h"

#define INVERTER_MSGS 6
#define CHECK_INVERTER_SIDE(id)id==385?0:1

struct inverter_temp_t{
    uint64_t _timestamp;
    float temp;
};
struct inverter_motor_temp_t{
    uint64_t _timestamp;
    float motor_temp;
};
struct inverter_speed_t{
    uint64_t _timestamp;
    float speed;
};
struct inverter_data_t{
    inverter_temp_t inverter_temp;
    inverter_motor_temp_t inverter_motor_temp;
    inverter_speed_t inverter_speed;
};
struct inverter_files_t{
    FILE* inverter_temp;
    FILE* inverter_motor_temp;
    FILE* inverter_speed;
};

void inverter_close_files(inverter_files_t* inverters_files);
void inverter_open_files(std::string folder, inverter_files_t* inverters_files);

std::string inverter_filename(int j);
int inverter_fields(FILE *buffer, int j);

bool inverter_to_string(primary_message_INV_L_RESPONSE *data, int id, inverter_files_t files[2]);
bool parse_inverter(primary_message_INV_L_RESPONSE *data, int id);



// --- PROTO IMPLEMENTATION ---
#include "devices.pb.h"
#include "can/proto/primary/cpp/mapping.h"

struct inverter_proto_t{
    canlib_circular_buffer<inverter_motor_temp_t, CANLIB_CIRCULAR_BUFFER_SIZE> motor_temp;
    canlib_circular_buffer<inverter_temp_t, CANLIB_CIRCULAR_BUFFER_SIZE> temp;
    canlib_circular_buffer<inverter_speed_t, CANLIB_CIRCULAR_BUFFER_SIZE> speed;
};

void inverter_serialize(void* data, int id, devices::InverterVec* inverter);
void inverter_deserialize(inverter_proto_t* data, devices::InverterVec* inverter);


#endif