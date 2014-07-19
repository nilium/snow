/*
  physicsfs_vfs.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__PHYSICSFS_VFS_HH__
#define __SNOW__PHYSICSFS_VFS_HH__

#include <snow-ext/sqlite3.h>


namespace snow {


int register_physfs_vfs(int make_default = 0);
int unregister_physfs_vfs();


} // namespace snow

#endif /* end __SNOW__PHYSICSFS_VFS_HH__ include guard */
