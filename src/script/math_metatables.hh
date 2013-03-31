#ifndef __SNOW__MATH_METATABLES_HH__
#define __SNOW__MATH_METATABLES_HH__

#include <lua.hpp>


namespace snow {


extern const char *const VEC2_METATABLE_NAME;
extern const char *const VEC3_METATABLE_NAME;
extern const char *const VEC4_METATABLE_NAME;
extern const char *const QUAT_METATABLE_NAME;
extern const char *const MAT3_METATABLE_NAME;
extern const char *const MAT4_METATABLE_NAME;
extern const char *const PLANE_METATABLE_NAME;
extern const char *const LINE_METATABLE_NAME;


template <int N>
int script_push_const_int(lua_State *L)
{
  lua_pushinteger(L, N);
  return 1;
}


} // namespace snow

#endif /* end __SNOW__MATH_METATABLES_HH__ include guard */
