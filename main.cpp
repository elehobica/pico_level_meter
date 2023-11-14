/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"

#include <cstdio>
#include <algorithm>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "st7735_80x160/my_lcd.h"

// dbScale (max - min <= 40, otherwise lowest scale has no meaning against 12bit ADC resolution)
//static const std::vector<float> dbScale{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};  // default
static const std::vector<float> dbScale{-30, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const int NUM_LEVELS = dbScale.size();
static int greenTh;
static int redTh;

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

void drawLevel(int ch, int level)
{
    const u16 Y_CH_HEIGHT = 10;
    const u16 Y_OFFSET = LCD_HEIGHT / 2 - Y_CH_HEIGHT;
    const u16 WIDTH = LCD_WIDTH / NUM_LEVELS;
    const u16 X_OFFSET = (LCD_WIDTH -  WIDTH * NUM_LEVELS) / 2;

    for (int i = 0; i < NUM_LEVELS; i++) {
        if (i == 0 || i < level) {  // level0 is always on
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

    // BackLight PWM (125MHz / 65536 / 4 = 476.84 Hz)
    gpio_set_function(PIN_LCD_BLK, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_num, &config, true);
    int bl_val = 196;
    // Square bl_val to make brightness appear more linear
    pwm_set_gpio_level(PIN_LCD_BLK, bl_val * bl_val);

    u8 rotation = 3;
    LCD_Init();
    LCD_SetRotation(rotation);
    LCD_Clear(BLACK);
    BACK_COLOR=BLACK;


    // level meter
    level_meter::init(dbScale);
    level_meter::start();
    prepareLevel();

    while (true) {
        int level[NUM_ADC_CH];
        if (level_meter::get_level(level)) {
            printf("level %d %d\n", level[0], level[1]);
            for (int i = 0; i < NUM_ADC_CH; i++) {
                drawLevel(i, level[i]);
            }
        }
    }

    sleep_ms(100);

    level_meter::stop();

    return 0;
}
