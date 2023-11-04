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
class Linear2Db
{
public:
    /**
    * default series of dB scale
    */
    static const std::vector<float> DEFAULT_DB_SCALE;

    /**
    * constructor of Linear2Db
    *
    * @param[in] numCh the number of channels
    * @param[in] dbScale the series of dB scale in vector, which needs to be ascending order
    */
    Linear2Db(const int numCh = 2, const std::vector<float>& dbScale = DEFAULT_DB_SCALE);

    /**
    * destructor of Linear2Db
    */
    virtual ~Linear2Db();

    /**
    * constructor of Linear2Db
    *
    * @param[in] in the linear input array
    * @param[out] out the level array corresponsing to the series of dB scale
    */
    void getLevel(const float in[], unsigned int out[]);

protected:
    int _numCh;
    std::vector<float> _linearScale;
    float _db2Linear(float db);
};
}