/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "Linear2Db.h"

#include <cmath>

namespace level_meter
{

const std::vector<float> DEFAULT_DB_SCALE{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};

Linear2Db::Linear2Db(const int numCh, const std::vector<float>& dbScale) :
    _numCh(numCh),
    _linearScale(dbScale.size())
{
    // dB to linear and normalize to max
    auto it0 = dbScale.rbegin();
    float max = _db2Linear(*it0);
    for (auto it1 = _linearScale.rbegin(); it1 != _linearScale.rend(); it0++, it1++) {
        *it1 = _db2Linear(*it0) / max;
    }
}

Linear2Db::~Linear2Db()
{
}

float Linear2Db::_db2Linear(float db)
{
    return pow(10.0, db / 10.0);
}

unsigned int Linear2Db::_findPos(float value, const std::vector<float>& scale)
{
    auto low = scale.begin();
    auto mid = low + scale.size() / 2;
    auto high = scale.end() - 1;

    if (value < *low) {
        return 0;
    } else if (value >= *high) {
        return scale.size();
    } if (value <= *mid) {
        std::vector<float> nextDbScale(low, mid);
        return _findPos(value, nextDbScale);
    } else {
        std::vector<float> nextDbScale(mid, high);
        return scale.size() / 2 + _findPos(value, nextDbScale);
    }
}

void Linear2Db::getLevel(const float in[], unsigned int out[])
{
    for (int i = 0; i < _numCh; i++) {
        float value = in[i];
        if (value < 0.0) value = 0.0;
        if (value > 1.0) value = 1.0;
        out[i] = _findPos(in[i], _linearScale);
    }
}

}