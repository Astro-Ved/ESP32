#include "Game.h"
#include "GUI.h"
#include "Jet.hpp"

using namespace Renderer;

#define GAME_W 160
#define GAME_H 160

uint16_t *colorBuf = nullptr;
uint16_t *depthBuf = nullptr;

Scene *scene = nullptr;
Camera *camera = nullptr;
Object *cube = nullptr;
DirectionalLight *sun = nullptr;
AmbientLight *amb = nullptr;
Material *redMat = nullptr;

void initGame() {
    // Buttons pin mapping
    pinMode(25, INPUT_PULLUP); // UP
    pinMode(32, INPUT_PULLUP); // DOWN
    pinMode(22, INPUT_PULLUP); // LEFT
    pinMode(0, INPUT_PULLUP); // RIGHT
    pinMode(17, INPUT_PULLUP); // A
    pinMode(21, INPUT_PULLUP); // B
    pinMode(4, INPUT_PULLUP); // START
    pinMode(16, INPUT_PULLUP); // SELECT

    // Allocate buffers
    colorBuf = (uint16_t*)heap_caps_malloc(GAME_W * GAME_H * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    depthBuf = (uint16_t*)heap_caps_malloc(ZBUFFER_STRIDE(GAME_W) * GAME_H * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (!colorBuf || !depthBuf) {
        Serial.println("Game buffer alloc failed!");
        return;
    }

    scene = new Scene(colorBuf, depthBuf, GAME_W, GAME_H);
    scene->setBackcolor(0x0000); // Black
    scene->setClearBuffer(true);

    camera = new Camera();
    camera->setPosition(0, 0, -500);
    camera->setFOV((int32_t)75, (int32_t)GAME_W);
    camera->nearPlane = 16;
    camera->farPlane = 8192;
    scene->setCamera(camera);

    sun = new DirectionalLight(Vector3{45, 35, 0}, Color{255, 245, 220}, 220);
    amb = new AmbientLight(Color{40, 48, 64});
    scene->setDirectionalLight(sun);
    scene->setAmbientLight(amb);

    redMat = new Material(0xF800); // Red
    redMat->shadingMode = ShadingMode::FLAT;

    cube = Primitives::createCube(200, 200, 200, redMat);
    cube->setPosition(0, 0, 200);
    scene->addObject(cube);
}

void processGameTask() {
    if (!colorBuf || !depthBuf || !scene) {
        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }

    // Handle inputs
    if (digitalRead(22) == LOW) cube->rotate(0, -5, 0); // Left
    if (digitalRead(0) == LOW) cube->rotate(0, 5, 0);  // Right
    if (digitalRead(25) == LOW) cube->rotate(-5, 0, 0); // Up
    if (digitalRead(32) == LOW) cube->rotate(5, 0, 0);  // Down
    if (digitalRead(17) == LOW) cube->rotate(0, 0, 5);  // A
    if (digitalRead(21) == LOW) cube->rotate(0, 0, -5); // B

    // Render frame
    scene->render();

    // Push to TFT
    xSemaphoreTake(tftMutex, portMAX_DELAY);
    // Draw centered on 320x240 screen, offset below the 30px tabs
    int startX = (320 - GAME_W) / 2;
    int startY = 30 + (210 - GAME_H) / 2;
    tft.pushImage(startX, startY, GAME_W, GAME_H, colorBuf);
    xSemaphoreGive(tftMutex);

    vTaskDelay(pdMS_TO_TICKS(16)); // ~60fps
}
