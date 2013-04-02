#include "physicsfs_vfs.hh"
#include <snow/config.hh>
#include <physfs.h>
#ifdef USE_DEV_RANDOM
#include <cstdio>
#endif
#include <ctime>
#include <random>
#include <thread>


namespace snow {


namespace {


#define P_MAX_PATH_LENGTH 512
#define P_VFS_NAME "physfs"


using p_dlfn_t = void (*)(void);



// sqlite3_io_methods
int xClose(sqlite3_file *file);
int xRead(sqlite3_file *file, void *out, int bytes, sqlite3_int64 offset);
int xWrite(sqlite3_file *file, const void*, int num_bytes, sqlite3_int64 offset);
int xTruncate(sqlite3_file *file, sqlite3_int64 size);
int xSync(sqlite3_file *file, int flags);
int xFileSize(sqlite3_file *file, sqlite3_int64 *pSize);
int xLock(sqlite3_file *file, int lock);
int xUnlock(sqlite3_file *file, int lock);
int xCheckReservedLock(sqlite3_file *file, int *pResOut);
int xFileControl(sqlite3_file *file, int op, void *pArg);
int xSectorSize(sqlite3_file *file);
int xDeviceCharacteristics(sqlite3_file *file);



// sqlite3_vfs
int xOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *fileout, int flags, int *pOutFlags);
int xDelete(sqlite3_vfs *vfs, const char *zName, int syncDir);
int xAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut);
int xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut);
void *xDlOpen(sqlite3_vfs *vfs, const char *zFilename);
void xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg);
p_dlfn_t xDlSym(sqlite3_vfs *vfs,void*, const char *zSymbol);
void xDlClose(sqlite3_vfs *vfs, void*);
int xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut);
int xSleep(sqlite3_vfs *vfs, int microseconds);
int xCurrentTime(sqlite3_vfs *vfs, double*);
int xGetLastError(sqlite3_vfs *vfs, int, char *);



struct p_file
{
  sqlite3_file super;
  PHYSFS_File *file;
  bool no_file;
};



sqlite3_io_methods p_io_methods = {
  1,
  xClose,
  xRead,
  xWrite,
  xTruncate,
  xSync,
  xFileSize,
  xLock,
  xUnlock,
  xCheckReservedLock,
  xFileControl,
  xSectorSize,
  xDeviceCharacteristics,
  NULL,
  NULL,
  NULL
};



sqlite3_vfs p_vfs = {
  1,
  sizeof(p_file),
  P_MAX_PATH_LENGTH,
  NULL,
  P_VFS_NAME,
  NULL,
  // V1
  xOpen,
  xDelete,
  xAccess,
  xFullPathname,
  xDlOpen,
  xDlError,
  xDlSym,
  xDlClose,
  xRandomness,
  xSleep,
  xCurrentTime,
  xGetLastError,
  // V2
  NULL,
  // V3
  NULL,
  NULL,
  NULL
};



/*******************************************************************************
*                              sqlite3_io_methods                              *
*******************************************************************************/

int xClose(sqlite3_file *file)
{
  p_file *pf = (p_file *)file;
  if (pf->no_file) {
    return SQLITE_OK;
  } else if (pf->file && PHYSFS_close(pf->file)) {
    pf->file = NULL;
    return SQLITE_OK;
  }
  return SQLITE_IOERR_CLOSE;
}



int xRead(sqlite3_file *file, void *out, int num_bytes, sqlite3_int64 offset)
{
  p_file *pf = (p_file *)file;

  if (pf->no_file || pf->file == NULL) {
    return SQLITE_IOERR;
  }

  if (!PHYSFS_seek(pf->file, (PHYSFS_uint64)offset)) {
    return SQLITE_IOERR_SEEK;
  }

  PHYSFS_sint64 num_read = PHYSFS_readBytes(pf->file, out, num_bytes);
  if (num_read > 0) {
    return num_read == num_bytes ? SQLITE_OK : SQLITE_IOERR_SHORT_READ;
  }

  return SQLITE_IOERR_READ;;
}



int xWrite(sqlite3_file *file, const void *in, int num_bytes, sqlite3_int64 offset)
{
  p_file *pf = (p_file *)file;

  if (pf->no_file) {
    return SQLITE_OK;
  } else if (!pf->file) {
    return SQLITE_IOERR_WRITE;
  }

  if (!PHYSFS_seek(pf->file, (PHYSFS_uint64)offset)) {
    return SQLITE_IOERR_SEEK;
  }

  PHYSFS_sint64 num_written = PHYSFS_writeBytes(pf->file, in, num_bytes);
  if (num_written > 0) {
    return num_written == num_bytes ? SQLITE_OK : SQLITE_IOERR_WRITE;
  }

  return SQLITE_IOERR_WRITE;
}



int xTruncate(sqlite3_file *file, sqlite3_int64 size)
{
  p_file *pf = (p_file *)file;
  if (pf->no_file || pf->file == NULL) {
    return SQLITE_IOERR;
  }
  return SQLITE_OK;
}



int xSync(sqlite3_file *file, int flags)
{
  p_file *pf = (p_file *)file;
  if (pf->no_file || (pf->file && PHYSFS_flush(pf->file))) {
    return SQLITE_OK;
  } else {
    return SQLITE_IOERR_FSYNC;
  }
}



int xFileSize(sqlite3_file *file, sqlite3_int64 *pSize)
{

  p_file *pf = (p_file *)file;
  if (pf->no_file || pf->file == NULL) {
    return SQLITE_IOERR;
  }

  *pSize = (sqlite3_int64)PHYSFS_fileLength(pf->file);
  return SQLITE_OK;
}



int xLock(sqlite3_file *file, int lock)
{
  // NOP
  return SQLITE_OK;
}



int xUnlock(sqlite3_file *file, int lock)
{
  // NOP
  return SQLITE_OK;
}



int xCheckReservedLock(sqlite3_file *file, int *pResOut)
{
  // Mostly a NOP
  *pResOut = 0;
  return SQLITE_OK;
}



int xFileControl(sqlite3_file *file, int op, void *pArg)
{
  // NOP
  return SQLITE_OK;
}



int xSectorSize(sqlite3_file *file)
{
  return 512;
}



int xDeviceCharacteristics(sqlite3_file *file)
{
  return 0;
}



/*******************************************************************************
*                                 sqlite3_vfs                                  *
*******************************************************************************/

int xOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *fileout, int flags, int *pOutFlags)
{
  p_file *pf = (p_file *)fileout;

  pf->super.pMethods = &p_io_methods;

  if (!zName) {
    return SQLITE_IOERR;
  }

  pf->no_file = false;

  if (flags & SQLITE_OPEN_MAIN_JOURNAL ||
    flags & SQLITE_OPEN_TEMP_JOURNAL ||
    flags & SQLITE_OPEN_MASTER_JOURNAL ||
    flags & SQLITE_OPEN_SUBJOURNAL)
  {
    pf->file = NULL;
    pf->no_file = true;
    *pOutFlags = SQLITE_OPEN_READONLY;
  } else {
    pf->no_file = false;

    if (flags & SQLITE_OPEN_READONLY) {
      pf->file = PHYSFS_openRead(zName);
      *pOutFlags = SQLITE_OPEN_READONLY;
    } else if (flags & SQLITE_OPEN_CREATE) {
      pf->file = PHYSFS_openWrite(zName);
      *pOutFlags = SQLITE_OPEN_CREATE;
    } else if (flags & SQLITE_OPEN_READWRITE) {
      pf->file = NULL;
    }
  }

  return (pf->file || pf->no_file) ? SQLITE_OK : SQLITE_IOERR;
}



int xDelete(sqlite3_vfs *vfs, const char *zName, int syncDir)
{
  return PHYSFS_delete(zName) ? SQLITE_OK : SQLITE_IOERR_DELETE;
}



int xAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut)
{
  if (flags == SQLITE_ACCESS_READ || flags == SQLITE_ACCESS_EXISTS) {
    *pResOut = PHYSFS_exists(zName);
    return SQLITE_OK;
  } else if (flags == SQLITE_ACCESS_READ) {
    PHYSFS_File *file = PHYSFS_openRead(zName);
    *pResOut = (file != NULL);
    if (file != NULL && !PHYSFS_close(file)) {
      return SQLITE_IOERR_ACCESS;
    } else {
      return SQLITE_OK;
    }
  } else if (flags == SQLITE_ACCESS_READWRITE) {
    *pResOut = 0;
    return SQLITE_OK;
  } else {
    return SQLITE_IOERR_ACCESS;
  }
}



int xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut)
{
  /*
  const char *realdir = PHYSFS_getRealDir(zName);
  if (realdir != NULL) {
    string input { zName };
    auto last_slash = input.rfind('/');
    if (last_slash != string::npos) {
      input = input.substr(last_slash + 1);
    }
    return sqlite3_snprintf(nOut, zOut, "%s/%s", realdir, input.c_str()) ? SQLITE_OK : SQLITE_IOERR;
  } else {
  */
    return sqlite3_snprintf(nOut, zOut, "%s", zName) ? SQLITE_OK : SQLITE_IOERR;
  /*
  }
  */
}



void *xDlOpen(sqlite3_vfs *vfs, const char *zFilename)
{
  return NULL;
}



void xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg)
{
  sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are unsupported");
}



p_dlfn_t xDlSym(sqlite3_vfs *vfs, void *dl, const char *zSymbol)
{
  return NULL;
}



void xDlClose(sqlite3_vfs *vfs, void *dl)
{
  return;
}



int xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut)
{
  int result = nByte;
#ifdef USE_DEV_RANDOM
  FILE *dev_rand = std::fopen("/dev/urandom", "rb");
  if (dev_rand == NULL) {
    dev_rand = std::fopen("/dev/random", "rb");
  }

  if (dev_rand != NULL) {
    size_t last_read = 1;
    while (nByte) {
      size_t bytes_read = fread(zOut, 1, nByte, dev_rand);
      zOut += bytes_read;
      nByte -= bytes_read;
      if (bytes_read == 0 && last_read == 0) {
        // Assume unable to read more, use other PRNG for remaining bytes
        break;
      }
    }

    fclose(dev_rand);
    dev_rand = NULL;

    if (nByte == 0)
      return result;
  }
#endif

  using prng = std::mt19937;
  prng engine;
  while (nByte) {
    if (nByte >= sizeof(prng::result_type)) {
      *(prng::result_type *)zOut = engine();
    } else {
      *(char *)zOut = (char)engine();
    }
    nByte -= 1;
  }

  return result;
}



int xSleep(sqlite3_vfs *vfs, int microseconds)
{
  std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
  return microseconds;
}



int xCurrentTime(sqlite3_vfs *vfs, double *out)
{
  // borrowed from sqlite3 demo vfs
  time_t t = time(0);
  *out = t / 86400.0 + 2440587.5;
  return SQLITE_OK;
}



int xGetLastError(sqlite3_vfs *vfs, int nBytes, char *out)
{
  sqlite3_snprintf(nBytes, out, "PhysFS Error: %s", PHYSFS_getLastError());
  return SQLITE_OK;
}



} // namespace <anon>



int register_physfs_vfs()
{
  return sqlite3_vfs_register(&p_vfs, 1);
}



int unregister_physfs_vfs()
{
  return sqlite3_vfs_unregister(&p_vfs);
}


} // namespace snow

