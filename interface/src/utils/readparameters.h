#pragma once

#include <string>
#include "parameters.h"
#include "misc/scanlist.h"


namespace morphomaker {

int Import_parameters(std::string file, Parameters *par);

ScanList* Read_scanlist(std::string);

}
