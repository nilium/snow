#ifndef __SNOW__SYS_MAIN_HH__
#define __SNOW__SYS_MAIN_HH__

#include "config.hh"


namespace snow {

/*==============================================================================
  sys_init

    Initializes external libraries and any shared global data.
==============================================================================*/
S_EXPORT void sys_init(int argc, const char **argv);
/*==============================================================================
  sys_quit

    Shuts down external libraries and frees any resources necessary.
==============================================================================*/
S_EXPORT void sys_quit();

} // namespace snow

#endif /* end __SNOW__SYS_MAIN_HH__ include guard */

