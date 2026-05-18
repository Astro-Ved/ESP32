#include "Config.h"
#include <Preferences.h>

Preferences prefs;
PinConfig currentPinConfig;

void initConfig() {
    prefs.begin("tv_config", false);

    currentPinConfig.sd_miso = prefs.getInt("sd_miso", DEFAULT_SD_MISO);
    currentPinConfig.sd_mosi = prefs.getInt("sd_mosi", DEFAULT_SD_MOSI);
    currentPinConfig.sd_sclk = prefs.getInt("sd_sclk", DEFAULT_SD_SCLK);
    currentPinConfig.sd_cs = prefs.getInt("sd_cs", DEFAULT_SD_CS);

    currentPinConfig.i2s_bclk = prefs.getInt("i2s_bclk", DEFAULT_I2S_BCLK);
    currentPinConfig.i2s_lrc = prefs.getInt("i2s_lrc", DEFAULT_I2S_LRC);
    currentPinConfig.i2s_dout = prefs.getInt("i2s_dout", DEFAULT_I2S_DOUT);
}

void saveConfig() {
    prefs.putInt("sd_miso", currentPinConfig.sd_miso);
    prefs.putInt("sd_mosi", currentPinConfig.sd_mosi);
    prefs.putInt("sd_sclk", currentPinConfig.sd_sclk);
    prefs.putInt("sd_cs", currentPinConfig.sd_cs);

    prefs.putInt("i2s_bclk", currentPinConfig.i2s_bclk);
    prefs.putInt("i2s_lrc", currentPinConfig.i2s_lrc);
    prefs.putInt("i2s_dout", currentPinConfig.i2s_dout);
}

void printConfig() {
    Serial.println("--- Current Pin Mapping ---");
    Serial.println("Display & Touch pins are fixed via HSPI in platformio.ini");
    Serial.println("You can change SD and I2S pins via commands like: set_pin sd_cs 5");
    Serial.println("SD VSPI Pins:");
    Serial.printf("  sd_miso: %d\n", currentPinConfig.sd_miso);
    Serial.printf("  sd_mosi: %d\n", currentPinConfig.sd_mosi);
    Serial.printf("  sd_sclk: %d\n", currentPinConfig.sd_sclk);
    Serial.printf("  sd_cs: %d\n", currentPinConfig.sd_cs);
    Serial.println("I2S DAC Pins:");
    Serial.printf("  i2s_bclk: %d\n", currentPinConfig.i2s_bclk);
    Serial.printf("  i2s_lrc: %d\n", currentPinConfig.i2s_lrc);
    Serial.printf("  i2s_dout: %d\n", currentPinConfig.i2s_dout);
    Serial.println("---------------------------");
}

bool updatePinConfig(const String& param, int pin) {
    bool found = true;
    if (param == "sd_miso") currentPinConfig.sd_miso = pin;
    else if (param == "sd_mosi") currentPinConfig.sd_mosi = pin;
    else if (param == "sd_sclk") currentPinConfig.sd_sclk = pin;
    else if (param == "sd_cs") currentPinConfig.sd_cs = pin;
    else if (param == "i2s_bclk") currentPinConfig.i2s_bclk = pin;
    else if (param == "i2s_lrc") currentPinConfig.i2s_lrc = pin;
    else if (param == "i2s_dout") currentPinConfig.i2s_dout = pin;
    else found = false;

    if (found) {
        saveConfig();
        return true;
    }
    return false;
}
