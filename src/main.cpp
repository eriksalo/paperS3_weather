#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "weather_api.h"
#include "display_manager.h"
#include "sleep_manager.h"

// DEBUG MODE - set to false for production
#define DEBUG_MODE false
#define DEBUG_DELAY_MS 5000

// Global objects
WeatherAPI weatherAPI;
DisplayManager display;
SleepManager sleepMgr;

// Function prototypes
bool connectWiFi();
void disconnectWiFi();
bool syncTime();

void setup() {
    // Initialize M5Stack
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);

    // Brief delay for initialization
    delay(500);

    Serial.println("\n========================================");
    Serial.println("M5Stack Paper S3 Weather Display");
    Serial.println("========================================\n");

    // Initialize display manager
    display.begin();
    display.renderStatus("Starting...");

    // Step 1: Connect to WiFi
    Serial.println("Step 1: Connecting to WiFi...");
    display.renderStatus("Connecting WiFi...");

    if (!connectWiFi()) {
        Serial.println("WiFi connection failed!");
        display.renderError("WiFi failed");
        if (!DEBUG_MODE) {
            sleepMgr.sleepForRetry();
        }
        return;
    }

    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Step 2: Sync time via NTP
    Serial.println("\nStep 2: Syncing time via NTP...");
    display.renderStatus("Syncing time...");

    if (!syncTime()) {
        Serial.println("Time sync failed!");
        display.renderError("Time sync failed");
        disconnectWiFi();
        if (!DEBUG_MODE) {
            sleepMgr.sleepForRetry();
        }
        return;
    }

    // Print current time
    time_t now;
    time(&now);
    Serial.print("Current time: ");
    Serial.println(ctime(&now));

    // Step 3: Fetch weather data
    Serial.println("Step 3: Fetching weather data...");
    display.renderStatus("Fetching weather...");

    bool weatherSuccess = weatherAPI.fetchWeather(
        LOCATION_LAT,
        LOCATION_LON,
        OWM_API_KEY,
        WEATHER_UNITS
    );

    // Step 4: Disconnect WiFi to save power
    Serial.println("\nStep 4: Disconnecting WiFi...");
    disconnectWiFi();

    // Step 5: Render weather or error
    if (weatherSuccess) {
        Serial.println("\nStep 5: Rendering weather display...");
        WeatherData& data = weatherAPI.getData();

        Serial.printf("Current: %.1f°F, %s\n",
                      data.current.temp,
                      data.current.description.c_str());
        Serial.printf("Hourly forecasts: %d\n", data.hourlyCount);
        Serial.printf("Daily forecasts: %d\n", data.dailyCount);

        display.renderWeather(data);
    } else {
        Serial.println("\nWeather fetch failed!");
        Serial.println("Error: " + weatherAPI.getError());
        display.renderError(weatherAPI.getError());
        if (!DEBUG_MODE) {
            sleepMgr.sleepForRetry();
        }
        return;
    }

    // Step 6: Enter deep sleep until next update
    Serial.println("\nStep 6: Calculating sleep duration...");

    if (DEBUG_MODE) {
        Serial.println("DEBUG: Skipping deep sleep - staying awake");
        Serial.println("DEBUG: Will refresh every 60 seconds");
    } else {
        sleepMgr.sleepUntilNextUpdate();
    }
}

void loop() {
    if (DEBUG_MODE) {
        // In debug mode, just keep running and update M5
        M5.update();
        delay(1000);

        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 60000) {
            Serial.println("DEBUG: Still running...");
            lastUpdate = millis();
        }
    } else {
        delay(1000);
    }
}

bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_TIMEOUT_MS) {
            Serial.println("\nConnection timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println(" Connected!");
    return true;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disconnected");
}

bool syncTime() {
    // Multiple NTP servers for reliability
    const char* ntpServers[] = {
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com"
    };
    const int numServers = 3;

    for (int server = 0; server < numServers; server++) {
        Serial.printf("Trying NTP server: %s\n", ntpServers[server]);

        // Configure NTP with current server
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServers[server]);

        Serial.print("Waiting for NTP time sync");

        // Wait for time to be set (max 15 seconds per server)
        int retry = 0;
        const int maxRetries = 30;

        while (!sleepMgr.isTimeSynced() && retry < maxRetries) {
            delay(500);
            Serial.print(".");
            retry++;
        }

        Serial.println();

        if (sleepMgr.isTimeSynced()) {
            Serial.println("Time synchronized!");
            return true;
        }

        Serial.println("Server timeout, trying next...");
    }

    Serial.println("All NTP servers failed!");
    return false;
}
