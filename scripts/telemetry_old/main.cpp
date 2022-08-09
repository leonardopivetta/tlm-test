#define __TELEMETRY_CONFIG_IMPLEMENTATION__
#define __MESSAGES_IMPLEMENTATION__
#include "telemetry_sm.h"

int main(int argc, char **argv)
{
  TelemetrySM telemetry;
  int MAX_RETRIES = 10;
  for (int retry = 0; retry < MAX_RETRIES; retry++)
  {
    try
    {
      telemetry.Init();
    }
    catch (exception e)
    {
      CONSOLE.LogError("Fatal exception", e.what());
    }
    if (telemetry.GetError() != TelemetryError::TEL_NONE)
    {
      CONSOLE.LogError("Telemetry error", TelemetryErrorStr[telemetry.GetError()]);
      telemetry.Reset();
    }

    CONSOLE.LogWarn("Retry number:", retry);
    usleep(1000000);
  }

  CONSOLE.Log("Telemetry quitted");

  return 0;
}