#include "system.hh"
#include "renderer/sgl.hh"
#include <enet/enet.h>
#include <physfs.h>
#include <sqlite3.h>
#include <cstring>
#include <cerrno>
#include <functional>
#include <list>
#include <sys/stat.h>
#include "data/physicsfs_vfs.hh"

namespace snow {


namespace {


#define PKGNAME_EXT    (".snowball")
#define PKGNAME_LENGTH (9ul)
#define MAX_PATH_LEN   (512)
const string default_game_dir { "base" };



void mount_snowballs();
void set_physfs_config();
void glfw_error_callback(int error, const char *msg);
void *s_enet_malloc(size_t sz);
void s_enet_free(void *m);
void s_enet_no_memory();



void create_directory_if_not_exists(const char *dir)
{
  const int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  int succ = mkdir(dir, mode);
  if (!succ && errno != EEXIST) {
    s_log_error("Unable to create directory %s", dir);
    throw std::runtime_error("Unable to create directory");
  }
}



void create_write_dir(const char *dir)
{
  char dirdup[MAX_PATH_LEN];
  char *iter = dirdup;
  std::memset(dirdup, 0, MAX_PATH_LEN);
  size_t len;
  if (!dir) {
    throw std::invalid_argument("Write directory is NULL");
  }

  len = std::strlen(dir);
  if (len > MAX_PATH_LEN) {
    throw std::invalid_argument("Write path too long");
  } else if (*dir == '/' && len <= 1) {
    return; // nop -- / always exists.
  }

  std::strncpy(dirdup, dir, len);

  // 1: skip first / if present
  if (*iter == '/') {
    iter += 1;
  }

  for (; *iter != '\0'; ++iter) {
    if (*iter == '/' || *iter == '\\') {
      char temp = *iter;
      *iter = '\0';
      create_directory_if_not_exists(dirdup);
      *iter = temp;
    }
  }
  create_directory_if_not_exists(dirdup);
}



void mount_snowballs()
{
  char temp_path[MAX_PATH_LEN];
  const char *pfs_dir_sep = PHYSFS_getDirSeparator();
  std::list<string> snowballs;

  std::memset(temp_path, 0, MAX_PATH_LEN);

  // Find snowballs
  char **filenames = PHYSFS_enumerateFiles("/");
  for (char **iter = filenames; *iter != NULL; ++iter) {
    const char *dot = std::strrchr(*iter, '.');
    const int ext_len = dot ? std::min(PKGNAME_LENGTH, std::strlen(dot)) : 0;
    const bool is_snowball = (ext_len == PKGNAME_LENGTH) &&
      (sqlite3_strnicmp(dot, PKGNAME_EXT, ext_len) == 0);
    if (is_snowball) {
      snowballs.emplace_back(*iter);
    }
  }
  PHYSFS_freeList(filenames);
  // Mount found snowballs
  snowballs.sort(std::greater<string>());
  for (const string &snowball : snowballs) {
    const char *realdir = PHYSFS_getRealDir(snowball.c_str());
    if (realdir) {
      sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s%s",
        realdir, pfs_dir_sep, snowball.c_str());
    } else {
      continue;
    }
    bool not_mounted = (PHYSFS_getMountPoint(temp_path) == NULL);
    if (not_mounted) {
      s_log_note("Mounting snowball %s", temp_path);
      PHYSFS_mount(temp_path, "/", 1);
    }
  }
}



void set_physfs_config()
{
  #define perr(EXPR) if (!(EXPR)) throw std::runtime_error(PHYSFS_getLastError());

  int result = 0;
  char temp_path[MAX_PATH_LEN];
  std::memset(temp_path, 0, MAX_PATH_LEN);

  const char *pfs_base_dir = PHYSFS_getBaseDir();
  const char *pfs_pref_dir = PHYSFS_getPrefDir("Spifftastic", "Snow");

  // TODO: Check game cvar, load that in addition to the base dir (if
  // MOUNT_BASE_ALWAYS is defined).
  // e.g., if (game_dir != base) mount(game_dir, "/", 1);
  // That way the search path is included when looking for snowballs below.
  string game_dir = default_game_dir; // cvar_string("fs_game", "base")

  // Mount write directory for specific game
  sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_pref_dir, game_dir.c_str());
  s_log_note("Mounting %s as user game directory", temp_path);
  bool try_again = true;
  mount_write_dir:
  result = PHYSFS_setWriteDir(pfs_pref_dir);
  perr(result);
  if (!PHYSFS_mount(temp_path, "/", 1)) {
    auto code = PHYSFS_getLastErrorCode();
    if (try_again && code == PHYSFS_ERR_NOT_FOUND) {
      try_again = false;
      s_log_note("Attempting to create user game directory");
      create_write_dir(temp_path);
      goto mount_write_dir;
    }
    s_log_error("PhysFS Error: %s", PHYSFS_getErrorByCode(code));
    throw std::runtime_error("Unable to mount user game directory, PhysicsFS init failed");
  }

  // Mount base directory for specific game
  sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_base_dir, game_dir.c_str());
  s_log_note("Mounting %s as game directory", temp_path);
  if (!PHYSFS_mount(temp_path, "/", 1)) {
    auto code = PHYSFS_getLastErrorCode();
    s_log_error("PhysFS Error: %s", PHYSFS_getErrorByCode(code));
    throw std::runtime_error("Unable to mount game directory, PhysicsFS init failed");
  }

  // Mount any snowballs found as a result of mounting the read/write paths
  mount_snowballs();

#define MOUNT_BASE_ALWAYS 0
#if MOUNT_BASE_ALWAYS
  const bool is_base = (game_dir == default_game_dir);
  if (!is_base) {
    // Mount base user directory as search path if it won't automagically be
    // handled below
    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_pref_dir, default_game_dir.c_str());
    s_log_note("Mounting %s as user base directory", temp_path);
    if (!PHYSFS_mount(temp_path, "/", 1)) {
      auto code = PHYSFS_getLastErrorCode();
      if (code != PHYSFS_ERR_NOT_FOUND) {
        s_log_error("PhysFS Error: %s", PHYSFS_getErrorByCode(code));
        throw std::runtime_error("Unable to mount user base directory, PhysicsFS init failed");
      }
    }

    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_base_dir, default_game_dir.c_str());
    s_log_note("Mounting %s as base directory", temp_path);
    // Mount base/ directory
    if (!PHYSFS_mount(temp_path, "/", 1)) {
      auto code = PHYSFS_getLastErrorCode();
      if (code != PHYSFS_ERR_NOT_FOUND) {
        s_log_error("PhysFS Error: %s", PHYSFS_getErrorByCode(code));
        throw std::runtime_error("Unable to mount base directory, PhysicsFS init failed");
      }
    }

    // Mount any snowballs found as a result of adding the base paths. These are
    // appended, so the game paths will always take priority.
    mount_snowballs();
  }
#endif
}



void glfw_error_callback(int error, const char *msg)
{
  s_log_error("GLFW Error [%d] %s", error, msg);
  throw std::runtime_error("GLFW encountered an error");
}



void *s_enet_malloc(size_t sz)
{
  return new char[sz];
}



void s_enet_free(void *m)
{
  delete[] (char *)m;
}



void s_enet_no_memory()
{
  throw std::runtime_error("Unable to allocate memory for ENet");
}



ENetCallbacks g_enet_callbacks;


} // namespace



void sys_init(int argc, const char **argv)
{
  if (argc > 0) {
    s_log_note("arg0: %s", argv[0]);
  }

  s_log_note("Performing system initialization...");

  s_log_note("Initializing SQLite3");
  if (sqlite3_initialize() != SQLITE_OK) {
    throw std::runtime_error("Failed to initialize SQLite3");
  }

  // Initialize PhysFS
  s_log_note("Initializing PhysicsFS");
  if (!PHYSFS_init(argv && argc >= 1 ? argv[0] : NULL)) {
    // fail
    PHYSFS_ErrorCode err = PHYSFS_getLastErrorCode();
    const char *err_str = PHYSFS_getErrorByCode(err);
    throw std::runtime_error(err_str);
  }

  if (register_physfs_vfs(false) != SQLITE_OK) {
    throw std::runtime_error("Failed to initialize SQLite3 PhysicsFS VFS");
  }

  set_physfs_config();

  s_log_note("PhysicsFS initialized");

  // Initialize ENet
  s_log_note("Initializing ENet");
  std::memset(&g_enet_callbacks, 0, sizeof(g_enet_callbacks));
  g_enet_callbacks.malloc   = s_enet_malloc;
  g_enet_callbacks.free     = s_enet_free;
  g_enet_callbacks.no_memory = s_enet_no_memory;
  if (enet_initialize_with_callbacks(ENET_VERSION, &g_enet_callbacks)) {
    throw std::runtime_error("Failed to initialize ENet");
  }
  s_log_note("ENet initialized");

  s_log_note("Initializing GLFW");
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
  s_log_note("GLFW initialized");

  s_log_note("System initialization complete");
}



void sys_quit()
{
  glfwTerminate();
  enet_deinitialize();
  PHYSFS_deinit();
  exit(0);
}

} // namespace snow
