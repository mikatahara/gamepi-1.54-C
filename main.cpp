
#include "GamePiLCD.h"

// Example program
int main()
{
    try
    {
        GamePiLCD lcd;

        while (1)
        {

            lcd.fill(255, 0, 0); // Red
            std::this_thread::sleep_for(std::chrono::seconds(1));

            lcd.fill(0, 255, 0); // Green
            std::this_thread::sleep_for(std::chrono::seconds(1));

            lcd.fill(0, 0, 255); // Blue
            std::this_thread::sleep_for(std::chrono::seconds(1));

            lcd.fill(0, 0, 0);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // y = x
            uint8_t rgb[3] = {255, 0, 0}; // Red
            for (uint16_t x = 0; x < 240; x += 20)
            {
                lcd.box(x, x, 10, 10, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // y = -x
            rgb[0] = 0;
            rgb[1] = 255;
            rgb[2] = 0; // Green
            for (uint16_t x = 0; x < 240; x += 20)
            {
                lcd.box(x, 240 - x, 10, 10, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // y = 120
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 255; // Blue
            for (uint16_t x = 0; x < 240; x += 20)
            {
                lcd.box(x, 120, 10, 10, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // lcd.close() is also called automatically by the destructor.
            lcd.fill(0, 0, 0);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            rgb[0] = 255;
            rgb[1] = 0;
            rgb[2] = 0; // Red
            for (uint16_t y = 0; y < 80; y += 2)
            {
                lcd.hline(y, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            rgb[0] = 0;
            rgb[1] = 255;
            rgb[2] = 0; // Green
            for (uint16_t y = 80; y < 160; y += 2)
            {
                lcd.hline(y, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 255; // Blue
            for (uint16_t y = 160; y < 240; y += 2)
            {
                lcd.hline(y, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            lcd.fill(0, 0, 0);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            rgb[0] = 255;
            rgb[1] = 0;
            rgb[2] = 0; // Red
            for (uint16_t x = 0; x < 80; x += 2)
            {
                lcd.vline(x, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            rgb[0] = 0;
            rgb[1] = 255;
            rgb[2] = 0; // Green
            for (uint16_t x = 80; x < 160; x += 2)
            {
                lcd.vline(x, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 255; // Blue
            for (uint16_t x = 160; x < 240; x += 2)
            {
                lcd.vline(x, rgb);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            lcd.fill(0, 0, 0);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "GamePiLCD error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
