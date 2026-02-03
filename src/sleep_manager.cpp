#include "sleep_manager.h"
#include "config.h"
#include <M5Unified.h>
#include <time.h>

SleepManager::SleepManager() {
}

bool SleepManager::isTimeSynced() {
    time_t now;
    time(&now);
    // If time is before year 2020, it's not synced
    return (now > 1577836800);  // Jan 1, 2020
}

int SleepManager::getNextUpdateHour(int currentHour) {
    // Find the next scheduled update time
    for (int i = 0; i < NUM_UPDATE_TIMES; i++) {
        if (UPDATE_TIMES[i] > currentHour) {
            return UPDATE_TIMES[i];
        }
    }
    // Wrap to first update time of next day
    return UPDATE_TIMES[0] + 24;
}

int32_t SleepManager::getSecondsUntilNextUpdate() {
    if (!isTimeSynced()) {
        Serial.println("Time not synced, cannot calculate sleep duration");
        return -1;
    }

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    int currentHour = timeinfo->tm_hour;
    int currentMinute = timeinfo->tm_min;
    int currentSecond = timeinfo->tm_sec;

    // Get next update hour
    int nextHour = getNextUpdateHour(currentHour);

    // Calculate hours until next update
    int hoursUntil = nextHour - currentHour;
    if (hoursUntil <= 0) {
        hoursUntil += 24;
    }

    // Calculate total seconds
    // Next update is at nextHour:00:00
    // Current time is currentHour:currentMinute:currentSecond
    int32_t secondsUntil = (hoursUntil * 3600) - (currentMinute * 60) - currentSecond;

    // Add a small buffer (30 seconds) to ensure we wake up slightly after the target time
    secondsUntil += 30;

    Serial.printf("Current time: %02d:%02d:%02d\n", currentHour, currentMinute, currentSecond);
    Serial.printf("Next update at: %02d:00:00 (in %d hours)\n", nextHour % 24, hoursUntil);
    Serial.printf("Sleep duration: %d seconds (%.1f hours)\n", secondsUntil, secondsUntil / 3600.0);

    return secondsUntil;
}

void SleepManager::enterDeepSleep(int32_t seconds) {
    if (seconds <= 0) {
        Serial.println("Invalid sleep duration, using error retry interval");
        seconds = ERROR_RETRY_SECONDS;
    }

    Serial.printf("Entering deep sleep for %d seconds...\n", seconds);
    Serial.flush();

    // Use M5Stack's timer sleep (RTC-based)
    M5.Power.timerSleep(seconds);

    // If timerSleep returns (shouldn't normally), use ESP deep sleep as fallback
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    esp_deep_sleep_start();
}

void SleepManager::sleepUntilNextUpdate() {
    int32_t seconds = getSecondsUntilNextUpdate();

    if (seconds < 0) {
        // Time not synced, use error retry
        Serial.println("Cannot calculate next update, using retry interval");
        seconds = ERROR_RETRY_SECONDS;
    }

    enterDeepSleep(seconds);
}

void SleepManager::sleepForRetry() {
    Serial.printf("Sleeping for retry in %d seconds...\n", ERROR_RETRY_SECONDS);
    enterDeepSleep(ERROR_RETRY_SECONDS);
}
