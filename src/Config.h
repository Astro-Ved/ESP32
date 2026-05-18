#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Default Pins
#define DEFAULT_SD_MISO 19
#define DEFAULT_SD_MOSI 23
#define DEFAULT_SD_SCLK 18
#define DEFAULT_SD_CS   5

#define DEFAULT_I2S_BCLK 26
#define DEFAULT_I2S_LRC  25
#define DEFAULT_I2S_DOUT 22

struct PinConfig {
    int sd_miso;
    int sd_mosi;
    int sd_sclk;
    int sd_cs;
    int i2s_bclk;
    int i2s_lrc;
    int i2s_dout;
};

extern PinConfig currentPinConfig;

void initConfig();
void saveConfig();
void printConfig();
bool updatePinConfig(const String& param, int pin);

#endif // CONFIG_H
