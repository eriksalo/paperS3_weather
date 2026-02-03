#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>

class SleepManager {
public:
    SleepManager();

    // Calculate seconds until next scheduled update
    // Returns seconds to sleep, or -1 if time not synced
    int32_t getSecondsUntilNextUpdate();

    // Enter deep sleep for specified seconds
    void enterDeepSleep(int32_t seconds);

    // Enter deep sleep until next scheduled update
    void sleepUntilNextUpdate();

    // Enter short error retry sleep (5 minutes)
    void sleepForRetry();

    // Check if current time is synced via NTP
    bool isTimeSynced();

private:
    // Find next update time from schedule
    int getNextUpdateHour(int currentHour);
};

#endif // SLEEP_MANAGER_H
