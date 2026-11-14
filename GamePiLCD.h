/*
 * GamePiLCD.h
 *
 * GamePi 1.54 / ST7789V 240x240 LCD driver
 * Raspberry Pi Zero 2 W + Linux
 *
 * Dependencies:
 *   sudo apt install g++ liblgpio-dev
 *
 * Build:
 *   g++ -std=c++17 -O2 -Wall -Wextra GamePiLCD.cpp -o gamepi_lcd
 *
 * Run:
 *   sudo ./gamepi_lcd
 */

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <lgpio.h>

class GamePiLCD
{
public:
    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 240;
    static constexpr int DC_PIN = 25;
    static constexpr int RESET_PIN = 27;

    explicit GamePiLCD(
        int spiBus = 0,
        int spiDevice = 0,
        uint32_t spiSpeedHz = 32'000'000);
    ~GamePiLCD();
    GamePiLCD(const GamePiLCD &) = delete;
    GamePiLCD &operator=(const GamePiLCD &) = delete;

    /*
     * Display an RGB888 image.
     *
     * rgb: interleaved RGB bytes: R,G,B,R,G,B,...
     * width, height: source image dimensions
     *
     * If the source size differs from 240x240, it is resized using
     * nearest-neighbor sampling.
     */
    void show(const uint8_t *rgb, int width, int height);

    // Fill the entire display with an RGB color.
    void fill(uint8_t red, uint8_t green, uint8_t blue);

    // Draw a square box with an RGB color.
    void box(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *rgb);

    // Draw a horizontal line
    void hline(uint16_t y, uint8_t *rgb);

    // Draw a vertical line
    void vline(uint16_t y, uint8_t *rgb);

    // Release SPI and GPIO resources. Safe to call more than once.
    void close() noexcept;

private:
    // ST7789 commands
    static constexpr uint8_t SWRESET = 0x01;
    static constexpr uint8_t SLPOUT = 0x11;
    static constexpr uint8_t COLMOD = 0x3A;
    static constexpr uint8_t MADCTL = 0x36;
    static constexpr uint8_t DGMEN = 0xBA;
    static constexpr uint8_t INVON = 0x21;
    static constexpr uint8_t NORON = 0x13;
    static constexpr uint8_t DISPON = 0x29;
    static constexpr uint8_t CASET = 0x2A;
    static constexpr uint8_t RASET = 0x2B;
    static constexpr uint8_t RAMWR = 0x2C;

    static constexpr size_t CHUNK_SIZE = 4096;

    int spiFd_ = -1;
    int gpioChip_ = -1;
    uint32_t spiSpeedHz_ = 32'000'000;
    bool dcClaimed_ = false;
    bool resetClaimed_ = false;
    bool closed_ = false;

    static void delayMs(int milliseconds);
    static void checkGpio(int result, const char *operation);
    static uint16_t getpixel(uint8_t *rgb);

    void configureSpi(uint32_t speedHz);
    void setSpiSpeed(uint32_t speedHz);
    void setDc(bool dataMode);
    void reset();
    void writeBytes(const uint8_t *data, size_t length);
    void command(uint8_t cmd,
                 const uint8_t *data = nullptr,
                 size_t length = 0);
    void command(uint8_t cmd, uint8_t value);
    void initializeDisplay();
    void setWindow(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1);
    void writeData(const uint8_t *data, size_t length);
};
