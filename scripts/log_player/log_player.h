#pragma once

#include <stdio.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <signal.h>

#include <cstdio>

#include <filesystem>

#include <mutex>
#include <thread>

#include "can.h"
#include "utils.h"
#include "browse.h"

#define __TELEMETRY_JSON_IMPLEMENTATION__
#include "json-loader/telemetry_json.h"

using namespace std;
using namespace std::chrono;
using namespace filesystem;

string serialized_string;

struct CAN_Socket_t {
    Can *sock;
    string name;
    thread *thrd;
    sockaddr_can addr;
};

std::mutex mtx;
std::condition_variable cv;

string shared_string;

void writer(string fname);