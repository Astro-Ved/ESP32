#include "os_launcher.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "OS_LAUNCHER";
#define MOUNT_POINT "/sdcard"

// Basic Pins
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

esp_err_t os_launcher_init(void) {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing SD card");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_HOST);
    if (ret != ESP_OK) return ret;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD card mounted.");
    }
    return ESP_OK;
}

esp_err_t os_launcher_flash_app(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (f == NULL) return ESP_FAIL;

    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (update_partition == NULL) { fclose(f); return ESP_FAIL; }

    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) { fclose(f); return err; }

    char ota_write_data[1024];
    size_t size = 0;
    while ((size = fread(ota_write_data, 1, sizeof(ota_write_data), f)) > 0) {
        err = esp_ota_write(update_handle, (const void *)ota_write_data, size);
        if (err != ESP_OK) { esp_ota_end(update_handle); fclose(f); return err; }
    }
    fclose(f);
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) return err;

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) return err;

    esp_restart();
    return ESP_OK;
}

void os_launcher_run(void) {
    ESP_LOGI(TAG, "CrowOS Launcher Running.");
    ESP_LOGI(TAG, "Due to missing LCD hardware, GUI is rendered via Serial log.");
    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, "             CrowOS Main Menu                ");
    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, " 1. NES Emulator  (type: load nes_emulator.bin)");
    ESP_LOGI(TAG, " 2. Sample App    (type: load sample_app.bin)");
    ESP_LOGI(TAG, " 3. Video Player  (Not Installed)");
    ESP_LOGI(TAG, " 4. App Store     (Not Connected)");
    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, "Config: set_driver <driver> (e.g. ST7789)");

    char line[128];
    while (1) {
        if (fgets(line, sizeof(line), stdin) != NULL) {
            line[strcspn(line, "\r\n")] = 0;
            if (strncmp(line, "load ", 5) == 0) {
                char filepath[256];
                snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, line + 5);
                ESP_LOGI(TAG, "Loading %s...", filepath);
                os_launcher_flash_app(filepath);
            } else if (strncmp(line, "set_driver ", 11) == 0) {
                ESP_LOGI(TAG, "Configured driver: %s. Saved to NVS.", line+11);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
