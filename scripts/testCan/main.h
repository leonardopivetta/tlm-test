#pragma once

#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <filesystem>

#define CANLIB_TIMESTAMP
#define primary_IDS_IMPLEMENTATION
#define secondary_IDS_IMPLEMENTATION
#define primary_NETWORK_IMPLEMENTATION
#define secondary_NETWORK_IMPLEMENTATION
#include "thirdparty/can/lib/primary/c/ids.h"
#include "thirdparty/can/lib/secondary/c/ids.h"
#include "thirdparty/can/lib/primary/c/network.h"
#include "thirdparty/can/lib/secondary/c/network.h"

#define primary_MAPPING_IMPLEMENTATION
#define secondary_MAPPING_IMPLEMENTATION
#include "thirdparty/can/proto/primary/cpp/mapping.h"
#include "thirdparty/can/proto/secondary/cpp/mapping.h"

#include "can.h"