/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <vector>

namespace level_meter
{
/**
* class for converting the linear values to the levels in dB scale
*/
class conv_dB_level
{
public:
    /**
    * default series of dB scale
    */
    static const std::vector<float> DEFAULT_DB_SCALE;

    /**
    * constructor of conv_dB_level
    *
    * @param[in] num_ch the number of channels
    * @param[in] db_scale the series of dB scale in vector, which needs to be ascending order
    */
    conv_dB_level(const int num_ch = 2, const std::vector<float>& db_scale = DEFAULT_DB_SCALE);

    /**
    * destructor of conv_dB_level
    */
    virtual ~conv_dB_level();

    /**
    * constructor of conv_dB_level
    *
    * @param[in] in the linear input array
    * @param[out] out the level array corresponsing to the series of dB scale
    */
    void get_level(const float in[], unsigned int out[]);

protected:
    int _num_ch;
    std::vector<float> _linear_scale;
    float _db_to_linear(float db);
};
}