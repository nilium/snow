#ifndef __SNOW__VEC4_SCRIPT_HH__
#define __SNOW__VEC4_SCRIPT_HH__

#include "../config.hh"
#include <snow/math/vec4.hh>
#include <lua.hpp>

namespace snow {


S_EXPORT
void lua_bind_vec4(lua_State *L);
S_EXPORT
void lua_pushvec4(lua_State *L, const vec4f_t &v);
S_EXPORT
vec4f_t &lua_tovec4(lua_State *L, int index);
S_EXPORT
bool lua_isvec4(lua_State *L, int index);


} // namespace snow

#endif /* end __SNOW__VEC4_SCRIPT_HH__ include guard */
