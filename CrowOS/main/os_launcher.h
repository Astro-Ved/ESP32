#ifndef OS_LAUNCHER_H
#define OS_LAUNCHER_H

#include "esp_err.h"

esp_err_t os_launcher_init(void);
esp_err_t os_launcher_flash_app(const char* filepath);
void os_launcher_run(void);

#endif
