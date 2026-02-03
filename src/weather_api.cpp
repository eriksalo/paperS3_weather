#include "weather_api.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherAPI::WeatherAPI() {
    data.valid = false;
    data.hourlyCount = 0;
    data.dailyCount = 0;
}

bool WeatherAPI::fetchWeather(float lat, float lon, const char* apiKey, const char* units) {
    data.valid = false;
    data.errorMessage = "";

    if (WiFi.status() != WL_CONNECTED) {
        data.errorMessage = "WiFi not connected";
        return false;
    }

    // Fetch current weather using free API
    if (!fetchCurrentWeather(lat, lon, apiKey, units)) {
        return false;
    }

    // Fetch forecast using free API
    if (!fetchForecast(lat, lon, apiKey, units)) {
        return false;
    }

    data.valid = true;
    Serial.printf("Weather parsed: %.1f°, %d hourly, %d daily forecasts\n",
                  data.current.temp, data.hourlyCount, data.dailyCount);

    return true;
}

bool WeatherAPI::fetchCurrentWeather(float lat, float lon, const char* apiKey, const char* units) {
    // Build API URL for free current weather API
    String url = "https://";
    url += OWM_API_HOST;
    url += "/data/2.5/weather?lat=";
    url += String(lat, 4);
    url += "&lon=";
    url += String(lon, 4);
    url += "&units=";
    url += units;
    url += "&appid=";
    url += apiKey;

    Serial.println("Fetching current weather: " + url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        data.errorMessage = "Current weather HTTP error: " + String(httpCode);
        Serial.println(data.errorMessage);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse current weather response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        data.errorMessage = "JSON parse error: " + String(error.c_str());
        Serial.println(data.errorMessage);
        return false;
    }

    // Parse main data
    data.current.timestamp = doc["dt"].as<time_t>();
    data.current.temp = doc["main"]["temp"].as<float>();
    data.current.feelsLike = doc["main"]["feels_like"].as<float>();
    data.current.humidity = doc["main"]["humidity"].as<int>();
    data.current.pressure = doc["main"]["pressure"].as<int>();
    data.current.windSpeed = doc["wind"]["speed"].as<float>();
    data.current.windDeg = doc["wind"]["deg"].as<int>();
    data.current.visibility = doc["visibility"].as<int>();

    // Parse sunrise/sunset from sys
    data.current.sunrise = doc["sys"]["sunrise"].as<time_t>();
    data.current.sunset = doc["sys"]["sunset"].as<time_t>();

    // Parse weather condition
    JsonArray weather = doc["weather"];
    if (weather.size() > 0) {
        data.current.weatherId = weather[0]["id"].as<int>();
        data.current.description = weather[0]["description"].as<String>();
        data.current.icon = weather[0]["icon"].as<String>();
    }

    Serial.printf("Current: %.1f°F, %s\n", data.current.temp, data.current.description.c_str());
    return true;
}

bool WeatherAPI::fetchForecast(float lat, float lon, const char* apiKey, const char* units) {
    // Build API URL for free 5-day forecast API
    String url = "https://";
    url += OWM_API_HOST;
    url += "/data/2.5/forecast?lat=";
    url += String(lat, 4);
    url += "&lon=";
    url += String(lon, 4);
    url += "&units=";
    url += units;
    url += "&appid=";
    url += apiKey;

    Serial.println("Fetching forecast: " + url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        data.errorMessage = "Forecast HTTP error: " + String(httpCode);
        Serial.println(data.errorMessage);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse forecast response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        data.errorMessage = "Forecast JSON parse error: " + String(error.c_str());
        Serial.println(data.errorMessage);
        return false;
    }

    JsonArray list = doc["list"];

    // Parse hourly forecasts (every 3 hours, take first 5 = 15 hours)
    data.hourlyCount = 0;
    for (int i = 0; i < list.size() && data.hourlyCount < 12; i++) {
        JsonObject item = list[i];
        data.hourly[data.hourlyCount].timestamp = item["dt"].as<time_t>();
        data.hourly[data.hourlyCount].temp = item["main"]["temp"].as<float>();
        data.hourly[data.hourlyCount].humidity = item["main"]["humidity"].as<int>();

        JsonArray weather = item["weather"];
        if (weather.size() > 0) {
            data.hourly[data.hourlyCount].weatherId = weather[0]["id"].as<int>();
            data.hourly[data.hourlyCount].description = weather[0]["description"].as<String>();
            data.hourly[data.hourlyCount].icon = weather[0]["icon"].as<String>();
        }
        data.hourlyCount++;
    }

    // Parse daily forecasts - aggregate by day
    // The 5-day forecast gives 3-hour intervals, we need to find daily min/max
    data.dailyCount = 0;
    int currentDay = -1;
    float dayMin = 999, dayMax = -999;
    int dayWeatherId = 0;
    String dayDesc = "";
    time_t dayTimestamp = 0;
    int dayPop = 0;

    for (int i = 0; i < list.size() && data.dailyCount < 8; i++) {
        JsonObject item = list[i];
        time_t ts = item["dt"].as<time_t>();
        struct tm* timeinfo = localtime(&ts);
        int day = timeinfo->tm_mday;

        float temp = item["main"]["temp"].as<float>();
        int pop = (int)(item["pop"].as<float>() * 100);

        if (day != currentDay) {
            // Save previous day if exists
            if (currentDay != -1 && data.dailyCount < 8) {
                data.daily[data.dailyCount].timestamp = dayTimestamp;
                data.daily[data.dailyCount].tempMin = dayMin;
                data.daily[data.dailyCount].tempMax = dayMax;
                data.daily[data.dailyCount].weatherId = dayWeatherId;
                data.daily[data.dailyCount].description = dayDesc;
                data.daily[data.dailyCount].pop = dayPop;
                data.daily[data.dailyCount].humidity = 0;
                data.dailyCount++;
            }

            // Start new day
            currentDay = day;
            dayMin = temp;
            dayMax = temp;
            dayTimestamp = ts;
            dayPop = pop;

            JsonArray weather = item["weather"];
            if (weather.size() > 0) {
                dayWeatherId = weather[0]["id"].as<int>();
                dayDesc = weather[0]["description"].as<String>();
            }
        } else {
            // Update min/max
            if (temp < dayMin) dayMin = temp;
            if (temp > dayMax) dayMax = temp;
            if (pop > dayPop) dayPop = pop;
        }
    }

    // Don't forget last day
    if (currentDay != -1 && data.dailyCount < 8) {
        data.daily[data.dailyCount].timestamp = dayTimestamp;
        data.daily[data.dailyCount].tempMin = dayMin;
        data.daily[data.dailyCount].tempMax = dayMax;
        data.daily[data.dailyCount].weatherId = dayWeatherId;
        data.daily[data.dailyCount].description = dayDesc;
        data.daily[data.dailyCount].pop = dayPop;
        data.daily[data.dailyCount].humidity = 0;
        data.dailyCount++;
    }

    Serial.printf("Parsed %d hourly, %d daily forecasts\n", data.hourlyCount, data.dailyCount);
    return true;
}

WeatherData& WeatherAPI::getData() {
    return data;
}

String WeatherAPI::getError() {
    return data.errorMessage;
}

String WeatherAPI::getWeatherCategory(int weatherId) {
    if (weatherId >= 200 && weatherId < 300) return "thunderstorm";
    if (weatherId >= 300 && weatherId < 400) return "drizzle";
    if (weatherId >= 500 && weatherId < 600) return "rain";
    if (weatherId >= 600 && weatherId < 700) return "snow";
    if (weatherId >= 700 && weatherId < 800) return "atmosphere";
    if (weatherId == 800) return "clear";
    if (weatherId > 800 && weatherId < 900) return "clouds";
    return "unknown";
}
