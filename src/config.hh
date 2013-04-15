#ifndef __SNOW__CONFIG_HH__
#define __SNOW__CONFIG_HH__

#include "build-config.hh"
#include <snow/config.hh>

static_assert(std::numeric_limits<float>::is_iec559, "float is not IEEE 754 compatible");
static_assert(std::numeric_limits<double>::is_iec559, "float is not IEEE 754 compatible");

#endif /* end __SNOW__CONFIG_HH__ include guard */
