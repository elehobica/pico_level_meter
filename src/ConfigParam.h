/*-----------------------------------------------------------/
/ ConfigParam.h
/------------------------------------------------------------/
/ Copyright (c) 2024, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/-----------------------------------------------------------*/

#pragma once

#include "FlashParam.h"

//=================================
// Interface of ConfigParam class
//=================================
struct ConfigParam : FlashParamNs::FlashParam {
    static ConfigParam& instance()  // Singleton
    {
        static ConfigParam instance;
        return instance;
    }
    static constexpr uint32_t ID_BASE = FlashParamNs::CFG_ID_BASE;
    // Parameter<T>                      instance             id           name                  default size
    FlashParamNs::Parameter<int32_t>     P_CFG_ATT_DB_CH_L   {ID_BASE + 0, "CFG_ATT_DB_CH_L",    0};
    FlashParamNs::Parameter<int32_t>     P_CFG_ATT_DB_CH_R   {ID_BASE + 1, "CFG_ATT_DB_CH_R",    0};
    FlashParamNs::Parameter<bool>        P_CFG_PEAK_HOLD_MODE{ID_BASE + 2, "CFG_PEAK_HOLD_MODE", true};
};
