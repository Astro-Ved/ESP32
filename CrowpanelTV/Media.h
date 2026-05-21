#ifndef MEDIA_H
#define MEDIA_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDEC.h>

extern SPIClass vspi;
extern JPEGDEC jpeg;

void initMedia();
void initAudio();
void playMusic(const char *filename);
void processMediaTask(void *pvParameters);

int JPEGDraw(JPEGDRAW *pDraw);

#endif // MEDIA_H
