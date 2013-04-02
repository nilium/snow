#ifndef __SNOW__PHYSICSFS_VFS_HH__
#define __SNOW__PHYSICSFS_VFS_HH__

#include <sqlite3.h>


namespace snow {


int register_physfs_vfs();
int unregister_physfs_vfs();


} // namespace snow

#endif /* end __SNOW__PHYSICSFS_VFS_HH__ include guard */
