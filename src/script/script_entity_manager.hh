#ifndef __SNOW__SCRIPT_ENTITY_MANAGER_HH__
#define __SNOW__SCRIPT_ENTITY_MANAGER_HH__

#include "../config.hh"
#include <lua.hpp>
#include "script_utility.hh"

namespace snow {

struct entity_manager_t;


extern const char *const ENTITY_MANAGER_METATABLE_NAME;


S_EXPORT
void lua_bind_entity_manager(lua_State *L);
decl_lua_push_pointer(lua_push_entity_manager, entity_manager_t);
decl_lua_to_pointer(lua_to_entity_manager, entity_manager_t);
decl_lua_is_udata(lua_is_entity_manager, entity_manager_t);


} // namespace snow

#endif /* end __SNOW__SCRIPT_ENTITY_MANAGER_HH__ include guard */
