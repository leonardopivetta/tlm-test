#pragma once

using namespace std;

#include <iostream>
#include <string>

#define CANLIB_TIMESTAMP
#undef primary_IDS_IMPLEMENTATION
#undef secondary_IDS_IMPLEMENTATION
#undef primary_NETWORK_IMPLEMENTATION
#undef secondary_NETWORK_IMPLEMENTATION
#undef primary_MAPPING_IMPLEMENTATION
#undef secondary_MAPPING_IMPLEMENTATION

#define __APP_JSON_IMPLEMENTATION__
#define __MESSAGES_JSON_IMPLEMENTATION__
#define __TELEMETRY_JSON_IMPLEMENTATION__

#include "state_machines/fsm_telemetry.h"

#include <unistd.h>
#include <syslog.h>