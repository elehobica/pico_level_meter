/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"
#include "fm62429.h"

#include <cstdio>
#include <algorithm>

#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "ConfigParam.h"
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
static int attDb[2] = {0, 0};
static bool bothCh = true;
static int curCh = 0;;

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

static void printHelp()
{
    printf("Help Message:\r\n");
    printf(" h: Display help\r\n");
    printf(" space: Display current settings\r\n");
    printf(" s: Save current settings\r\n");
    printf(" +: Increase attenuation\r\n");
    printf(" -: Decrease attenuation\r\n");
    printf(" p: Toggle peak hold\r\n");
    printf(" b: Both channel\r\n");
    printf(" l: Left channel\r\n");
    printf(" r: Right channel\r\n");
}

static void printCurrentSettings()
{
    printf("[Current settings]\r\n");
    printf(" L: %d dB, R: %d dB\r\n", static_cast<int>(attDb[0]), static_cast<int>(attDb[1]));
    printf(" Peak hold: %s\r\n", peakHoldFlag ? "ON" : "OFF");
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

    // Load configuration from Flash
    ConfigParam& cfgParam = ConfigParam::instance();
    cfgParam.initialize();
    attDb[0] = cfgParam.P_CFG_ATT_DB_CH_L.get();
    attDb[1] = cfgParam.P_CFG_ATT_DB_CH_R.get();
    peakHoldFlag = cfgParam.P_CFG_PEAK_HOLD_MODE.get();

    // Electronic volume (FM62429)
    att = new fm62429(PIN_FM62429_CLOCK, PIN_FM62429_DATA);
    att->init();
    att->set_att(0, attDb[0]);
    att->set_att(1, attDb[1]);

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

    // Print initial settings
    printf("\r\n");
    printf("Level Meter (pico_level_meter)\r\n");
    printCurrentSettings();
    printHelp();

    getchar_timeout_us(1000);  // discard input

    while (true) {
        int chr;
        if ((chr = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {  // any key to toggle peakHold
            char c = static_cast<char>(chr);
            if (c == 'h') {
                printHelp();
            } else if (c == ' ') {
                printCurrentSettings();
            } else if (c == 's') {
                cfgParam.P_CFG_ATT_DB_CH_L.set(attDb[0]);
                cfgParam.P_CFG_ATT_DB_CH_R.set(attDb[1]);
                cfgParam.P_CFG_PEAK_HOLD_MODE.set(peakHoldFlag);
                level_meter::stop();
                if (cfgParam.finalize()) {
                    printf("Save settings to flash successfully\r\n");
                } else {
                    printf("ERROR: failed to save settings to flash\r\n");
                }
                level_meter::start();
            } else if (c == 'p') {
                peakHoldFlag = !peakHoldFlag;
                for (int i = 0; i < NUM_ADC_CH; i++) {
                    LCD_ShowString(8*14, i*16*4, reinterpret_cast<const u8*>("      "), StrColor);
                }
                if (peakHoldFlag) {
                    printf("Peak hold: ON\r\n");
                } else {
                    printf("Peak hold: OFF\r\n");
                }
            } else if (c == 'b') {
                bothCh = true;
                curCh = 0;
                printf("Ch: both\r\n");
            } else if (c == 'l') {
                bothCh = false;
                curCh = 0;
                printf("Ch: L\r\n");
            } else if (c == 'r') {
                bothCh = false;
                curCh = 1;
                printf("Ch: R\r\n");
            } else if (c == '+' || c == '=') {
                if (attDb[curCh] < fm62429::DB_MAX) {
                    attDb[curCh] += 1;
                }
                if (bothCh) {
                    att->set_att_both(attDb[curCh]);
                    attDb[1 - curCh] = attDb[curCh];
                } else {
                    att->set_att(curCh, attDb[curCh]);
                }
                printf("L: %d dB, R: %d dB\r\n", static_cast<int>(attDb[0]), static_cast<int>(attDb[1]));
            } else if (c == '-') {
                if (attDb[curCh] > fm62429::DB_MIN) {
                    attDb[curCh] -= 1;
                }
                if (bothCh) {
                    att->set_att_both(attDb[curCh]);
                    attDb[1 - curCh] = attDb[curCh];
                } else {
                    att->set_att(curCh, attDb[curCh]);
                }
                printf("L: %d dB, R: %d dB\r\n", static_cast<int>(attDb[0]), static_cast<int>(attDb[1]));
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
