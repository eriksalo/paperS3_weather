#include "display_manager.h"
#include "config.h"
#include <time.h>

// Use actual display dimensions
#define SCREEN_W 540
#define SCREEN_H 960

DisplayManager::DisplayManager() {
    // Calculate section positions for 540x960 portrait
    headerY = 0;
    currentY = 60;           // After header
    hourlyY = 370;           // After current weather
    dailyY = 510;            // After hourly
    footerY = SCREEN_H - 40; // Bottom footer
}

void DisplayManager::begin() {
    Serial.println("DisplayManager::begin() starting...");
    Serial.printf("  Display width: %d, height: %d\n", M5.Display.width(), M5.Display.height());

    // Set rotation for portrait mode
    M5.Display.setRotation(DISPLAY_ROTATION);
    Serial.printf("  After rotation: %d x %d\n", M5.Display.width(), M5.Display.height());

    // Set EPD mode for quality
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    Serial.println("  EPD mode set to quality");

    // Set default text settings - use simple font
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextDatum(TL_DATUM);
    Serial.println("  Text settings configured");

    clear();
    Serial.println("DisplayManager::begin() complete");
}

void DisplayManager::clear() {
    M5.Display.fillScreen(TFT_WHITE);
}

void DisplayManager::update() {
    Serial.println("  Pushing to e-ink display...");
    M5.Display.display();
    Serial.println("  Display update complete");
}

void DisplayManager::renderWeather(WeatherData& weather) {
    clear();

    renderHeader();
    renderCurrentWeather(weather.current);
    renderHourlyForecast(weather.hourly, min(weather.hourlyCount, 5));
    renderDailyForecast(weather.daily, min(weather.dailyCount, 6));
    renderFooter();

    update();
}

void DisplayManager::renderError(const String& message) {
    clear();

    int centerX = SCREEN_W / 2;
    int centerY = SCREEN_H / 2;

    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextSize(4);
    M5.Display.drawString("Error", centerX, centerY - 50);

    M5.Display.setTextSize(2);
    M5.Display.drawString(message.c_str(), centerX, centerY + 10);

    M5.Display.setTextSize(2);
    M5.Display.drawString("Will retry in 5 minutes", centerX, centerY + 50);

    M5.Display.setTextDatum(TL_DATUM);
    update();
}

void DisplayManager::renderStatus(const String& message) {
    clear();

    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextSize(4);
    M5.Display.drawString(message.c_str(), SCREEN_W / 2, SCREEN_H / 2);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextSize(2);

    update();
}

void DisplayManager::renderHeader() {
    // Battery indicator (left side)
    int batteryLevel = M5.Power.getBatteryLevel();
    M5.Display.setTextSize(2);

    // Draw battery outline
    int batX = 15;
    int batY = 15;
    M5.Display.drawRect(batX, batY, 40, 20, TFT_BLACK);
    M5.Display.fillRect(batX + 40, batY + 5, 5, 10, TFT_BLACK);

    // Fill based on battery level
    int fillWidth = (batteryLevel * 36) / 100;
    if (fillWidth > 0) {
        M5.Display.fillRect(batX + 2, batY + 2, fillWidth, 16, TFT_BLACK);
    }

    // Battery percentage
    M5.Display.drawString(String(batteryLevel) + "%", batX + 50, batY + 2);

    // Location name (center)
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.setTextSize(4);
    M5.Display.drawString(LOCATION_NAME, SCREEN_W / 2, 6);
    M5.Display.setTextDatum(TL_DATUM);

    // Current time (right side)
    time_t now;
    time(&now);
    String timeStr = formatTime(now);
    M5.Display.setTextSize(2);
    int timeWidth = M5.Display.textWidth(timeStr.c_str());
    M5.Display.drawString(timeStr.c_str(), SCREEN_W - timeWidth - 15, 15);

    // Separator line
    M5.Display.drawLine(0, 52, SCREEN_W, 52, TFT_BLACK);
}

void DisplayManager::renderCurrentWeather(CurrentWeather& current) {
    int centerX = SCREEN_W / 2;
    int y = currentY + 10;

    // Weather icon (centered)
    int iconSize = 90;
    drawWeatherIcon(centerX - iconSize / 2, y, iconSize, current.weatherId,
                    isNightTime(current.timestamp, current.sunrise, current.sunset));
    y += iconSize + 15;

    // Temperature (large)
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextSize(8);
    String tempStr = String((int)round(current.temp)) + "F";
    M5.Display.drawString(tempStr.c_str(), centerX, y);
    y += 70;

    // Description
    M5.Display.setTextSize(4);
    String desc = capitalizeFirst(current.description);
    M5.Display.drawString(desc.c_str(), centerX, y);
    y += 45;

    // Details row
    M5.Display.setTextSize(2);
    String details = "Feels " + String((int)round(current.feelsLike)) + "  " +
                     String(current.humidity) + "%  " +
                     String((int)round(current.windSpeed)) + "mph";
    M5.Display.drawString(details.c_str(), centerX, y);

    M5.Display.setTextDatum(TL_DATUM);

    // Separator line
    M5.Display.drawLine(10, hourlyY - 8, SCREEN_W - 10, hourlyY - 8, TFT_BLACK);
}

void DisplayManager::renderHourlyForecast(HourlyForecast* hourly, int count) {
    if (count == 0) return;

    int y = hourlyY;

    // Section title
    M5.Display.setTextSize(2);
    M5.Display.drawString("HOURLY", 15, y);
    y += 24;

    // Calculate column positions (5 columns)
    int cols = min(count, 5);
    int colWidth = SCREEN_W / cols;
    int startX = 0;

    for (int i = 0; i < cols && i < count; i++) {
        int colX = startX + i * colWidth + colWidth / 2;

        // Time label
        String timeLabel;
        if (i == 0) {
            timeLabel = "Now";
        } else {
            timeLabel = "+" + String(i * 3) + "h";
        }

        M5.Display.setTextDatum(TC_DATUM);
        M5.Display.setTextSize(2);
        M5.Display.drawString(timeLabel.c_str(), colX, y);

        // Weather icon
        int iconSize = 40;
        drawWeatherIcon(colX - iconSize / 2, y + 22, iconSize, hourly[i].weatherId);

        // Temperature
        M5.Display.setTextSize(4);
        String temp = String((int)round(hourly[i].temp));
        M5.Display.drawString(temp.c_str(), colX, y + 70);
    }

    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextSize(2);

    // Separator line
    M5.Display.drawLine(10, dailyY - 8, SCREEN_W - 10, dailyY - 8, TFT_BLACK);
}

void DisplayManager::renderDailyForecast(DailyForecast* daily, int count) {
    if (count == 0) return;

    int y = dailyY;

    // Section title
    M5.Display.setTextSize(2);
    M5.Display.drawString("DAILY", 15, y);
    y += 28;

    // Calculate row height
    int availableHeight = footerY - y - 15;
    int rowHeight = availableHeight / min(count, 6);

    for (int i = 0; i < count && i < 6; i++) {
        int rowY = y + i * rowHeight;

        // Day name (left)
        M5.Display.setTextSize(3);
        String dayName = getDayName(daily[i].timestamp);
        M5.Display.drawString(dayName.c_str(), 15, rowY + 10);

        // Weather icon
        int iconSize = 38;
        drawWeatherIcon(90, rowY + 4, iconSize, daily[i].weatherId);

        // Description (middle) - truncate if needed
        M5.Display.setTextSize(2);
        String desc = capitalizeFirst(daily[i].description);
        if (desc.length() > 10) {
            desc = desc.substring(0, 8) + "..";
        }
        M5.Display.drawString(desc.c_str(), 140, rowY + 14);

        // Precipitation % (if significant)
        if (daily[i].pop > 20) {
            M5.Display.drawString(String(daily[i].pop) + "%", 290, rowY + 14);
        }

        // High/Low temps (right aligned)
        M5.Display.setTextSize(4);
        String highLow = String((int)round(daily[i].tempMax)) + "/" +
                         String((int)round(daily[i].tempMin));
        int tempWidth = M5.Display.textWidth(highLow.c_str());
        M5.Display.drawString(highLow.c_str(), SCREEN_W - tempWidth - 15, rowY + 6);

        // Subtle row divider
        M5.Display.setTextSize(2);
        if (i < count - 1 && i < 5) {
            M5.Display.drawLine(15, rowY + rowHeight - 3, SCREEN_W - 15, rowY + rowHeight - 3, TFT_LIGHTGRAY);
        }
    }
}

void DisplayManager::renderFooter() {
    // Separator line
    M5.Display.drawLine(0, footerY, SCREEN_W, footerY, TFT_BLACK);

    // Last update time (centered)
    time_t now;
    time(&now);

    M5.Display.setTextSize(2);
    M5.Display.setTextDatum(MC_DATUM);
    String updateStr = "Updated " + formatDate(now) + " " + formatTime(now);
    M5.Display.drawString(updateStr.c_str(), SCREEN_W / 2, footerY + 18);
    M5.Display.setTextDatum(TL_DATUM);
}

// Weather icon drawing functions
void DisplayManager::drawWeatherIcon(int x, int y, int size, int weatherId, bool isNight) {
    if (weatherId >= 200 && weatherId < 300) {
        drawThunderIcon(x, y, size);
    } else if (weatherId >= 300 && weatherId < 600) {
        drawRainIcon(x, y, size);
    } else if (weatherId >= 600 && weatherId < 700) {
        drawSnowIcon(x, y, size);
    } else if (weatherId >= 700 && weatherId < 800) {
        drawFogIcon(x, y, size);
    } else if (weatherId == 800) {
        if (isNight) {
            drawMoonIcon(x, y, size);
        } else {
            drawSunIcon(x, y, size);
        }
    } else if (weatherId > 800) {
        if (weatherId <= 802) {
            drawPartlyCloudyIcon(x, y, size, isNight);
        } else {
            drawCloudIcon(x, y, size);
        }
    }
}

void DisplayManager::drawSunIcon(int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 4;

    // Sun circle (filled)
    M5.Display.fillCircle(cx, cy, r, TFT_BLACK);

    // Rays
    int rayLen = size * 2 / 5;
    int rayGap = r + 2;
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int x1 = cx + cos(angle) * rayGap;
        int y1 = cy + sin(angle) * rayGap;
        int x2 = cx + cos(angle) * rayLen;
        int y2 = cy + sin(angle) * rayLen;
        M5.Display.drawLine(x1, y1, x2, y2, TFT_BLACK);
    }
}

void DisplayManager::drawMoonIcon(int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 3;

    M5.Display.fillCircle(cx, cy, r, TFT_BLACK);
    M5.Display.fillCircle(cx + r * 0.6, cy - r * 0.3, r * 0.85, TFT_WHITE);
}

void DisplayManager::drawCloudIcon(int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 5;

    // Cloud shape
    M5.Display.fillCircle(cx - r, cy + r/2, r, TFT_BLACK);
    M5.Display.fillCircle(cx, cy - r/3, r * 1.3, TFT_BLACK);
    M5.Display.fillCircle(cx + r, cy + r/2, r, TFT_BLACK);
    M5.Display.fillRect(cx - r, cy + r/2, r * 2, r, TFT_BLACK);
}

void DisplayManager::drawRainIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.8);

    // Rain drops
    int dropY = y + size / 2;
    int cx = x + size / 2;
    for (int i = -1; i <= 1; i++) {
        int dx = cx + i * size / 5;
        M5.Display.drawLine(dx, dropY, dx - 4, dropY + size/4, TFT_BLACK);
        M5.Display.drawLine(dx - 1, dropY, dx - 5, dropY + size/4, TFT_BLACK);
    }
}

void DisplayManager::drawSnowIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.8);

    // Snowflakes - scale based on icon size
    int flakeY = y + size / 2;
    int cx = x + size / 2;
    int dotSize = max(1, size / 15);
    for (int i = -1; i <= 1; i++) {
        M5.Display.fillCircle(cx + i * size/5, flakeY + 3, dotSize, TFT_BLACK);
        M5.Display.fillCircle(cx + i * size/5 - 3, flakeY + size/4, dotSize, TFT_BLACK);
    }
}

void DisplayManager::drawThunderIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.8);

    // Lightning bolt
    int bx = x + size / 2;
    int by = y + size / 2;

    M5.Display.fillTriangle(bx, by, bx - 8, by + 12, bx + 4, by + 10, TFT_BLACK);
    M5.Display.fillTriangle(bx - 4, by + 8, bx - 10, by + 22, bx + 2, by + 14, TFT_BLACK);
}

void DisplayManager::drawFogIcon(int x, int y, int size) {
    int cy = y + size / 3;
    int lineSpacing = size / 5;

    for (int i = 0; i < 4; i++) {
        int sx = (i % 2 == 0) ? x + 8 : x + 18;
        int ex = (i % 2 == 0) ? x + size - 18 : x + size - 8;
        int ly = cy + i * lineSpacing;
        M5.Display.drawLine(sx, ly, ex, ly, TFT_BLACK);
        M5.Display.drawLine(sx, ly + 1, ex, ly + 1, TFT_BLACK);
    }
}

void DisplayManager::drawPartlyCloudyIcon(int x, int y, int size, bool isNight) {
    // Sun/moon in background (smaller, offset)
    if (isNight) {
        drawMoonIcon(x, y - size/8, size * 0.6);
    } else {
        drawSunIcon(x, y - size/8, size * 0.6);
    }

    // Cloud in foreground (offset down-right)
    int cloudX = x + size / 4;
    int cloudY = y + size / 4;
    int cs = size * 0.7;
    int r = cs / 5;

    // White background to cover sun
    M5.Display.fillCircle(cloudX + cs/2 - r, cloudY + cs/2, r + 3, TFT_WHITE);
    M5.Display.fillCircle(cloudX + cs/2, cloudY + cs/3, r * 1.3 + 3, TFT_WHITE);
    M5.Display.fillCircle(cloudX + cs/2 + r, cloudY + cs/2, r + 3, TFT_WHITE);

    // Cloud shape
    M5.Display.fillCircle(cloudX + cs/2 - r, cloudY + cs/2, r, TFT_BLACK);
    M5.Display.fillCircle(cloudX + cs/2, cloudY + cs/3, r * 1.3, TFT_BLACK);
    M5.Display.fillCircle(cloudX + cs/2 + r, cloudY + cs/2, r, TFT_BLACK);
    M5.Display.fillRect(cloudX + cs/2 - r, cloudY + cs/2, r * 2, r, TFT_BLACK);
}

// Utility functions
String DisplayManager::getDayName(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return String(days[timeinfo->tm_wday]);
}

String DisplayManager::formatTime(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    char buffer[12];
    int hour = timeinfo->tm_hour;
    const char* ampm = (hour >= 12) ? "PM" : "AM";
    hour = hour % 12;
    if (hour == 0) hour = 12;
    sprintf(buffer, "%d:%02d %s", hour, timeinfo->tm_min, ampm);
    return String(buffer);
}

String DisplayManager::formatDate(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char buffer[12];
    sprintf(buffer, "%s %d", months[timeinfo->tm_mon], timeinfo->tm_mday);
    return String(buffer);
}

bool DisplayManager::isNightTime(time_t timestamp, time_t sunrise, time_t sunset) {
    return (timestamp < sunrise || timestamp > sunset);
}

String DisplayManager::capitalizeFirst(const String& str) {
    if (str.length() == 0) return str;
    String result = str;
    result[0] = toupper(result[0]);
    return result;
}
