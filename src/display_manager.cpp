#include "display_manager.h"
#include "config.h"
#include <time.h>

// Use actual display dimensions
#define SCREEN_W 540
#define SCREEN_H 960

DisplayManager::DisplayManager() {
    // Calculate section positions for 540x960 portrait
    headerY = 0;
    currentY = 70;           // After header
    hourlyY = 400;           // After current weather
    dailyY = 540;            // After hourly
    footerY = SCREEN_H - 50; // Bottom footer
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

    // Set default text settings
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextSize(2);
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

    // Decorative border
    M5.Display.drawRoundRect(50, centerY - 100, SCREEN_W - 100, 200, 10, TFT_BLACK);
    M5.Display.drawRoundRect(52, centerY - 98, SCREEN_W - 104, 196, 8, TFT_BLACK);

    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.drawString("Error", centerX, centerY - 45);

    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawString(message.c_str(), centerX, centerY + 10);

    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("Will retry in 5 minutes", centerX, centerY + 55);

    M5.Display.setTextDatum(TL_DATUM);
    update();
}

void DisplayManager::renderStatus(const String& message) {
    clear();

    int centerX = SCREEN_W / 2;
    int centerY = SCREEN_H / 2;

    // Decorative elements
    M5.Display.drawLine(centerX - 100, centerY - 40, centerX + 100, centerY - 40, TFT_BLACK);
    M5.Display.drawLine(centerX - 80, centerY + 40, centerX + 80, centerY + 40, TFT_BLACK);

    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.drawString(message.c_str(), centerX, centerY);
    M5.Display.setTextDatum(TL_DATUM);

    update();
}

void DisplayManager::renderHeader() {
    // Elegant header with decorative elements

    // Battery indicator (left side) - stylized
    int batteryLevel = M5.Power.getBatteryLevel();
    int batX = 20;
    int batY = 18;

    // Rounded battery outline
    M5.Display.drawRoundRect(batX, batY, 44, 22, 3, TFT_BLACK);
    M5.Display.drawRoundRect(batX + 1, batY + 1, 42, 20, 2, TFT_BLACK);
    M5.Display.fillRoundRect(batX + 44, batY + 6, 6, 10, 2, TFT_BLACK);

    // Fill based on battery level
    int fillWidth = (batteryLevel * 38) / 100;
    if (fillWidth > 0) {
        M5.Display.fillRoundRect(batX + 3, batY + 3, fillWidth, 16, 2, TFT_BLACK);
    }

    // Battery percentage - modern font
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString(String(batteryLevel) + "%", batX + 52, batY + 4);

    // Location name (center)
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.drawString(LOCATION_NAME, SCREEN_W / 2, 15);

    // Current time (right side)
    time_t now;
    time(&now);
    String timeStr = formatTime(now);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextDatum(TR_DATUM);
    M5.Display.drawString(timeStr.c_str(), SCREEN_W - 20, 20);
    M5.Display.setTextDatum(TL_DATUM);

    // Decorative double line separator
    M5.Display.drawLine(30, 55, SCREEN_W - 30, 55, TFT_BLACK);
    M5.Display.drawLine(60, 60, SCREEN_W - 60, 60, TFT_BLACK);
}

void DisplayManager::renderCurrentWeather(CurrentWeather& current) {
    int centerX = SCREEN_W / 2;
    int y = currentY + 10;

    // Weather icon (centered) - detailed
    int iconSize = 100;
    drawWeatherIcon(centerX - iconSize / 2, y, iconSize, current.weatherId,
                    isNightTime(current.timestamp, current.sunrise, current.sunset));
    y += iconSize + 20;

    // Temperature - large modern font
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setFont(&fonts::FreeSansBold24pt7b);
    String tempStr = String((int)round(current.temp)) + "°F";
    M5.Display.drawString(tempStr.c_str(), centerX, y);
    y += 50;

    // Description
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    String desc = capitalizeFirst(current.description);
    M5.Display.drawString(desc.c_str(), centerX, y);
    y += 35;

    // Feels like
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    String feels = "Feels like " + String((int)round(current.feelsLike)) + "°";
    M5.Display.drawString(feels.c_str(), centerX, y);
    y += 25;

    // Humidity and Wind
    String details = String(current.humidity) + "% humidity  ·  " +
                     String((int)round(current.windSpeed)) + " mph wind";
    M5.Display.drawString(details.c_str(), centerX, y);

    M5.Display.setTextDatum(TL_DATUM);

    // Elegant separator with diamond
    int lineY = hourlyY - 12;
    M5.Display.drawLine(50, lineY, SCREEN_W - 50, lineY, TFT_BLACK);
    int diamondX = centerX;
    M5.Display.fillTriangle(diamondX, lineY - 5, diamondX - 5, lineY, diamondX, lineY + 5, TFT_BLACK);
    M5.Display.fillTriangle(diamondX, lineY - 5, diamondX + 5, lineY, diamondX, lineY + 5, TFT_BLACK);
}

void DisplayManager::renderHourlyForecast(HourlyForecast* hourly, int count) {
    if (count == 0) return;

    int y = hourlyY;

    // Section title - spaced letters
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("HOURLY", 20, y);
    y += 22;

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
        M5.Display.setFont(&fonts::FreeSans9pt7b);
        M5.Display.drawString(timeLabel.c_str(), colX, y);

        // Weather icon
        int iconSize = 40;
        drawWeatherIcon(colX - iconSize / 2, y + 18, iconSize, hourly[i].weatherId);

        // Temperature
        M5.Display.setFont(&fonts::FreeSansBold12pt7b);
        String temp = String((int)round(hourly[i].temp)) + "°";
        M5.Display.drawString(temp.c_str(), colX, y + 68);
    }

    M5.Display.setTextDatum(TL_DATUM);

    // Elegant separator with diamond
    int lineY = dailyY - 12;
    M5.Display.drawLine(50, lineY, SCREEN_W - 50, lineY, TFT_BLACK);
    int diamondX = SCREEN_W / 2;
    M5.Display.fillTriangle(diamondX, lineY - 5, diamondX - 5, lineY, diamondX, lineY + 5, TFT_BLACK);
    M5.Display.fillTriangle(diamondX, lineY - 5, diamondX + 5, lineY, diamondX, lineY + 5, TFT_BLACK);
}

void DisplayManager::renderDailyForecast(DailyForecast* daily, int count) {
    if (count == 0) return;

    int y = dailyY;

    // Section title
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("EXTENDED FORECAST", 20, y);
    y += 24;

    // Calculate row height
    int availableHeight = footerY - y - 15;
    int rowHeight = availableHeight / min(count, 6);

    for (int i = 0; i < count && i < 6; i++) {
        int rowY = y + i * rowHeight;

        // Day name (left)
        M5.Display.setFont(&fonts::FreeSansBold9pt7b);
        String dayName = getDayName(daily[i].timestamp);
        M5.Display.drawString(dayName.c_str(), 20, rowY + 12);

        // Weather icon
        int iconSize = 36;
        drawWeatherIcon(80, rowY + 4, iconSize, daily[i].weatherId);

        // Description (middle)
        M5.Display.setFont(&fonts::FreeSans9pt7b);
        String desc = capitalizeFirst(daily[i].description);
        if (desc.length() > 12) {
            desc = desc.substring(0, 10) + "..";
        }
        M5.Display.drawString(desc.c_str(), 130, rowY + 12);

        // Precipitation % (if significant)
        if (daily[i].pop > 20) {
            M5.Display.drawString(String(daily[i].pop) + "%", 280, rowY + 12);
        }

        // High/Low temps (right aligned)
        M5.Display.setFont(&fonts::FreeSansBold12pt7b);
        String temps = String((int)round(daily[i].tempMax)) + "°/" +
                       String((int)round(daily[i].tempMin)) + "°";
        M5.Display.setTextDatum(TR_DATUM);
        M5.Display.drawString(temps.c_str(), SCREEN_W - 15, rowY + 8);
        M5.Display.setTextDatum(TL_DATUM);

        // Elegant dotted row divider
        if (i < count - 1 && i < 5) {
            int dotY = rowY + rowHeight - 4;
            for (int dx = 40; dx < SCREEN_W - 40; dx += 8) {
                M5.Display.fillCircle(dx, dotY, 1, TFT_BLACK);
            }
        }
    }
}

void DisplayManager::renderFooter() {
    // Decorative double line separator
    M5.Display.drawLine(60, footerY, SCREEN_W - 60, footerY, TFT_BLACK);
    M5.Display.drawLine(30, footerY + 5, SCREEN_W - 30, footerY + 5, TFT_BLACK);

    // Last update time (centered)
    time_t now;
    time(&now);

    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextDatum(MC_DATUM);
    String updateStr = "Updated " + formatDate(now) + " at " + formatTime(now);
    M5.Display.drawString(updateStr.c_str(), SCREEN_W / 2, footerY + 28);
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

    // Sun circle with gradient effect (concentric circles)
    M5.Display.fillCircle(cx, cy, r, TFT_BLACK);
    M5.Display.drawCircle(cx, cy, r + 2, TFT_BLACK);

    // Elegant rays - alternating long and short
    int rayLenLong = size * 2 / 5;
    int rayLenShort = size / 3;
    int rayGap = r + 4;

    for (int i = 0; i < 12; i++) {
        float angle = i * PI / 6;
        int rayLen = (i % 2 == 0) ? rayLenLong : rayLenShort;
        int x1 = cx + cos(angle) * rayGap;
        int y1 = cy + sin(angle) * rayGap;
        int x2 = cx + cos(angle) * rayLen;
        int y2 = cy + sin(angle) * rayLen;

        // Thicker rays for main directions
        M5.Display.drawLine(x1, y1, x2, y2, TFT_BLACK);
        if (i % 3 == 0) {
            M5.Display.drawLine(x1 + 1, y1, x2 + 1, y2, TFT_BLACK);
        }
    }
}

void DisplayManager::drawMoonIcon(int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 3;

    // Crescent moon with elegant curve
    M5.Display.fillCircle(cx, cy, r, TFT_BLACK);
    M5.Display.fillCircle(cx + r * 0.55, cy - r * 0.25, r * 0.82, TFT_WHITE);

    // Add subtle stars around moon
    int starSize = max(2, size / 20);
    // Star 1
    drawStar(cx - r - starSize * 2, cy - r / 2, starSize);
    // Star 2
    drawStar(cx + r / 2, cy - r - starSize, starSize - 1);
    // Star 3
    drawStar(cx - r / 2, cy + r, starSize - 1);
}

void DisplayManager::drawCloudIcon(int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 5;

    // Fluffy cloud with multiple bumps for more natural look
    // Bottom base
    M5.Display.fillCircle(cx - r * 1.2, cy + r * 0.4, r * 0.9, TFT_BLACK);
    M5.Display.fillCircle(cx + r * 1.2, cy + r * 0.4, r * 0.9, TFT_BLACK);

    // Middle bumps
    M5.Display.fillCircle(cx - r * 0.5, cy - r * 0.2, r * 1.1, TFT_BLACK);
    M5.Display.fillCircle(cx + r * 0.5, cy, r * 1.0, TFT_BLACK);

    // Top bump
    M5.Display.fillCircle(cx, cy - r * 0.5, r * 1.2, TFT_BLACK);

    // Fill gaps
    M5.Display.fillRect(cx - r * 1.2, cy + r * 0.3, r * 2.4, r * 0.8, TFT_BLACK);

    // Outline for definition
    M5.Display.drawCircle(cx, cy - r * 0.5, r * 1.2, TFT_BLACK);
}

void DisplayManager::drawRainIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.75);

    // Elegant raindrops - teardrop shape
    int dropStartY = y + size / 2 - 5;
    int cx = x + size / 2;
    int dropLen = size / 4;

    for (int i = -1; i <= 1; i++) {
        int dx = cx + i * size / 4;
        int dy = dropStartY + (i == 0 ? 0 : 5);  // Stagger drops

        // Teardrop shape
        M5.Display.fillCircle(dx, dy + dropLen, size / 15 + 1, TFT_BLACK);
        M5.Display.fillTriangle(
            dx, dy,
            dx - size / 15 - 1, dy + dropLen,
            dx + size / 15 + 1, dy + dropLen,
            TFT_BLACK
        );
    }
}

void DisplayManager::drawSnowIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.75);

    // Elegant snowflakes
    int flakeY = y + size / 2;
    int cx = x + size / 2;
    int flakeSize = max(4, size / 8);

    // Draw three snowflakes at different positions
    drawSnowflake(cx - size / 4, flakeY, flakeSize);
    drawSnowflake(cx + size / 5, flakeY + flakeSize, flakeSize - 1);
    drawSnowflake(cx, flakeY + flakeSize * 2, flakeSize - 1);
}

void DisplayManager::drawThunderIcon(int x, int y, int size) {
    drawCloudIcon(x, y - size/6, size * 0.75);

    // Elegant lightning bolt - zigzag shape
    int bx = x + size / 2;
    int by = y + size / 2 - 5;
    int boltWidth = size / 6;
    int boltHeight = size / 3;

    // Main bolt shape
    M5.Display.fillTriangle(
        bx - boltWidth / 2, by,
        bx + boltWidth, by + boltHeight / 2,
        bx, by + boltHeight / 2,
        TFT_BLACK
    );
    M5.Display.fillTriangle(
        bx + boltWidth / 2, by + boltHeight / 2 - 2,
        bx - boltWidth / 2, by + boltHeight,
        bx, by + boltHeight / 2 - 2,
        TFT_BLACK
    );

    // Bolt outline for definition
    M5.Display.drawLine(bx - boltWidth / 2, by, bx + boltWidth, by + boltHeight / 2, TFT_BLACK);
    M5.Display.drawLine(bx + boltWidth / 2, by + boltHeight / 2 - 2, bx - boltWidth / 2, by + boltHeight, TFT_BLACK);
}

void DisplayManager::drawFogIcon(int x, int y, int size) {
    int cy = y + size / 4;
    int lineSpacing = size / 5;

    // Wavy fog lines for more artistic look
    for (int i = 0; i < 4; i++) {
        int ly = cy + i * lineSpacing;
        int startX = x + (i % 2 == 0 ? 5 : 15);
        int endX = x + size - (i % 2 == 0 ? 15 : 5);

        // Draw wavy line using small segments
        for (int wx = startX; wx < endX - 5; wx += 3) {
            int waveOffset = (int)(sin((wx - startX) * 0.15) * 2);
            M5.Display.fillCircle(wx, ly + waveOffset, 2, TFT_BLACK);
        }
    }
}

void DisplayManager::drawPartlyCloudyIcon(int x, int y, int size, bool isNight) {
    // Sun/moon in background (smaller, offset to upper left)
    if (isNight) {
        // Simple moon for background
        int mx = x + size / 6;
        int my = y + size / 6;
        int mr = size / 5;
        M5.Display.fillCircle(mx, my, mr, TFT_BLACK);
        M5.Display.fillCircle(mx + mr * 0.5, my - mr * 0.2, mr * 0.8, TFT_WHITE);
    } else {
        // Simple sun for background
        int sx = x + size / 5;
        int sy = y + size / 5;
        int sr = size / 7;
        M5.Display.fillCircle(sx, sy, sr, TFT_BLACK);
        // A few rays peeking out
        for (int i = 0; i < 6; i++) {
            float angle = i * PI / 3 - PI / 6;
            int x1 = sx + cos(angle) * (sr + 2);
            int y1 = sy + sin(angle) * (sr + 2);
            int x2 = sx + cos(angle) * (sr + size / 10);
            int y2 = sy + sin(angle) * (sr + size / 10);
            M5.Display.drawLine(x1, y1, x2, y2, TFT_BLACK);
        }
    }

    // Cloud in foreground (offset down-right) - elegant fluffy cloud
    int cloudX = x + size / 3;
    int cloudY = y + size / 3;
    int r = size / 7;

    // White background to cleanly cover sun/moon
    M5.Display.fillCircle(cloudX, cloudY + r / 2, r + 4, TFT_WHITE);
    M5.Display.fillCircle(cloudX + r, cloudY - r / 4, r * 1.2 + 4, TFT_WHITE);
    M5.Display.fillCircle(cloudX + r * 2, cloudY + r / 2, r + 4, TFT_WHITE);
    M5.Display.fillRect(cloudX - r / 2, cloudY + r / 2, r * 3, r, TFT_WHITE);

    // Cloud outline for elegant look
    M5.Display.fillCircle(cloudX, cloudY + r / 2, r, TFT_BLACK);
    M5.Display.fillCircle(cloudX + r, cloudY - r / 4, r * 1.2, TFT_BLACK);
    M5.Display.fillCircle(cloudX + r * 2, cloudY + r / 2, r, TFT_BLACK);
    M5.Display.fillRect(cloudX, cloudY + r / 2, r * 2, r, TFT_BLACK);
}

// Helper function - draw a small star
void DisplayManager::drawStar(int x, int y, int size) {
    // Simple 4-point star
    M5.Display.drawLine(x - size, y, x + size, y, TFT_BLACK);
    M5.Display.drawLine(x, y - size, x, y + size, TFT_BLACK);
    // Diagonal lines for 8-point effect
    int d = size * 0.7;
    M5.Display.drawLine(x - d, y - d, x + d, y + d, TFT_BLACK);
    M5.Display.drawLine(x + d, y - d, x - d, y + d, TFT_BLACK);
}

// Helper function - draw an elegant snowflake
void DisplayManager::drawSnowflake(int x, int y, int size) {
    // 6-armed snowflake
    for (int i = 0; i < 6; i++) {
        float angle = i * PI / 3;
        int x2 = x + cos(angle) * size;
        int y2 = y + sin(angle) * size;
        M5.Display.drawLine(x, y, x2, y2, TFT_BLACK);

        // Small branches on each arm
        if (size > 3) {
            float branchAngle1 = angle + PI / 6;
            float branchAngle2 = angle - PI / 6;
            int bx = x + cos(angle) * size * 0.6;
            int by = y + sin(angle) * size * 0.6;
            int bLen = size * 0.4;
            M5.Display.drawLine(bx, by, bx + cos(branchAngle1) * bLen, by + sin(branchAngle1) * bLen, TFT_BLACK);
            M5.Display.drawLine(bx, by, bx + cos(branchAngle2) * bLen, by + sin(branchAngle2) * bLen, TFT_BLACK);
        }
    }
    // Center dot
    M5.Display.fillCircle(x, y, 1, TFT_BLACK);
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
