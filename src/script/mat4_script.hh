#ifndef __SNOW__MAT4_SCRIPT_HH__
#define __SNOW__MAT4_SCRIPT_HH__

#include "../config.hh"
#include <snow/math/mat4.hh>
#include <lua.hpp>

namespace snow {


S_EXPORT
void lua_bind_mat4(lua_State *L);
S_EXPORT
void lua_pushmat4(lua_State *L, const mat4f_t &m);
S_EXPORT
mat4f_t &lua_tomat4(lua_State *L, int index);
S_EXPORT
bool lua_ismat4(lua_State *L, int index);


} // namespace snow

#endif /* end __SNOW__MAT4_SCRIPT_HH__ include guard */
