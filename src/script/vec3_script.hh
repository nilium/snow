#ifndef __SNOW__VEC3_SCRIPT_HH__
#define __SNOW__VEC3_SCRIPT_HH__

#include "../config.hh"
#include <snow/math/vec3.hh>
#include <lua.hpp>

namespace snow {


S_EXPORT
void lua_bind_vec3(lua_State *L);
S_EXPORT
void lua_pushvec3(lua_State *L, const vec3f_t &v);
S_EXPORT
vec3f_t &lua_tovec3(lua_State *L, int index);
S_EXPORT
bool lua_isvec3(lua_State *L, int index);


} // namespace snow

#endif /* end __SNOW__VEC3_SCRIPT_HH__ include guard */
