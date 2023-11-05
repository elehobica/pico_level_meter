/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"

#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "st7735_80x160/my_lcd.h"

void drawLevel(int ch, int level)
{
    int NUM_LEVELS = 11;
    u16 Y_CH_HEIGHT = 10;
    u16 Y_OFFSET = LCD_HEIGHT / 2 - Y_CH_HEIGHT;
    u16 width = LCD_WIDTH / NUM_LEVELS;
    u16 height = 5;

    for (int i = 0; i < NUM_LEVELS; i++) {
        if (i < level) {
            u16 color = (i < 6) ? GREEN : (i < 9) ? BRRED : RED;
            LCD_Fill(width*i, Y_OFFSET + Y_CH_HEIGHT*ch, width*i + width-2, Y_OFFSET + Y_CH_HEIGHT*ch + 5, color);
        } else {
            LCD_Fill(width*i, Y_OFFSET + Y_CH_HEIGHT*ch, width*i + width-2, Y_OFFSET + Y_CH_HEIGHT*ch + 5, DARKGRAY);
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
    level_meter::init();
    level_meter::start();

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
