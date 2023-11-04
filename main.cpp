/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"

#include <cstdio>

#include "pico/stdlib.h"

int main()
{
    stdio_init_all();

    level_meter::init();
    level_meter::start();

    while (true) {
        int level[NUM_ADC_CH];
        if (level_meter::get_level(level)) {
            printf("level %d %d\n", level[0], level[1]);
        }
    }

    sleep_ms(100);

    level_meter::stop();

    return 0;
}
