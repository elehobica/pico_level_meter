/*------------------------------------------------------/
/ Copyright (c) 2025, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#pragma once

#include "pico/stdlib.h"

class fm62429
{
public:
    static constexpr int8_t DB_MAX = 0;
    static constexpr int8_t DB_MIN = -83;

    /**
    * @brief constructor of fm62429
    * 
    * @param pin_clock GPIO pin number for clock
    * @param pin_data GPIO pin number for data
    */
    fm62429(const uint pin_clock, const uint pin_data) : _pin_clock(pin_clock), _pin_data(pin_data) {}

    /**
    * @brief initialization of fm62429
    *
    */
    void init();

    /**
    * @brief set attenuation level of fm62429 for a channel
    *
    * @param ch channel number (0 or 1)
    * @param db attenuation level in dB (DB_MIN <= db <= DB_MAX)
    */
    void set_att(const uint8_t ch, const int8_t db);

    /**
    * @brief set attenuation level of fm62429 for both channels
    *
    * @param db attenuation level in dB (DB_MIN <= db <= DB_MAX)
    */
    void set_att_both(const int8_t db);

private:
    static constexpr uint8_t NUM_CH = 2;
    static constexpr uint64_t QUARTER_CYC_US = 10;
    const uint _pin_clock;
    const uint _pin_data;
    uint8_t get_att_code(const int8_t db);
    void send_code(const uint16_t code);
};