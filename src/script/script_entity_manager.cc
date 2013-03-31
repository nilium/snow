#include "script_entity_manager.hh"

namespace snow {

#define METATABLE_NAME "s_gentity_manager_mt_"
const char *const ENTITY_MANAGER_METATABLE_NAME = METATABLE_NAME;


def_lua_push_pointer(lua_push_entity_manager, entity_manager_t, METATABLE_NAME);
def_lua_to_pointer(lua_to_entity_manager, entity_manager_t, METATABLE_NAME);
def_lua_is_udata(lua_is_entity_manager, entity_manager_t, METATABLE_NAME);


} // namespace snow
