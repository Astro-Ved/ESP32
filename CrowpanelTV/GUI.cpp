#include "GUI.h"
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();
Tab currentTab = TAB_MEDIA;
SemaphoreHandle_t tftMutex;
bool redrawContent = true;
bool wifiScanning = false;
unsigned long lastWifiUpdate = 0;

// Tab positions
const int tabWidth = 80;
const int tabHeight = 30;

void initGUI() {
    tftMutex = xSemaphoreCreateMutex();

    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.init();
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);

    // Calibration for touch - you might need to adjust or run a calibration sketch
    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
    tft.setTouch(calData);
    xSemaphoreGive(tftMutex);

    drawTabs();
    drawTabContent();
}

void drawTabs() {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.fillRect(0, 0, tft.width(), tabHeight, TFT_DARKGREY);

    int dynamicTabWidth = tft.width() / 4;

    for (int i = 0; i < 4; i++) {
        int x = i * dynamicTabWidth;
        if (i == currentTab) {
            tft.fillRect(x, 0, dynamicTabWidth, tabHeight, TFT_BLUE);
            tft.setTextColor(TFT_WHITE, TFT_BLUE);
        } else {
            tft.drawRect(x, 0, dynamicTabWidth, tabHeight, TFT_LIGHTGREY);
            tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
        }

        tft.setCursor(x + 5, 8);
        if (i == 0) tft.print("Media");
        else if (i == 1) tft.print("WiFi");
        else if (i == 2) tft.print("Config");
        else if (i == 3) tft.print("Game");
    }
    xSemaphoreGive(tftMutex);
}

void drawTabContent() {
    if (!redrawContent) return;

    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.fillRect(0, tabHeight, tft.width(), tft.height() - tabHeight, TFT_BLACK); // Clear content area
    xSemaphoreGive(tftMutex);

    switch (currentTab) {
        case TAB_MEDIA: drawMediaTab(); break;
        case TAB_WIFI: drawWiFiTab(); break;
        case TAB_CONFIG: drawConfigTab(); break;
        case TAB_GAME: drawGameTab(); break;
    }

    redrawContent = false;
}

void drawMediaTab() {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, tabHeight + 20);
    tft.println("Media Player");
    tft.setCursor(10, tabHeight + 40);
    tft.println("Select a file from SD");
    xSemaphoreGive(tftMutex);
    // Placeholder for actual file listing
}

void drawWiFiTab() {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, tabHeight + 10);

    if (!wifiScanning) {
        tft.println("Starting WiFi scan...");
        xSemaphoreGive(tftMutex); // Give early before async task start
        WiFi.scanNetworks(true); // async
        wifiScanning = true;
    } else {
        tft.println("Scanning WiFi...");
        xSemaphoreGive(tftMutex);
    }
}

void updateGUI() {
    if (currentTab == TAB_WIFI && wifiScanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            wifiScanning = false;

            xSemaphoreTake(tftMutex, portMAX_DELAY);
            tft.fillRect(0, tabHeight, tft.width(), tft.height() - tabHeight, TFT_BLACK); // Clear "Scanning..."
            tft.setCursor(10, tabHeight + 10);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);

            if (n == 0) {
                tft.println("No networks found.");
            } else {
                tft.printf("%d networks found:\n", n);
                for (int i = 0; i < n && i < 10; ++i) { // Show up to 10
                    tft.setCursor(10, tabHeight + 30 + (i * 15));
                    tft.printf("%d: %s (%d)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
                }
            }
            xSemaphoreGive(tftMutex);
            WiFi.scanDelete();
        } else if (n == WIFI_SCAN_FAILED) {
             wifiScanning = false;
             xSemaphoreTake(tftMutex, portMAX_DELAY);
             tft.fillRect(0, tabHeight, tft.width(), tft.height() - tabHeight, TFT_BLACK);
             tft.setCursor(10, tabHeight + 10);
             tft.println("WiFi Scan Failed.");
             xSemaphoreGive(tftMutex);
        }
    }
}

void drawConfigTab() {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, tabHeight + 20);
    tft.println("System Config");
    tft.setCursor(10, tabHeight + 40);
    tft.println("Use Serial Monitor to");
    tft.setCursor(10, tabHeight + 55);
    tft.println("update pin map.");
    xSemaphoreGive(tftMutex);
}

void drawGameTab() {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, tabHeight + 20);
    tft.println("Game Tab");
    tft.setCursor(10, tabHeight + 40);
    tft.println("Loading Jet Renderer...");
    xSemaphoreGive(tftMutex);
}

void handleTouch() {
    uint16_t x, y;
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    bool pressed = tft.getTouch(&x, &y);
    xSemaphoreGive(tftMutex);

    if (pressed) {
        // Map Y coordinate check for tabs (top area)
        if (y < tabHeight) {
            xSemaphoreTake(tftMutex, portMAX_DELAY);
            int dynamicTabWidth = tft.width() / 4;
            xSemaphoreGive(tftMutex);

            int newTab = x / dynamicTabWidth;
            if (newTab >= 0 && newTab < 4 && newTab != currentTab) {
                currentTab = (Tab)newTab;
                redrawContent = true;
                drawTabs();
                drawTabContent();
            }
        }
    }
}
