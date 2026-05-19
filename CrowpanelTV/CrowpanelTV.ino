#include <Arduino.h>
#include "Config.h"
#include "GUI.h"
#include "Media.h"

// Task handles
TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;

void GUI_Task(void *pvParameters);

void setup() {
    Serial.begin(115200);

    // Initialize TFT Mutex before any tasks start
    tftMutex = xSemaphoreCreateMutex();

    // Forcefully turn on Backlight as requested
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    // Initialize NVS and load pin map
    initConfig();

    // Print current configuration
    printConfig();

    // Core 0 Task: Media Decoding and Audio
    xTaskCreatePinnedToCore(
        processMediaTask,   // Function to implement the task
        "TaskCore0",        // Name of the task
        10000,              // Stack size in words
        NULL,               // Task input parameter
        1,                  // Priority of the task
        &TaskCore0,         // Task handle
        0                   // Core where the task should run
    );

    // Core 1 Task: GUI and Serial Monitor
    xTaskCreatePinnedToCore(
        GUI_Task,           // Function to implement the task
        "TaskCore1",        // Name of the task
        10000,              // Stack size in words
        NULL,               // Task input parameter
        1,                  // Priority of the task
        &TaskCore1,         // Task handle
        1                   // Core where the task should run
    );
}

void loop() {
    // Empty, loop is handled by FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void GUI_Task(void *pvParameters) {
    initGUI();

    String serialCommand = "";

    while(1) {
        // Update async GUI components (like WiFi scan results)
        updateGUI();

        // Handle touch inputs for GUI navigation
        handleTouch();

        // Handle Serial Monitor CLI for pin mapping updates
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\n') {
                serialCommand.trim();

                if (serialCommand.startsWith("set_pin ")) {
                    int spaceIdx = serialCommand.lastIndexOf(' ');
                    if (spaceIdx > 0) {
                        String pinName = serialCommand.substring(8, spaceIdx);
                        int pinNum = serialCommand.substring(spaceIdx + 1).toInt();

                        if (updatePinConfig(pinName, pinNum)) {
                            Serial.printf("Successfully updated %s to pin %d\n", pinName.c_str(), pinNum);
                        } else {
                            Serial.println("Failed to update: invalid pin name.");
                        }
                    }
                } else if (serialCommand == "print_map") {
                    printConfig();
                } else if (serialCommand == "reboot") {
                    Serial.println("Rebooting...");
                    ESP.restart();
                }

                serialCommand = "";
            } else {
                serialCommand += c;
            }
        }

        // Delay to prevent task watchdog from triggering
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
