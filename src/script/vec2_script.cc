#include "vec2_script.hh"
#include "math_metatables.hh"
#include <snow/types/object_pool.hh>

namespace snow {


namespace {

using vec2_pool_t = object_pool_t<vec2f_t>;
vec2_pool_t g_vec2_pool;
const char *        METATABLE_NAME = VEC2_METATABLE_NAME;


// extract vec2 pool index from udata at index
inline vec2_pool_t::index_t extract_vec2_index(lua_State *L, int index);
int script_gc_vec2(lua_State *L);
void script_push_vec2_metatable(lua_State *L);
int script_vec2_normalize(lua_State *L);
int script_vec2_normalized(lua_State *L);
int script_vec2_magnitude(lua_State *L);
int script_vec2_length(lua_State *L);
int script_vec2_difference(lua_State *L);
int script_vec2_subtract(lua_State *L);
int script_vec2_sum(lua_State *L);
int script_vec2_add(lua_State *L);
int script_vec2_scaled(lua_State *L);
int script_vec2_scaled(lua_State *L);
int script_vec2_scale(lua_State *L);
int script_vec2_scale(lua_State *L);
int script_vec2_negated(lua_State *L);
int script_vec2_negate(lua_State *L);
int script_vec2_inverse(lua_State *L);
int script_vec2_invert(lua_State *L);
int script_vec2_dot_product(lua_State *L);
int script_vec2_rotate_elems(lua_State *L);
int script_vec2_rotated_elems(lua_State *L);


inline vec2_pool_t::index_t extract_vec2_index(lua_State *L, int index)
{
  const vec2_pool_t::index_t *obj_box;
#ifndef NDEBUG
  obj_box = (const vec2_pool_t::index_t *)luaL_checkudata(L, index, METATABLE_NAME);
#else
  obj_box = (const vec2_pool_t::index_t *)lua_touserdata(L, index);
#endif
  const vec2_pool_t::index_t pool_index = *obj_box;
  return pool_index;
}



int script_gc_vec2(lua_State *L)
{
  const vec2_pool_t::index_t pool_index = extract_vec2_index(L, 1);
  g_vec2_pool.collect(pool_index);
  return 0;
}


int script_newindex_vec2(lua_State *L)
{
  const float newval = lua_tonumber(L, 3);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TSTRING: {
      size_t len = 0;
      const char *lstr = lua_tolstring(L, 2, &len);
      if (len == 1) {
        vec2f_t &v = lua_tovec2(L, 1);
        switch (lstr[0]) {
        case 'x':
          v.x = newval;
          return 0;
        case 'y':
          v.y = newval;
          return 0;
        default: break;
        }
      }

      return luaL_error(L, "Expected x or y, got %s", lstr);
    }

  case LUA_TNUMBER: {
      const int comp_index = lua_tointeger(L, 2);
      if (comp_index < 1 || 3 < comp_index)
        return luaL_error(L, "Index out of range for vec2 [1..2]: %d", comp_index);

      lua_tovec2(L, 1)[comp_index - 1] = lua_tonumber(L, 3);
      return 0;
    }

  default:
    return luaL_error(L, "Expected string or index for vec2 member, got type %s",
                      lua_typename(L, type));
  }
}



int script_index_vec2(lua_State *L)
{
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TSTRING: {
      size_t len = 0;
      const char *lstr = lua_tolstring(L, 2, &len);
      if (len == 1) {
        vec2f_t &v = lua_tovec2(L, 1);
        switch (lstr[0]) {
        case 'x':
          lua_pushnumber(L, v.x);
          return 1;
        case 'y':
          lua_pushnumber(L, v.y);
          return 1;
        default:
          return luaL_error(L, "Expected x, y, or a member function, got %s", lstr);
        }
      } else {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_insert(L, 2);
        lua_rawget(L, -2);
        return 1;
      }
    }

  case LUA_TNUMBER: {
      const int comp_index = lua_tointeger(L, 2);
      if (comp_index < 1 || 3 < comp_index)
        return luaL_error(L, "Index out of range for vec2 [1..2]: %d", comp_index);

      lua_pushnumber(L, lua_tovec2(L, 1)[comp_index - 1]);
      return 1;
    }

  default:
    return luaL_error(L, "Expected string or index for vec2 member, got type %s",
                      lua_typename(L, type));
  }
}



int script_vec2_normalize(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.normalize();
  lua_settop(L, 1);
  return 1;
}



int script_vec2_normalized(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.normalized());
  return 1;
}



int script_vec2_magnitude(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushnumber(L, v.magnitude());
  return 1;
}



int script_vec2_length(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushnumber(L, v.length());
  return 1;
}



int script_vec2_difference(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.difference(lua_tovec2(L, 2)));;
  return 1;
}



int script_vec2_subtract(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.subtract(lua_tovec2(L, 2));
  lua_settop(L, 1);
  return 1;
}



int script_vec2_sum(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.sum(lua_tovec2(L, 2)));;
  return 1;
}



int script_vec2_add(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.add(lua_tovec2(L, 2));
  lua_settop(L, 1);
  return 1;
}



int script_vec2_scaled(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    lua_pushvec2(L, v.scaled(lua_tonumber(L, 2)));
    return 1;
  case LUA_TUSERDATA:
    lua_pushvec2(L, v.scaled(lua_tovec2(L, 2)));;
    return 1;
  default:
    return luaL_error(L, "vec2:scaled - Expected number or vec2, got %s",
               lua_typename(L, type));
  }
}



int script_vec2_scale(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    lua_pushvec2(L, v.scaled(lua_tonumber(L, 2)));
    break;
  case LUA_TUSERDATA:
    lua_pushvec2(L, v.scaled(lua_tovec2(L, 2)));;
    break;
  default:
    return luaL_error(L, "vec2:scaled - Expected number or vec2, got %s",
               lua_typename(L, type));
  }
  lua_settop(L, 1);
  return 1;
}



int script_vec2_negated(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.negated());
  return 1;
}



int script_vec2_negate(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.negate();
  lua_settop(L, 1);
  return 1;
}



int script_vec2_inverse(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.inverse());
  return 1;
}



int script_vec2_invert(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.invert();
  lua_settop(L, 1);
  return 1;
}



int script_vec2_dot_product(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushnumber(L, v.dot_product(lua_tovec2(L, 2)));
  return 1;
}



int script_vec2_rotate_elems(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  v.rotate_elems();
  lua_settop(L, 1);
  return 1;
}



int script_vec2_rotated_elems(lua_State *L)
{
  vec2f_t &v = lua_tovec2(L, 1);
  lua_pushvec2(L, v.rotated_elems());
  return 1;
}



int script_vec2(lua_State *L)
{
  float x, y;
  x = luaL_optnumber(L, 1, 0.0);
  y = luaL_optnumber(L, 2, x);
  lua_pushvec2(L, {x, y});
  return 1;
}



void script_push_vec2_metatable(lua_State *L)
{
  if (luaL_newmetatable(L, METATABLE_NAME)) {
    lua_pushcfunction(L, script_gc_vec2);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, script_push_const_int<2>);
    lua_setfield(L, -2, "__len");
    lua_createtable(L, 0, 19); // method table
    lua_pushcfunction(L, script_vec2_normalize);
    lua_setfield(L, -2, "normalize");
    lua_pushcfunction(L, script_vec2_normalized);
    lua_setfield(L, -2, "normalized");
    lua_pushcfunction(L, script_vec2_magnitude);
    lua_setfield(L, -2, "magnitude");
    lua_pushcfunction(L, script_vec2_length);
    lua_setfield(L, -2, "length");
    lua_pushcfunction(L, script_vec2_difference);
    lua_setfield(L, -2, "difference");
    lua_pushcfunction(L, script_vec2_subtract);
    lua_setfield(L, -2, "subtract");
    lua_pushcfunction(L, script_vec2_sum);
    lua_setfield(L, -2, "sum");
    lua_pushcfunction(L, script_vec2_add);
    lua_setfield(L, -2, "add");
    lua_pushcfunction(L, script_vec2_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_vec2_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_vec2_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_vec2_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_vec2_negated);
    lua_setfield(L, -2, "negated");
    lua_pushcfunction(L, script_vec2_negate);
    lua_setfield(L, -2, "negate");
    lua_pushcfunction(L, script_vec2_inverse);
    lua_setfield(L, -2, "inverse");
    lua_pushcfunction(L, script_vec2_invert);
    lua_setfield(L, -2, "invert");
    lua_pushcfunction(L, script_vec2_dot_product);
    lua_setfield(L, -2, "dot_product");
    lua_pushcfunction(L, script_vec2_rotate_elems);
    lua_setfield(L, -2, "rotate_elems");
    lua_pushcfunction(L, script_vec2_rotated_elems);
    lua_setfield(L, -2, "rotated_elems");
    lua_pushcclosure(L, script_index_vec2, 1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, script_newindex_vec2, 0);
    lua_setfield(L, -2, "__newindex");
  }
}


} // namespace <anon>



/*==============================================================================
  lua_bind_vec2

    Binds the vec2 script API to the Lua state.
==============================================================================*/
void lua_bind_vec2(lua_State *L)
{
  script_push_vec2_metatable(L);
  lua_pop(L, 1);
  lua_register(L, "vec2", script_vec2);
}



void lua_pushvec2(lua_State *L, const vec2f_t &m)
{
  const vec2_pool_t::index_t pool_index = g_vec2_pool.reserve_with(m);
  vec2_pool_t::index_t *obj_box;
  obj_box = (vec2_pool_t::index_t *)lua_newuserdata(L, sizeof(pool_index));
  *obj_box = pool_index;
  luaL_setmetatable(L, METATABLE_NAME);
}



vec2f_t &lua_tovec2(lua_State *L, int index)
{
  return g_vec2_pool[extract_vec2_index(L, index)];
}



bool lua_isvec2(lua_State *L, int index)
{
  return (luaL_checkudata(L, index, METATABLE_NAME) != NULL);
}


} // namespace snow
