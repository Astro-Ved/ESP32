#include "Media.h"
#include "Config.h"
#include "GUI.h" // For tft access
#include "driver/i2s.h"

SPIClass vspi(VSPI);
JPEGDEC jpeg;

bool sdMounted = false;

// Callback for JPEG drawing
int JPEGDraw(JPEGDRAW *pDraw) {
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    tft.pushImage(pDraw->x, pDraw->y + 30, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    xSemaphoreGive(tftMutex);
    return 1; // 1 to continue decoding
}

void playRGB(const char *filename) {
    File f = SD.open(filename, FILE_READ);
    if (!f) return;

    xSemaphoreTake(tftMutex, portMAX_DELAY);
    uint16_t w = tft.width();
    uint16_t h = tft.height() - 30;
    xSemaphoreGive(tftMutex);

    uint16_t *buf = (uint16_t*)malloc(w * 10 * 2); // 10 lines buffer

    if (buf) {
        xSemaphoreTake(tftMutex, portMAX_DELAY);
        tft.setAddrWindow(0, 30, w, h);
        xSemaphoreGive(tftMutex);

        for (int y = 0; y < h; y += 10) {
            f.read((uint8_t*)buf, w * 10 * 2);
            xSemaphoreTake(tftMutex, portMAX_DELAY);
            tft.pushColors(buf, w * 10, true);
            xSemaphoreGive(tftMutex);
        }
        free(buf);
    }
    f.close();
}

void playMJPEG(const char *filename) {
    File f = SD.open(filename, FILE_READ);
    if (!f) return;

    uint8_t *buf = (uint8_t*)malloc(1024 * 30); // 30KB frame buffer assumption
    if (!buf) {
        f.close();
        return;
    }

    // Chunked MJPEG stream parsing for better performance
    bool inFrame = false;
    size_t frameSize = 0;
    uint8_t prevByte = 0;

    const size_t CHUNK_SIZE = 512;
    uint8_t chunk[CHUNK_SIZE];

    while (f.available()) {
        if (currentTab != TAB_MEDIA) break; // Exit if tab changed

        int bytesRead = f.read(chunk, CHUNK_SIZE);
        if (bytesRead <= 0) break;

        for (int i = 0; i < bytesRead; i++) {
            uint8_t b = chunk[i];

            if (!inFrame && prevByte == 0xFF && b == 0xD8) {
                inFrame = true;
                frameSize = 0;
                buf[frameSize++] = 0xFF;
                buf[frameSize++] = 0xD8;
            } else if (inFrame) {
                buf[frameSize++] = b;
                if (prevByte == 0xFF && b == 0xD9) {
                    // End of frame, decode it
                    if (jpeg.openRAM(buf, frameSize, JPEGDraw)) {
                        jpeg.setPixelType(RGB565_BIG_ENDIAN);
                        jpeg.decode(0, 0, 0);
                        jpeg.close();
                    }

                    // Play a generic beep for demonstration of audio running concurrently
                    int16_t samples[64];
                    for (int s=0; s<64; s++) samples[s] = (s%2) ? 1000 : -1000;
                    size_t bytes_written = 0;
                    i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);

                    inFrame = false;
                }
                // Protect buffer overflow
                if (frameSize >= 1024 * 30) inFrame = false;
            }
            prevByte = b;
        }
    }

    free(buf);
    f.close();
}

void initMedia() {
    vspi.begin(currentPinConfig.sd_sclk, currentPinConfig.sd_miso, currentPinConfig.sd_mosi, currentPinConfig.sd_cs);

    if (!SD.begin(currentPinConfig.sd_cs, vspi)) {
        Serial.println("Card Mount Failed");
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }

    sdMounted = true;
    Serial.println("SD Card Initialized.");
}

void initAudio() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100, // standard sample rate, adjust as needed
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = true,
        .tx_desc_auto_clear = true
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, NULL); // NULL for internal DAC

    // Enable I2S DAC channels
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);

    Serial.println("Audio Initialized (Internal DAC with Oversampling setup).");
}

void processMediaTask(void *pvParameters) {
    initMedia();
    initAudio();

    while(1) {
        if (currentTab == TAB_MEDIA && sdMounted) {
            // Check if user requested to play a file (hardcoded logic for demo)
            // In a real GUI, this would be selected via touch
            if (SD.exists("/video.mjpeg")) {
                playMJPEG("/video.mjpeg");
            } else if (SD.exists("/image.rgb")) {
                playRGB("/image.rgb");
            } else {
                 vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            // Delay to allow other tasks to run
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
