/*
 * GamePiLCD.cpp
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
#include "GamePiLCD.h"

GamePiLCD::GamePiLCD(
    int spiBus,
    int spiDevice,
    uint32_t spiSpeedHz) : spiSpeedHz_(spiSpeedHz)
{
    const std::string device =
        "/dev/spidev" + std::to_string(spiBus) + "." +
        std::to_string(spiDevice);

    spiFd_ = ::open(device.c_str(), O_RDWR);
    if (spiFd_ < 0)
    {
        throw std::runtime_error(
            "Cannot open " + device + ": " + std::strerror(errno));
    }

    try
    {
        configureSpi(spiSpeedHz_);
        gpioChip_ = lgGpiochipOpen(0);
        if (gpioChip_ < 0)
        {
            throw std::runtime_error("lgGpiochipOpen failed: " +
                                     std::to_string(gpioChip_));
        }

        checkGpio(lgGpioClaimOutput(gpioChip_, 0, DC_PIN, 0),
                  "claim DC GPIO");
        dcClaimed_ = true;

        checkGpio(lgGpioClaimOutput(gpioChip_, 0, RESET_PIN, 1),
                  "claim RESET GPIO");
        resetClaimed_ = true;

        reset();
        initializeDisplay();
    }
    catch (...)
    {
        close();
        throw;
    }
}

GamePiLCD::~GamePiLCD()
{
    close();
}

// GamePiLCD(const GamePiLCD&) = delete;
// GamePiLCD& operator=(const GamePiLCD&) = delete;

/*
 * Display an RGB888 image.
 *
 * rgb: interleaved RGB bytes: R,G,B,R,G,B,...
 * width, height: source image dimensions
 *
 * If the source size differs from 240x240, it is resized using
 * nearest-neighbor sampling.
 */
void GamePiLCD::show(const uint8_t *rgb, int width, int height)
{
    if (closed_)
    {
        throw std::runtime_error("GamePiLCD is closed");
    }
    if (rgb == nullptr || width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Invalid RGB image");
    }

    std::vector<uint8_t> frame(WIDTH * HEIGHT * 2);
    size_t pos = 0;

    for (int y = 0; y < HEIGHT; ++y)
    {
        const int srcY = y * height / HEIGHT;
        for (int x = 0; x < WIDTH; ++x)
        {
            const int srcX = x * width / WIDTH;
            const size_t src =
                (static_cast<size_t>(srcY) * width + srcX) * 3;

            const uint8_t r = rgb[src];
            const uint8_t g = rgb[src + 1];
            const uint8_t b = rgb[src + 2];

            const uint16_t pixel =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                (b >> 3);

            // ST7789 receives RGB565, high byte first.
            frame[pos++] = static_cast<uint8_t>(pixel >> 8);
            frame[pos++] = static_cast<uint8_t>(pixel & 0xFF);
        }
    }

    setWindow(0, 0, WIDTH - 1, HEIGHT - 1);
    writeData(frame.data(), frame.size());
}

// Fill the entire display with an RGB color.
void GamePiLCD::fill(uint8_t red, uint8_t green, uint8_t blue)
{
    if (closed_)
    {
        throw std::runtime_error("GamePiLCD is closed");
    }

    const uint16_t pixel =
        ((red & 0xF8) << 8) |
        ((green & 0xFC) << 3) |
        (blue >> 3);

    std::vector<uint8_t> frame(WIDTH * HEIGHT * 2);
    for (size_t i = 0; i < frame.size(); i += 2)
    {
        frame[i] = static_cast<uint8_t>(pixel >> 8);
        frame[i + 1] = static_cast<uint8_t>(pixel & 0xFF);
    }

    setWindow(0, 0, WIDTH - 1, HEIGHT - 1);
    writeData(frame.data(), frame.size());
}

// Draw a square box with an RGB color.
void GamePiLCD::box(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *rgb)
{

    const uint16_t pixel = getpixel(rgb);
    std::vector<uint8_t> frame(width * height * 2);

    for (size_t i = 0; i < frame.size(); i += 2)
    {
        frame[i] = static_cast<uint8_t>(pixel >> 8);
        frame[i + 1] = static_cast<uint8_t>(pixel & 0xFF);
    }

    // Drawing area
    uint16_t x1 = x + width - 1;
    uint16_t y1 = y + height - 1;

    setWindow(x, y, x1, y1);
    writeData(frame.data(), frame.size());
}


// Draw a horizontal line
void GamePiLCD::hline(uint16_t y, uint8_t *rgb)
{
    const uint16_t pixel = getpixel(rgb);
    std::vector<uint8_t> frame(WIDTH * 1 * 2);

    for (size_t i = 0; i < frame.size(); i += 2)
    {
        frame[i] = static_cast<uint8_t>(pixel >> 8);
        frame[i + 1] = static_cast<uint8_t>(pixel & 0xFF);
    }

    setWindow(0, y, WIDTH-1, y);
    writeData(frame.data(), frame.size());
}

// Draw a vertical line
void GamePiLCD::vline(uint16_t x, uint8_t *rgb)
{
    const uint16_t pixel = getpixel(rgb);
    std::vector<uint8_t> frame(1 * HEIGHT * 2);
    for (size_t i = 0; i < frame.size(); i += 2)
    {
        frame[i] = static_cast<uint8_t>(pixel >> 8);
        frame[i + 1] = static_cast<uint8_t>(pixel & 0xFF);
    }

    setWindow(x, 0, x, HEIGHT - 1);
    writeData(frame.data(), frame.size());
}

/*    def vline(self, x, color):
        """Draw a vertical line."""
        r, g, b = color
        value = (
            ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        )
        pixel = bytes([(value >> 8) & 0xff,value & 0xff])
        height = self.HEIGHT - 80
        # 1 pixel × 240 lines in height
        data = pixel * height

        self._set_window(x, 80, x, self.HEIGHT - 1)
        self.spi.writebytes(data)
*/

// Release SPI and GPIO resources. Safe to call more than once.
void GamePiLCD::close() noexcept
{
    if (closed_)
    {
        return;
    }

    if (gpioChip_ >= 0)
    {
        if (resetClaimed_)
        {
            lgGpioFree(gpioChip_, RESET_PIN);
            resetClaimed_ = false;
        }
        if (dcClaimed_)
        {
            lgGpioFree(gpioChip_, DC_PIN);
            dcClaimed_ = false;
        }
        lgGpiochipClose(gpioChip_);
        gpioChip_ = -1;
    }

    if (spiFd_ >= 0)
    {
        ::close(spiFd_);
        spiFd_ = -1;
    }

    closed_ = true;
}

void GamePiLCD::delayMs(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(milliseconds));
}

void GamePiLCD::checkGpio(int result, const char *operation)
{
    if (result < 0)
    {
        throw std::runtime_error(
            std::string(operation) + " failed: " +
            std::to_string(result));
    }
}

uint16_t GamePiLCD::getpixel(uint8_t *rgb){
    const uint8_t r = rgb[0];
    const uint8_t g = rgb[1];
    const uint8_t b = rgb[2];

    uint16_t pixel =
        ((r & 0xF8) << 8) |
        ((g & 0xFC) << 3) |
        (b >> 3);
    return pixel;
}

void GamePiLCD::configureSpi(uint32_t speedHz)
{
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;

    if (::ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0)
    {
        throw std::runtime_error("SPI mode setup failed");
    }
    if (::ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
    {
        throw std::runtime_error("SPI bits-per-word setup failed");
    }
    if (::ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz) < 0)
    {
        throw std::runtime_error("SPI speed setup failed");
    }
}

void GamePiLCD::setSpiSpeed(uint32_t speedHz)
{
    if (::ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz) < 0)
    {
        throw std::runtime_error("SPI speed change failed");
    }
}

void GamePiLCD::setDc(bool dataMode)
{
    const int result =
        lgGpioWrite(gpioChip_, DC_PIN, dataMode ? 1 : 0);
    checkGpio(result, "write DC GPIO");
}

void GamePiLCD::reset()
{
    checkGpio(lgGpioWrite(gpioChip_, RESET_PIN, 1), "RESET high");
    delayMs(120);

    checkGpio(lgGpioWrite(gpioChip_, RESET_PIN, 0), "RESET low");
    delayMs(120);

    checkGpio(lgGpioWrite(gpioChip_, RESET_PIN, 1), "RESET high");
    delayMs(120);
}

void GamePiLCD::writeBytes(const uint8_t *data, size_t length)
{
    size_t offset = 0;
    while (offset < length)
    {
        const size_t amount =
            std::min(CHUNK_SIZE, length - offset);

        const ssize_t written =
            ::write(spiFd_, data + offset, amount);

        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            throw std::runtime_error(
                std::string("SPI write failed: ") +
                std::strerror(errno));
        }
        if (written == 0)
        {
            throw std::runtime_error("SPI write returned zero");
        }
        offset += static_cast<size_t>(written);
    }
}

void GamePiLCD::command(uint8_t cmd,
                        const uint8_t *data,
                        size_t length)
{
    setDc(false);
    writeBytes(&cmd, 1);

    if (data != nullptr && length > 0)
    {
        setDc(true);
        writeBytes(data, length);
    }
}

void GamePiLCD::command(uint8_t cmd, uint8_t value)
{
    command(cmd, &value, 1);
}

void GamePiLCD::initializeDisplay()
{
    const uint32_t oldSpeed = spiSpeedHz_;
    setSpiSpeed(std::min(oldSpeed, 1'000'000U));

    command(SWRESET);
    delayMs(120);

    command(SLPOUT);
    delayMs(120);

    command(0x26, 0x04);   // Gamma Curve Select
    command(COLMOD, 0x05); // 16-bit pixel format
    delayMs(20);

    command(MADCTL, 0xC0);
    delayMs(10);

    command(DGMEN, 0x04);
    command(INVON);
    command(NORON);
    delayMs(10);

    command(DISPON);
    delayMs(100);

    setSpiSpeed(oldSpeed);
    fill(0, 0, 0);
}

void GamePiLCD::setWindow(uint16_t x0, uint16_t y0,
                          uint16_t x1, uint16_t y1)
{
    y0 = y0 + 80;
    y1 = y1 + 80;
    const std::array<uint8_t, 4> xData = {
        static_cast<uint8_t>(x0 >> 8),
        static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8),
        static_cast<uint8_t>(x1 & 0xFF)};
    command(CASET, xData.data(), xData.size());

    const std::array<uint8_t, 4> yData = {
        static_cast<uint8_t>(y0 >> 8),
        static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8),
        static_cast<uint8_t>(y1 & 0xFF)};
    command(RASET, yData.data(), yData.size());

    setDc(false);
    const uint8_t ramWrite = RAMWR;
    writeBytes(&ramWrite, 1);
    setDc(true);
}

void GamePiLCD::writeData(const uint8_t *data, size_t length)
{
    writeBytes(data, length);
}
