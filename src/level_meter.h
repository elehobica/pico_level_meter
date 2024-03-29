/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#pragma once

#ifndef PICO_LEVEL_METER_DMA_IRQ
#define PICO_LEVEL_METER_DMA_IRQ 0
#endif

#include "conv_dB_level.h"

#define PIN_ADC_OFFSET 0  // use ADC channel from PIN_ADC_BASE + PIN_ADC_OFFSET
#define NUM_ADC_CH 2      // number of channels

namespace level_meter
{
    void init(const std::vector<float>& db_scale = conv_dB_level::DEFAULT_DB_SCALE);
    void start();
    bool get_level(int level[NUM_ADC_CH], int peak_hold[NUM_ADC_CH] = nullptr);
    void stop();
}