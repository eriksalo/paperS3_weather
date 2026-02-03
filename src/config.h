#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "SALO"
#define WIFI_PASSWORD "eriklori"
#define WIFI_TIMEOUT_MS 30000

// OpenWeatherMap API Configuration
#define OWM_API_KEY "43c5433daa47204b13788c8190bf45da"
#define OWM_API_HOST "api.openweathermap.org"

// Location: Longmont, Colorado
#define LOCATION_LAT 40.1672
#define LOCATION_LON -105.1019
#define LOCATION_NAME "Longmont, CO"

// Units: imperial (Fahrenheit, mph) or metric (Celsius, m/s)
#define WEATHER_UNITS "imperial"

// Display Configuration (M5Stack Paper S3)
#define DISPLAY_WIDTH 540
#define DISPLAY_HEIGHT 960
#define DISPLAY_ROTATION 2  // Portrait mode, rotated 180 degrees

// Time Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (-7 * 3600)  // Mountain Time (GMT-7)
#define DAYLIGHT_OFFSET_SEC 0       // Adjust for DST if needed

// Update Schedule (times in hours, 24h format)
// Updates at 00:00, 06:00, 12:00, 18:00
#define UPDATE_INTERVAL_HOURS 6
const int UPDATE_TIMES[] = {0, 6, 12, 18};
#define NUM_UPDATE_TIMES 4

// Error retry interval (5 minutes)
#define ERROR_RETRY_SECONDS 300

// Forecast settings
#define HOURLY_FORECAST_COUNT 5   // Number of hourly forecasts to show
#define DAILY_FORECAST_COUNT 7    // Number of daily forecasts to show

#endif // CONFIG_H
