#ifdef TARGET_OS_MAC
#include <dispatch/dispatch.h>
#include <Foundation/Foundation.h>
#endif

#include <unistd.h>

#include "client/cl_main.hh"



#ifdef LUA_TEST
#include <lua.hpp>
#include "script/vec2_script.hh"
#include "script/vec3_script.hh"
#include "script/vec4_script.hh"

void lua_test()
{
  using namespace snow;

  lua_State *L = luaL_newstate();
  luaopen_base(L);
  luaopen_debug(L);
  lua_bind_vec2(L);
  lua_bind_vec3(L);
  lua_bind_vec4(L);

  if (luaL_dofile(L, "test.lua")) {
    std::clog << "Lua error: " << lua_tostring(L, -1) << std::endl;
  }

  lua_close(L);

  exit(0);
}
#endif


// #define DB_TEST
#ifdef DB_TEST
#include "data/database.hh"
#include "renderer/font.hh"

void db_test()
{
  using namespace snow;

  database_t db("fonts.db", true, SQLITE_OPEN_READONLY);
  rfont_t font(db, "Menlo-Regular");
  db.close();
  exit(0);
}
#endif



int main(int argc, char const *argv[])
{
  using namespace snow;

#ifdef LUA_TEST
  lua_test();
#endif
#ifdef DB_TEST
  db_test();
#endif

  // Bootstrap
#ifdef TARGET_OS_MAC
    @autoreleasepool {
#endif
  // Queue up the actual main routine
  dispatch_async(dispatch_get_main_queue(), [&] {
    client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);
  });
#ifdef TARGET_OS_MAC
  [[NSRunLoop mainRunLoop] run];
  } // @autoreleasepool
#else
  dispatch_main();
#endif
  return 0;
}
