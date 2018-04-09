#ifndef READDATA_H
#define READDATA_H

#include "tooth.h"

namespace morphomaker {

int Read_BIN_matrix( const std::string&, Tooth& );

int Read_PLY_file(const std::string&, Tooth&);

int Read_OFF_file(const std::string&, Tooth&);

int Read_Humppa_DAD_file( int, int, int, Tooth& );

}

#endif
