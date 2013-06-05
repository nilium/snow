#ifndef __SNOW__CONFIG_HH__
#define __SNOW__CONFIG_HH__

#include "build-config.hh"

#define s_log_note(FORMAT, ARGS...)    s_log(FORMAT "\n", ##ARGS)
#define s_log_warning(FORMAT, ARGS...) s_log("Warning: " FORMAT "\n", ##ARGS)
#define s_log_error(FORMAT, ARGS...)   s_log("Error: " FORMAT "\n", ##ARGS)

// #define S_LOG_ERROR_PREFIX "E"
// #define S_LOG_WARNING_PREFIX "W"
// #define S_LOG_NOTE_PREFIX "N"

#include <snow/config.hh>
#include <snow-common.hh>

static_assert(std::numeric_limits<float>::is_iec559, "float is not IEEE 754 compatible");
static_assert(std::numeric_limits<double>::is_iec559, "float is not IEEE 754 compatible");

#include <cassert>

// Libraries
#include <physfs.h>
#include "ext/sqlite3.h"

#endif /* end __SNOW__CONFIG_HH__ include guard */
