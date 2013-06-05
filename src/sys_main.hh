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
/*==============================================================================
  sys_set_physfs_config
 
    Initializes PhysFS. Is called by sys_init. If not using sys_init, this
    should be called on its own to mount the game directory and any snowballs.
*==============================================================================*/
S_EXPORT void sys_set_physfs_config(int argc, const char **argv, const char *in_base_dir);

} // namespace snow

#endif /* end __SNOW__SYS_MAIN_HH__ include guard */

