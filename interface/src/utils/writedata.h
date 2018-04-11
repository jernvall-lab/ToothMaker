#pragma once

#include <string>
#include "tooth.h"


namespace morphomaker {

void Export_local_maxima(Tooth&, std::string, std::string);

void Export_main_cusp_baseline(Tooth&, std::string, std::string);

}
