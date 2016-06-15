#ifndef FRONTENDS_INCLUDEE_COMMON_HH_
#define FRONTENDS_INCLUDEE_COMMON_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   common.hh

about:  Contains the data structures for several hardware devices in a single
        location.  The header should be included in any program that aims
        to interface with (read or write) data with this daq.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <vector>
#include <array>
#include <mutex>
#include <cstdarg>
#include <sys/time.h>

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/json_parser.cpp>
#include <boost/property_tree/json_parser.cpp>
#include "midas.h"

//--- projects includes -----------------------------------------------------//

// Some useful frontend functions.
inline load_equipment_settings(

}

#endif
