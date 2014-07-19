/*
  sys_main.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "sys_main.hh"
#include "renderer/sgl.hh"
#include <enet/enet.h>
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
void glfw_error_callback(int error, const char *msg);
void *s_enet_malloc(size_t sz);
void s_enet_free(void *m);
void s_enet_no_memory();



void create_directory_if_not_exists(const char *dir)
{
  const int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  int succ = mkdir(dir, mode);
  if (!succ && errno != EEXIST) {
    s_throw(std::runtime_error, "Unable to create directory %s", dir);
  }
}



void create_write_dir(const char *dir)
{
  char dirdup[MAX_PATH_LEN];
  char *iter = dirdup;
  std::memset(dirdup, 0, MAX_PATH_LEN);
  size_t len;
  if (!dir) {
    s_throw(std::invalid_argument, "Write directory is NULL");
  }

  len = std::strlen(dir);
  if (len > MAX_PATH_LEN) {
    s_throw(std::invalid_argument, "Write path too long");
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
    const int ext_len = dot ? (int)std::min(PKGNAME_LENGTH, std::strlen(dot)) : 0;
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



void glfw_error_callback(int error, const char *msg)
{
  s_throw(std::runtime_error, "GLFW Error [%d] %s", error, msg);
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
  s_throw(std::runtime_error, "Unable to allocate memory for ENet");
}



ENetCallbacks g_enet_callbacks;


} // namespace <anon>



void sys_init(int argc, const char **argv)
{
  if (argc > 0) {
    s_log_note("arg0: %s", argv[0]);
  }

  s_log_note("Performing system initialization...");

  s_log_note("Initializing SQLite3");
  if (sqlite3_initialize() != SQLITE_OK) {
    s_throw(std::runtime_error, "Failed to initialize SQLite3");
  }

  sys_set_physfs_config(argc, argv, NULL);

  s_log_note("PhysicsFS initialized");

  // Initialize ENet
  s_log_note("Initializing ENet");
  std::memset(&g_enet_callbacks, 0, sizeof(g_enet_callbacks));
  g_enet_callbacks.malloc   = s_enet_malloc;
  g_enet_callbacks.free     = s_enet_free;
  g_enet_callbacks.no_memory = s_enet_no_memory;
  if (enet_initialize_with_callbacks(ENET_VERSION, &g_enet_callbacks)) {
    s_throw(std::runtime_error, "Failed to initialize ENet");
  }
  s_log_note("ENet initialized");

  s_log_note("Initializing GLFW");
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    s_throw(std::runtime_error, "Failed to initialize GLFW");
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



void sys_set_physfs_config(int argc, const char **argv, const char *in_base_dir)
{
#define perr(EXPR ) if (!(EXPR)) s_throw(std::runtime_error, "%s", PHYSFS_getLastError());

  int result = 0;
  char temp_path[MAX_PATH_LEN];
  std::memset(temp_path, 0, MAX_PATH_LEN);

  // Initialize PhysFS
  s_log_note("Initializing PhysicsFS");
  if (!PHYSFS_init(argv && argc >= 1 ? argv[0] : NULL)) {
    // fail
    PHYSFS_ErrorCode err = PHYSFS_getLastErrorCode();
    const char *err_str = PHYSFS_getErrorByCode(err);
    s_throw(std::runtime_error, "PhysFS Init Error: %s", err_str);
  }

  if (register_physfs_vfs(false) != SQLITE_OK) {
    s_throw(std::runtime_error, "Failed to initialize SQLite3 PhysicsFS VFS");
  }

#if BUILD_EDITOR
  PHYSFS_permitSymbolicLinks(1);
#endif

  const char *pfs_base_dir = in_base_dir ? in_base_dir : PHYSFS_getBaseDir();
  const char *pfs_pref_dir = PHYSFS_getPrefDir("Spifftastic", "Snow");
  const char *base_suffix = std::strrchr(pfs_base_dir, '.');
  if (base_suffix && std::strlen(base_suffix) == 5 &&
      sqlite3_strnicmp(base_suffix, ".app/", 5) == 0) {
    base_suffix = "Contents/Resources/";
  } else {
    base_suffix = "";
  }

  // TODO: Check game cvar, load that in addition to the base dir (if
  // MOUNT_BASE_ALWAYS is defined).
  // e.g., if (game_dir != base) mount(game_dir, "/", 1);
  // That way the search path is included when looking for snowballs below.
  string game_dir = default_game_dir; // cvar_string("fs_game", "base")
  const bool is_base = (game_dir == default_game_dir);

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
    s_throw(std::runtime_error, "PhysFS Error: %s", PHYSFS_getErrorByCode(code));
  }

  // Mount base directory for specific game
  sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s%s", pfs_base_dir, base_suffix, game_dir.c_str());
  s_log_note("Mounting %s as game directory", temp_path);
  if (!PHYSFS_mount(temp_path, "/", 1)) {
    auto code = PHYSFS_getLastErrorCode();
    if (code == PHYSFS_ERR_NOT_FOUND && !is_base) {
      sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_pref_dir, game_dir.c_str());
      s_log_note("Failed - attempting again, mounting %s as game directory", temp_path);
      if (PHYSFS_mount(temp_path, "/", 1)) {
        goto skip_fs_error;
      }
    }
    s_throw(std::runtime_error, "PhysFS Error: %s", PHYSFS_getErrorByCode(code));
  }
skip_fs_error:

  // Mount any snowballs found as a result of mounting the read/write paths
  mount_snowballs();

#if MOUNT_BASE_ALWAYS
  if (!is_base) {
    // Mount base user directory as search path if it won't automagically be
    // handled below
    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s", pfs_pref_dir, default_game_dir.c_str());
    s_log_note("Mounting %s as user base directory", temp_path);
    if (!PHYSFS_mount(temp_path, "/", 1)) {
      auto code = PHYSFS_getLastErrorCode();
      if (code != PHYSFS_ERR_NOT_FOUND) {
        s_throw(std::runtime_error, "PhysFS Error: %s", PHYSFS_getErrorByCode(code));
      }
    }

    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s%s%s", pfs_base_dir, base_suffix, default_game_dir.c_str());
    s_log_note("Mounting %s as base directory", temp_path);
    // Mount base/ directory
    if (!PHYSFS_mount(temp_path, "/", 1)) {
      auto code = PHYSFS_getLastErrorCode();
      if (code != PHYSFS_ERR_NOT_FOUND) {
        s_throw(std::runtime_error, "PhysFS Error: %s", PHYSFS_getErrorByCode(code));
      }
    }

    // Mount any snowballs found as a result of adding the base paths. These are
    // appended, so the game paths will always take priority.
    mount_snowballs();
  }
#endif
}


} // namespace snow
