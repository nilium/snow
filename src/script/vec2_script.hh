#ifndef __SNOW__VEC2_SCRIPT_HH__
#define __SNOW__VEC2_SCRIPT_HH__

#include <snow/config.hh>
#include <snow/math/vec2.hh>
#include <lua.hpp>

namespace snow {


S_EXPORT
void lua_bind_vec2(lua_State *L);
S_EXPORT
void lua_pushvec2(lua_State *L, const vec2f_t &v);
S_EXPORT
vec2f_t &lua_tovec2(lua_State *L, int index);
S_EXPORT
bool lua_isvec2(lua_State *L, int index);


} // namespace snow

#endif /* end __SNOW__VEC2_SCRIPT_HH__ include guard */
