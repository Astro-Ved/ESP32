#ifndef GUI_H
#define GUI_H

#include <TFT_eSPI.h>

enum Tab {
    TAB_MEDIA,
    TAB_WIFI,
    TAB_CONFIG,
    TAB_GAME
};

extern TFT_eSPI tft;
extern Tab currentTab;
extern SemaphoreHandle_t tftMutex;

void initGUI();
void drawTabs();
void drawTabContent();
void handleTouch();
void updateGUI();

void drawMediaTab();
void drawWiFiTab();
void drawConfigTab();
void drawGameTab();

#endif // GUI_H
