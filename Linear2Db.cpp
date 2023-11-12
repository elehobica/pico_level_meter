/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "Linear2Db.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace level_meter
{

const std::vector<float> Linear2Db::DEFAULT_DB_SCALE{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};

Linear2Db::Linear2Db(const int numCh, const std::vector<float>& dbScale) :
    _numCh(numCh),
    _linearScale(dbScale.size())
{
    // dB to linear and normalize to max
    auto it0 = dbScale.rbegin();
    float max = _db2Linear(*it0);
    for (auto it1 = _linearScale.rbegin(); it1 != _linearScale.rend(); it0++, it1++) {
        *it1 = _db2Linear(*it0) / max;
        printf("%d ", (int) (*it1 * 4095));
    }
    printf("\n");
}

Linear2Db::~Linear2Db()
{
}

float Linear2Db::_db2Linear(float db)
{
    return pow(10.0, db / 10.0);
}

void Linear2Db::getLevel(const float in[], unsigned int out[])
{
    for (int i = 0; i < _numCh; i++) {
        // use upper_bound to find the position (not lower_bound because of 0 < linear value <= 1.0)
        auto it = std::upper_bound(_linearScale.cbegin(), _linearScale.cend(), in[i]);
        out[i] = std::distance(_linearScale.cbegin(), it);
    }
}

}