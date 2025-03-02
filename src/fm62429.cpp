/*------------------------------------------------------/
/ Copyright (c) 2025, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "fm62429.h"

void fm62429::init()
{
    gpio_init(_pin_clock);
    gpio_set_dir(_pin_clock, GPIO_OUT);
    gpio_put(_pin_clock, true);
    gpio_init(_pin_data);
    gpio_set_dir(_pin_data, GPIO_OUT);
    gpio_put(_pin_data, true);
}

void fm62429::set_att(const uint8_t ch, const int8_t db)
{
    if (ch >= NUM_CH) { return; }
    const uint16_t code = (0b11 << 9) + (get_att_code(db) << 2) + 0b10 + ch;
    send_code(code);
}

void fm62429::set_att_both(const int8_t db)
{
    const uint16_t code = (0b11 << 9) + (get_att_code(db) << 2) + 0b00;
    send_code(code);
}

uint8_t fm62429::get_att_code(const int8_t db)
{
    int8_t db2 = db;
    // Note: dB must be in the range of -83 to 0
    if (db2 < DB_MIN) {
        db2 = DB_MIN;
    } else if (db2 > DB_MAX) {
        db2 = DB_MAX;
    }
    // convert dB to 7bit attenuation code
    const uint8_t att1 = 21 + db2/4;    // D6 ~ D2
    const uint8_t att2 = 3 - (-db2)%4;  // D8, D7
    return (att2 << 5) + att1;          // D8 ~ D2
}

void fm62429::send_code(const uint16_t code)
{
    // Caution: gpio logic is opposite to actual signal due to inverter by external MOSFET
    // initial status
    gpio_put(_pin_clock, true);
    gpio_put(_pin_data, true);
    sleep_us(QUARTER_CYC_US*4);
    // data bits 0 ~ 9
    for (int i = 0; i < 10; i++) {
        gpio_put(_pin_data, ~(code >> i) & 0b1);
        sleep_us(QUARTER_CYC_US);
        gpio_put(_pin_clock, false);
        sleep_us(QUARTER_CYC_US);
        gpio_put(_pin_data, true);
        sleep_us(QUARTER_CYC_US);
        gpio_put(_pin_clock, true);
        sleep_us(QUARTER_CYC_US);
    }
    // data bits 10
    gpio_put(_pin_data, ~(code >> 10) & 0b1);
    sleep_us(QUARTER_CYC_US);
    gpio_put(_pin_clock, false);
    sleep_us(QUARTER_CYC_US);
    gpio_put(_pin_data, false);
    sleep_us(QUARTER_CYC_US);
    gpio_put(_pin_clock, true);
    sleep_us(QUARTER_CYC_US);
    gpio_put(_pin_data, true);
}