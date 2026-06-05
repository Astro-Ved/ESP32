#!/bin/bash
set -e

# Download and install arduino-cli if not present
if [ ! -f "./bin/arduino-cli" ]; then
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
fi

# Configure arduino-cli and install dependencies
./bin/arduino-cli config init --overwrite
./bin/arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
./bin/arduino-cli core update-index
./bin/arduino-cli core install esp32:esp32
./bin/arduino-cli lib install "TFT_eSPI" "JPEGDEC"

# Install Jet if not present
if [ ! -d "libraries/Jet" ]; then
    mkdir -p libraries/Jet
    curl -fsSL https://github.com/CubeCoders/Jet/archive/refs/heads/main.zip -o main.zip
    unzip -q main.zip
    mv Jet-main/* libraries/Jet/
    rm -rf Jet-main main.zip
    # Configure Jet
    cp libraries/Jet/src/JetConfig.example.hpp libraries/Jet/src/JetConfig.hpp
    sed -i 's/#define HALF_WIDTH_BUFFERS 1/#define HALF_WIDTH_BUFFERS 0/g' libraries/Jet/src/JetConfig.hpp
    sed -i 's/#define FIELD_BUFFERS 1/#define FIELD_BUFFERS 0/g' libraries/Jet/src/JetConfig.hpp
    sed -i 's/#define Z_BUFFERING 0/#define Z_BUFFERING 1/g' libraries/Jet/src/JetConfig.hpp
fi

# Define the build flags exactly as they were in platformio.ini
BUILD_FLAGS="-DUSER_SETUP_LOADED=1 -DUSE_HSPI_PORT=1 -DILI9341_DRIVER=1 -DTFT_WIDTH=240 -DTFT_HEIGHT=320 -DTFT_MISO=12 -DTFT_MOSI=13 -DTFT_SCLK=14 -DTFT_CS=15 -DTFT_DC=2 -DTFT_RST=-1 -DTFT_BL=27 -DTOUCH_CS=33 -DLOAD_GLCD=1 -DLOAD_FONT2=1 -DLOAD_FONT4=1 -DLOAD_FONT6=1 -DLOAD_FONT7=1 -DLOAD_FONT8=1 -DLOAD_GFXFF=1 -DSMOOTH_FONT=1 -DSPI_FREQUENCY=15999999 -DSPI_READ_FREQUENCY=20000000 -DSPI_TOUCH_FREQUENCY=600000"

# Create build directory
mkdir -p build

# Compile the sketch and output binaries to the build directory
./bin/arduino-cli compile --fqbn esp32:esp32:esp32 --build-property "build.extra_flags=$BUILD_FLAGS" --libraries libraries --output-dir build CrowpanelTV
