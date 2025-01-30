/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"
#include "fm62429.h"

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
static const u16 StrColor = GRAY;

static fm62429 *att = nullptr;
static uint PIN_FM62429_CLOCK = 15;
static uint PIN_FM62429_DATA  = 14;
static int attDb = 0;

static inline uint32_t _millis()
{
    return to_ms_since_boot(get_absolute_time());
}

static void prepareLevel()
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

static void drawLevelMeter(int ch, int level, int peakHold = -1)
{
    const u16 Y_CH_HEIGHT = 10;
    const u16 Y_GAP = 6;
    const u16 Y_OFFSET = LCD_H() / 2 - Y_CH_HEIGHT + Y_GAP / 2;
    const u16 WIDTH = LCD_W() / NUM_LEVELS;
    const u16 X_OFFSET = (LCD_W() -  WIDTH * NUM_LEVELS) / 2;
    const u16 X_GAP = 2;

    for (int i = 0; i < NUM_LEVELS; i++) {
        if (i == 0 || i < level || i == peakHold) {  // level0 is always on
            u16 color = (i < greenTh) ? GREEN : (i < redTh) ? BRRED : RED;
            LCD_Fill(WIDTH*i + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch, WIDTH*i + WIDTH-X_GAP + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch + Y_GAP, color);
        } else {
            LCD_Fill(WIDTH*i + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch, WIDTH*i + WIDTH-X_GAP + X_OFFSET, Y_OFFSET + Y_CH_HEIGHT*ch + Y_GAP, DARKGRAY);
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

    // Electronic volume (FM62429)
    att = new fm62429(PIN_FM62429_CLOCK, PIN_FM62429_DATA);
    att->init();
    att->set_att_both(attDb);


    // level meter
    int level[NUM_ADC_CH];
    int peakHold[NUM_ADC_CH];
    level_meter::init(dbScale);
    level_meter::start();
    prepareLevel();

    // serial connection waiting (max 1 sec)
    while (!stdio_usb_connected() && _millis() < 1000) {
        sleep_ms(100);
    }
    printf("\r\n");

    getchar_timeout_us(1000);  // discard input

    while (true) {
        int chr;
        if ((chr = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {  // any key to toggle peakHold
            char c = static_cast<char>(chr);
            if (c == 'p') {
                peakHoldFlag = !peakHoldFlag;
                for (int i = 0; i < NUM_ADC_CH; i++) {
                    LCD_ShowString(8*14, i*16*4, reinterpret_cast<const u8*>("      "), StrColor);
                }
            } else if (c == '+' || c == '=') {
                if (attDb < fm62429::DB_MAX) {
                    attDb += 1;
                    att->set_att_both(attDb);
                    printf("%d dB\r\n", static_cast<int>(attDb));
                }
            } else if (c == '-') {
                if (attDb > fm62429::DB_MIN) {
                    attDb -= 1;
                    att->set_att_both(attDb);
                    printf("%d dB\r\n", static_cast<int>(attDb));
                }
            }
        }
        if (level_meter::get_level(level, peakHold)) {
            for (int i = 0; i < NUM_ADC_CH; i++) {
                if (peakHoldFlag) {
                    drawLevelMeter(i, level[i], peakHold[i]);
                    // display level by string
                    if (peakHold[i] > 0) {
                        char str[10];
                        sprintf(str, "%4ddB", static_cast<int>(dbScale[peakHold[i]]));
                        LCD_ShowString(8*14, i*16*4, reinterpret_cast<const u8*>(str), StrColor);
                    } else {
                        LCD_ShowString(8*14, i*16*4, reinterpret_cast<const u8*>("      "), StrColor);
                    }
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
