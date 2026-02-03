#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <Arduino.h>

// Hourly forecast data structure
struct HourlyForecast {
    time_t timestamp;
    float temp;
    int humidity;
    String description;
    String icon;
    int weatherId;
};

// Daily forecast data structure
struct DailyForecast {
    time_t timestamp;
    float tempMin;
    float tempMax;
    int humidity;
    String description;
    String icon;
    int weatherId;
    int pop;  // Probability of precipitation (0-100)
};

// Current weather data structure
struct CurrentWeather {
    time_t timestamp;
    float temp;
    float feelsLike;
    int humidity;
    float windSpeed;
    int windDeg;
    String description;
    String icon;
    int weatherId;
    int visibility;
    int pressure;
    time_t sunrise;
    time_t sunset;
};

// Complete weather data
struct WeatherData {
    bool valid;
    CurrentWeather current;
    HourlyForecast hourly[12];  // Up to 12 hours
    int hourlyCount;
    DailyForecast daily[8];     // Up to 8 days
    int dailyCount;
    String errorMessage;
};

class WeatherAPI {
public:
    WeatherAPI();

    // Fetch weather data from OpenWeatherMap
    bool fetchWeather(float lat, float lon, const char* apiKey, const char* units);

    // Get the fetched weather data
    WeatherData& getData();

    // Get error message if fetch failed
    String getError();

private:
    WeatherData data;

    // Fetch current weather from free API
    bool fetchCurrentWeather(float lat, float lon, const char* apiKey, const char* units);

    // Fetch forecast from free API
    bool fetchForecast(float lat, float lon, const char* apiKey, const char* units);

    // Map weather condition ID to simplified category
    String getWeatherCategory(int weatherId);
};

#endif // WEATHER_API_H
