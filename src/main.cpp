/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"

#include <cstdio>
#include <algorithm>

#include "pico/stdlib.h"
#include "lcd_extra.h"

// dbScale (max - min <= 40, otherwise lowest scale has no meaning against 12bit ADC resolution)
//static const std::vector<float> dbScale{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};  // default
static const std::vector<float> dbScale{-30, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const int NUM_LEVELS = dbScale.size();
static int greenTh;
static int redTh;

static bool peakHoldFlag = true;

void prepareLevel()
{
    {
        auto it = std::upper_bound(dbScale.cbegin(), dbScale.cend(), 0);
        greenTh = std::distance(dbScale.cbegin(), it);
    }
    {
        auto it = std::upper_bound(dbScale.cbegin(), dbScale.cend(), 5);
        redTh = std::distance(dbScale.cbegin(), it);
    }
}

void drawLevelMeter(int ch, int level, int peakHold = -1)
{
    const u16 Y_CH_HEIGHT = 10;
    const u16 Y_OFFSET = LCD_H() / 2 - Y_CH_HEIGHT;
    const u16 WIDTH = LCD_W() / NUM_LEVELS;
    const u16 X_OFFSET = (LCD_W() -  WIDTH * NUM_LEVELS) / 2;

    for (int i = 0; i < NUM_LEVELS; i++) {
        if (i == 0 || i < level || i == peakHold) {  // level0 is always on
            u16 color = (i < greenTh) ? GREEN : (i < redTh) ? BRRED : RED;
            LCD_Fill(WIDTH*i + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch, WIDTH*i + WIDTH-2 + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch + 5, color);
        } else {
            LCD_Fill(WIDTH*i + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch, WIDTH*i + WIDTH-2 + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch + 5, DARKGRAY);
        }
    }
}

int main()
{
    stdio_init_all();

    pico_st7735_80x160_config_t lcd_cfg = {
        SPI_CLK_FREQ_DEFAULT,
        spi1,
        PIN_LCD_SPI1_CS_WAVESHARE,
        PIN_LCD_SPI1_SCK_WAVESHARE,
        PIN_LCD_SPI1_MOSI_WAVESHARE,
        PIN_LCD_DC_WAVESHARE,
        PIN_LCD_RST_WAVESHARE,
        PIN_LCD_BLK_WAVESHARE,
        PWM_BLK_DEFAULT,
        INVERSION_DEFAULT,  // 0: non-color-inversion, 1: color-inversion
        RGB_ORDER_DEFAULT,  // 0: RGB, 1: BGR
        ROTATION_DEFAULT,
        H_OFS_DEFAULT,
        V_OFS_DEFAULT,
        X_MIRROR_DEFAULT
    };

    LCD_Config(&lcd_cfg);
    LCD_Init();
    LCD_SetRotation(1);
    LCD_Clear(BLACK);
    BACK_COLOR=BLACK;


    // level meter
    int level[NUM_ADC_CH];
    int peakHold[NUM_ADC_CH];
    level_meter::init(dbScale);
    level_meter::start();
    prepareLevel();

    while (true) {
        if (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) peakHoldFlag = !peakHoldFlag;  // any key to toggle peakHold
        if (level_meter::get_level(level, peakHold)) {
            for (int i = 0; i < NUM_ADC_CH; i++) {
                if (peakHoldFlag) {
                    drawLevelMeter(i, level[i], peakHold[i]);
                } else {
                    drawLevelMeter(i, level[i]);
                }
            }
        }
    }

    sleep_ms(100);

    level_meter::stop();

    return 0;
}
