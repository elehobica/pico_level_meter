/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "conv_dB_level.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace level_meter
{

const std::vector<float> conv_dB_level::DEFAULT_DB_SCALE{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};

conv_dB_level::conv_dB_level(const int num_ch, const std::vector<float>& db_scale) :
    _num_ch(num_ch),
    _linear_scale(db_scale.size())
{
    // dB to linear and normalize to max
    auto it0 = db_scale.rbegin();
    float max = _db_to_linear(*it0);
    for (auto it1 = _linear_scale.rbegin(); it1 != _linear_scale.rend(); it0++, it1++) {
        *it1 = _db_to_linear(*it0) / max;
        printf("%d ", (int) (*it1 * 4095));
    }
    printf("\n");
}

conv_dB_level::~conv_dB_level()
{
}

float conv_dB_level::_db_to_linear(float db)
{
    return pow(10.0, db / 10.0);
}

void conv_dB_level::get_level(const float in[], unsigned int out[])
{
    for (int i = 0; i < _num_ch; i++) {
        // use upper_bound to find the position (not lower_bound because of 0 < linear value <= 1.0)
        auto it = std::upper_bound(_linear_scale.cbegin(), _linear_scale.cend(), in[i]);
        out[i] = std::distance(_linear_scale.cbegin(), it);
    }
}

}