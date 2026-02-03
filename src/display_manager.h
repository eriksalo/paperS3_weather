#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Unified.h>
#include "weather_api.h"

class DisplayManager {
public:
    DisplayManager();

    // Initialize the display
    void begin();

    // Clear the display
    void clear();

    // Render complete weather display
    void renderWeather(WeatherData& weather);

    // Render error message
    void renderError(const String& message);

    // Render connecting/loading status
    void renderStatus(const String& message);

    // Push display buffer to e-ink
    void update();

private:
    // Layout constants
    static const int HEADER_HEIGHT = 40;
    static const int CURRENT_HEIGHT = 240;
    static const int HOURLY_HEIGHT = 120;
    static const int FOOTER_HEIGHT = 40;

    // Section Y positions
    int headerY;
    int currentY;
    int hourlyY;
    int dailyY;
    int footerY;

    // Render individual sections
    void renderHeader();
    void renderCurrentWeather(CurrentWeather& current);
    void renderHourlyForecast(HourlyForecast* hourly, int count);
    void renderDailyForecast(DailyForecast* daily, int count);
    void renderFooter();

    // Weather icon drawing
    void drawWeatherIcon(int x, int y, int size, int weatherId, bool isNight = false);
    void drawSunIcon(int x, int y, int size);
    void drawMoonIcon(int x, int y, int size);
    void drawCloudIcon(int x, int y, int size);
    void drawRainIcon(int x, int y, int size);
    void drawSnowIcon(int x, int y, int size);
    void drawThunderIcon(int x, int y, int size);
    void drawFogIcon(int x, int y, int size);
    void drawPartlyCloudyIcon(int x, int y, int size, bool isNight = false);
    void drawStar(int x, int y, int size);
    void drawSnowflake(int x, int y, int size);

    // Utility functions
    String getDayName(time_t timestamp);
    String formatTime(time_t timestamp);
    String formatHourlyTime(time_t timestamp);
    String formatDate(time_t timestamp);
    bool isNightTime(time_t timestamp, time_t sunrise, time_t sunset);
    String capitalizeFirst(const String& str);
};

#endif // DISPLAY_MANAGER_H
