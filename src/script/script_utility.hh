#ifndef __SNOW__SCRIPT_UTILITY_HH__
#define __SNOW__SCRIPT_UTILITY_HH__

namespace snow {


template <typename T>
void lua_push_copy_with_metatable(lua_State *L, const T &object, const char *table)
{
  T *udata = (decltype(udata))lua_newuserdata(L, sizeof(*udata));
  new(udata) T(object);
  luaL_setmetatable(L, table);
}



template <typename T>
void lua_push_moved_with_metatable(lua_State *L, T &&object, const char *table)
{
  T *udata = (decltype(udata))lua_newuserdata(L, sizeof(*udata));
  new(udata) T(object);
  luaL_setmetatable(L, table);
}



template <typename T>
void lua_push_pointer_with_metatable(lua_State *L, T *object, const char *table)
{
  T **udata = (decltype(udata))lua_newuserdata(L, sizeof(*udata));
  *udata = object;
  luaL_setmetatable(L, table);
}



template <typename T>
T &lua_to_ref_with_metatable(lua_State *L, int index, const char *table)
{
  T *udata = (decltype(udata))luaL_checkudata(L, index, table);
  return *udata;
}



template <typename T>
T *lua_to_pointer_with_metatable(lua_State *L, int index, const char *table)
{
  T **udata = (decltype(udata))luaL_checkudata(L, index, table);
  return *udata;
}


/*******************************************************************************
*                        Lua push/to/check declarations                        *
*******************************************************************************/

#define decl_lua_push_pointer(FNAME, TYPE)                           \
void FNAME (lua_State *L, TYPE *o)

#define decl_lua_push_copy(FNAME, TYPE)                              \
void FNAME (lua_State *L, const TYPE &o)

#define decl_lua_push_move(FNAME, TYPE)                              \
void FNAME (lua_State *L, TYPE &&o)

#define decl_lua_to_pointer(FNAME, TYPE)                             \
TYPE * FNAME (lua_State *L, int index)

#define decl_lua_to_ref(FNAME, TYPE)                                 \
TYPE & FNAME (lua_State *L, int index)

#define decl_lua_is_udata(FNAME, TYPE)                               \
int FNAME (lua_State *L, int index);



/*******************************************************************************
*                        Lua push/to/check definitions                         *
*******************************************************************************/

#define def_lua_push_pointer(FNAME, TYPE, METATABLE)                 \
void FNAME (lua_State *L, TYPE *o)                                   \
{                                                                    \
  lua_push_pointer_with_metatable(L, o, (METATABLE));                \
}



#define def_lua_push_copy(FNAME, TYPE, METATABLE)                    \
void FNAME (lua_State *L, const TYPE &o)                             \
{                                                                    \
  lua_push_copy_with_metatable(L, o, (METATABLE));                   \
}



#define def_lua_push_move(FNAME, TYPE, METATABLE)                    \
void FNAME (lua_State *L, TYPE &&o)                                  \
{                                                                    \
  lua_push_move_with_metatable(L, o, (METATABLE));                   \
}



#define def_lua_to_pointer(FNAME, TYPE, METATABLE)                   \
TYPE * FNAME (lua_State *L, int index)                               \
{                                                                    \
  return lua_to_pointer_with_metatable<TYPE>(L, index, (METATABLE)); \
}



#define def_lua_to_ref(FNAME, TYPE, METATABLE)                       \
TYPE & FNAME (lua_State *L, int index)                               \
{                                                                    \
  return lua_to_ref_with_metatable<TYPE>(L, index, (METATABLE));     \
}



#define def_lua_is_udata(FNAME, TYPE, METATABLE)                     \
int FNAME (lua_State *L, int index)                                  \
{                                                                    \
  return luaL_checkudata(L, index, (METATABLE)) != NULL;             \
}


} // namespace snow

#endif /* end __SNOW__SCRIPT_UTILITY_HH__ include guard */
