#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

static const char *TAG = "SAMPLE_APP";

void app_main(void) {
    ESP_LOGI(TAG, "Sample Application Running!");

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s", running->label);

    ESP_LOGI(TAG, "Doing some app logic for 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "App exiting, rebooting back to factory partition...");

    // Find factory partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition != NULL) {
        esp_err_t err = esp_ota_set_boot_partition(factory_partition);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Boot partition set to factory. Restarting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "Failed to set boot partition (%s)", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Factory partition not found!");
    }

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
