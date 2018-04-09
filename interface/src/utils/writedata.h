#ifndef WRITEDATA_H
#define WRITEDATA_H

#include <string>
#include "tooth.h"


namespace morphomaker {

void Export_local_maxima(Tooth&, std::string, std::string);

void Export_main_cusp_baseline(Tooth&, std::string, std::string);

}

#endif // WRITEDATA_H
