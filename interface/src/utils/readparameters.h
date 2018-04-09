#ifndef READPARAMETERS_H
#define READPARAMETERS_H

namespace morphomaker {

int Import_parameters(std::string file, Parameters *par);

ScanList* Read_scanlist(std::string);

}

#endif // READPARAMETERS_H
