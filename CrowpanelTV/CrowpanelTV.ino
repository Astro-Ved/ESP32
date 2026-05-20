#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <driver/i2s.h>

// User provided pin configurations
const int PIN_UP     = 25;
const int PIN_DOWN   = 32;
const int PIN_LEFT   = 22;
const int PIN_RIGHT  = 0;
const int PIN_A      = 17;
const int PIN_B      = 21;
const int PIN_START  = 4;
const int PIN_SELECT = 16;

TFT_eSPI tft = TFT_eSPI();

// Map dimensions
#define MAP_WIDTH 8
#define MAP_HEIGHT 8
#define MAP_SIZE 64

// Display settings
// For performance, we render at half resolution
#define SCREEN_W 320
#define SCREEN_H 240
#define RENDER_W 160
#define RENDER_H 120

// Framebuffer to avoid flickering
uint16_t framebuffer[RENDER_W * RENDER_H];

// Simple map (1 = wall, 0 = empty space)
const uint8_t worldMap[MAP_WIDTH * MAP_HEIGHT] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 1, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 0, 1, 0, 1,
    1, 0, 0, 0, 1, 1, 0, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
};

// Player state
float posX = 3.5, posY = 3.5;  // Initial player position
float dirX = -1, dirY = 0;     // Initial direction vector
float planeX = 0, planeY = 0.66; // 2D raycaster camera plane

// Timing
unsigned long oldTime = 0, time_ms = 0;

// Color palette
const uint16_t colorWallN = TFT_RED;
const uint16_t colorWallE = TFT_MAROON;
const uint16_t colorWallS = TFT_BLUE;
const uint16_t colorWallW = TFT_NAVY;
const uint16_t colorFloor = TFT_DARKGREEN;
const uint16_t colorCeiling = TFT_SKYBLUE;

void playSound(int freq, int duration_ms) {
    // Stereo I2S, so we need 2 channels per sample
    int sample_rate = 44100;
    int samples_per_channel = (sample_rate * duration_ms) / 1000;
    int total_samples = samples_per_channel * 2; // Left and Right

    int16_t *buffer = (int16_t*)malloc(total_samples * sizeof(int16_t));
    if (!buffer) return;

    for (int i = 0; i < samples_per_channel; i++) {
        // Simple square wave
        int16_t val = ((i / (sample_rate / freq)) % 2) ? 1000 : -1000;
        buffer[i * 2] = val;     // Left
        buffer[i * 2 + 1] = val; // Right
    }

    size_t bytes_written;
    i2s_write(I2S_NUM_0, buffer, total_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    free(buffer);
}

void initAudio() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100,
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

    // Using DAC 2 (GPIO 26) since DAC 1 (GPIO 25) is PIN_UP.
    // Usually I2S_DAC_CHANNEL_LEFT_EN = GPIO 26, I2S_DAC_CHANNEL_RIGHT_EN = GPIO 25.
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
}

void setup() {
    Serial.begin(115200);

    // Initialize inputs (pullups are typically needed if connected to ground)
    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_A, INPUT_PULLUP);
    pinMode(PIN_B, INPUT_PULLUP);
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_SELECT, INPUT_PULLUP);

    // Backlight initialization as per config
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    tft.init();
    tft.setRotation(1); // Landscape mode 320x240
    tft.fillScreen(TFT_BLACK);

    initAudio();

    playSound(440, 200);
}

void drawFrame() {
    // Fill ceiling and floor
    for (int y = 0; y < RENDER_H / 2; y++) {
        for (int x = 0; x < RENDER_W; x++) {
            framebuffer[y * RENDER_W + x] = colorCeiling;
            framebuffer[(RENDER_H - 1 - y) * RENDER_W + x] = colorFloor;
        }
    }

    // Raycasting
    for (int x = 0; x < RENDER_W; x++) {
        float cameraX = 2 * x / (float)RENDER_W - 1; // x-coordinate in camera space
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        int mapX = (int)posX;
        int mapY = (int)posY;

        float sideDistX;
        float sideDistY;

        float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
        float perpWallDist;

        int stepX;
        int stepY;

        int hit = 0;
        int side; // 0 for NS or 1 for EW

        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                hit = 1; // Out of bounds, act as if it's a wall
            } else if (worldMap[mapY * MAP_WIDTH + mapX] > 0) {
                hit = 1;
            }
        }

        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else           perpWallDist = (sideDistY - deltaDistY);

        int lineHeight = (int)(RENDER_H / perpWallDist);

        int drawStart = -lineHeight / 2 + RENDER_H / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + RENDER_H / 2;
        if (drawEnd >= RENDER_H) drawEnd = RENDER_H - 1;

        uint16_t color;
        if (side == 0) {
            color = (rayDirX > 0) ? colorWallE : colorWallW;
        } else {
            color = (rayDirY > 0) ? colorWallS : colorWallN;
        }

        // Draw vertical line
        for (int y = drawStart; y < drawEnd; y++) {
            framebuffer[y * RENDER_W + x] = color;
        }
    }

    // Push framebuffer to TFT (upscaled 2x)
    tft.setAddrWindow(0, 0, SCREEN_W, SCREEN_H);
    for (int y = 0; y < RENDER_H; y++) {
        // We write each line twice, and each pixel twice
        uint16_t lineBuf[SCREEN_W];
        for(int x=0; x < RENDER_W; x++) {
            lineBuf[x*2] = framebuffer[y * RENDER_W + x];
            lineBuf[x*2+1] = framebuffer[y * RENDER_W + x];
        }
        tft.pushColors(lineBuf, SCREEN_W, true);
        tft.pushColors(lineBuf, SCREEN_W, true);
    }
}

void loop() {
    time_ms = millis();
    float frameTime = (time_ms - oldTime) / 1000.0; // frameTime is the time this frame has taken, in seconds
    oldTime = time_ms;

    float moveSpeed = frameTime * 3.0; // the constant value is in squares/second
    float rotSpeed = frameTime * 2.0; // the constant value is in radians/second

    bool up = !digitalRead(PIN_UP);
    bool down = !digitalRead(PIN_DOWN);
    bool left = !digitalRead(PIN_LEFT);
    bool right = !digitalRead(PIN_RIGHT);
    bool a_btn = !digitalRead(PIN_A);
    bool b_btn = !digitalRead(PIN_B);

    if (up) {
        float nextX = posX + dirX * moveSpeed;
        float nextY = posY + dirY * moveSpeed;

        if (nextX >= 0 && nextX < MAP_WIDTH && worldMap[(int)posY * MAP_WIDTH + (int)nextX] == false) {
            posX = nextX;
        }
        if (nextY >= 0 && nextY < MAP_HEIGHT && worldMap[(int)nextY * MAP_WIDTH + (int)posX] == false) {
            posY = nextY;
        }
    }
    if (down) {
        float nextX = posX - dirX * moveSpeed;
        float nextY = posY - dirY * moveSpeed;

        if (nextX >= 0 && nextX < MAP_WIDTH && worldMap[(int)posY * MAP_WIDTH + (int)nextX] == false) {
            posX = nextX;
        }
        if (nextY >= 0 && nextY < MAP_HEIGHT && worldMap[(int)nextY * MAP_WIDTH + (int)posX] == false) {
            posY = nextY;
        }
    }
    if (right) {
        float oldDirX = dirX;
        dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
        dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
        float oldPlaneX = planeX;
        planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
        planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
    }
    if (left) {
        float oldDirX = dirX;
        dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
        dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
        float oldPlaneX = planeX;
        planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
        planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
    }

    if (a_btn) {
        // Pew pew sound
        playSound(800, 50);
        delay(50);
    }

    drawFrame();
}
