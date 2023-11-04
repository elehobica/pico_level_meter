/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#pragma once

#ifndef PICO_LEVEL_METER_DMA_IRQ
#define PICO_LEVEL_METER_DMA_IRQ 0
#endif

#define PIN_ADC_OFFSET 0  // use ADC channel from PIN_ADC_BASE + PIN_ADC_OFFSET
#define NUM_ADC_CH 2      // number of channels

namespace level_meter
{
    void init();
    void start();
    bool get_level(int level[NUM_ADC_CH]);
    void stop();
}