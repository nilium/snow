#include "vec4_script.hh"
#include "math_metatables.hh"
#include <snow/types/object_pool.hh>

namespace snow {


namespace {


using vec4_pool_t = object_pool_t<vec4f_t>;
vec4_pool_t g_vec4_pool;
const char *        METATABLE_NAME = VEC4_METATABLE_NAME;


// extract vec4 pool index from udata at index
inline vec4_pool_t::index_t extract_vec4_index(lua_State *L, int index);
int script_gc_vec4(lua_State *L);
void script_push_vec4_metatable(lua_State *L);
int script_vec4_normalize(lua_State *L);
int script_vec4_normalized(lua_State *L);
int script_vec4_magnitude(lua_State *L);
int script_vec4_length(lua_State *L);
int script_vec4_difference(lua_State *L);
int script_vec4_subtract(lua_State *L);
int script_vec4_sum(lua_State *L);
int script_vec4_add(lua_State *L);
int script_vec4_scaled(lua_State *L);
int script_vec4_scaled(lua_State *L);
int script_vec4_scale(lua_State *L);
int script_vec4_scale(lua_State *L);
int script_vec4_negated(lua_State *L);
int script_vec4_negate(lua_State *L);
int script_vec4_inverse(lua_State *L);
int script_vec4_invert(lua_State *L);
int script_vec4_dot_product(lua_State *L);
int script_vec4_rotate_elems(lua_State *L);
int script_vec4_rotated_elems(lua_State *L);


inline vec4_pool_t::index_t extract_vec4_index(lua_State *L, int index)
{
  const vec4_pool_t::index_t *obj_box;
#ifndef NDEBUG
  obj_box = (const vec4_pool_t::index_t *)luaL_checkudata(L, index, METATABLE_NAME);
#else
  obj_box = (const vec4_pool_t::index_t *)lua_touserdata(L, index);
#endif
  const vec4_pool_t::index_t pool_index = *obj_box;
  return pool_index;
}



int script_gc_vec4(lua_State *L)
{
  const vec4_pool_t::index_t pool_index = extract_vec4_index(L, 1);
  g_vec4_pool.collect(pool_index);
  return 0;
}


int script_newindex_vec4(lua_State *L)
{
  const float newval = lua_tonumber(L, 3);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TSTRING: {
      size_t len = 0;
      const char *lstr = lua_tolstring(L, 2, &len);
      if (len == 1) {
        vec4f_t &v = lua_tovec4(L, 1);
        switch (lstr[0]) {
        case 'x':
          v.x = newval;
          return 0;
        case 'y':
          v.y = newval;
          return 0;
        case 'z':
          v.z = newval;
          return 0;
        case 'w':
          v.w = newval;
          return 0;
        default: break;
        }
      }

      return luaL_error(L, "Expected x, y, z, or w, got %s", lstr);
    }

  case LUA_TNUMBER: {
      const int comp_index = lua_tointeger(L, 2);
      if (comp_index < 1 || 4 < comp_index)
        return luaL_error(L, "Index out of range for vec4 [1..4]: %d", comp_index);

      lua_tovec4(L, 1)[comp_index - 1] = lua_tonumber(L, 3);
      return 0;
    }

  default:
    return luaL_error(L, "Expected string or index for vec4 member, got type %s",
                      lua_typename(L, type));
  }
}



int script_index_vec4(lua_State *L)
{
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TSTRING: {
      size_t len = 0;
      const char *lstr = lua_tolstring(L, 2, &len);
      if (len == 1) {
        vec4f_t &v = lua_tovec4(L, 1);
        switch (lstr[0]) {
        case 'x':
          lua_pushnumber(L, v.x);
          return 1;
        case 'y':
          lua_pushnumber(L, v.y);
          return 1;
        case 'z':
          lua_pushnumber(L, v.z);
          return 1;
        case 'w':
          lua_pushnumber(L, v.w);
          return 1;
        default:
          return luaL_error(L, "Expected x, y, z, w, or a member function, got %s", lstr);
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
      if (comp_index < 1 || 4 < comp_index)
        return luaL_error(L, "Index out of range for vec4 [1..4]: %d", comp_index);

      lua_pushnumber(L, lua_tovec4(L, 1)[comp_index - 1]);
      return 1;
    }

  default:
    return luaL_error(L, "Expected string or index for vec4 member, got type %s",
                      lua_typename(L, type));
  }
}



int script_vec4_normalize(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.normalize();
  lua_settop(L, 1);
  return 1;
}



int script_vec4_normalized(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.normalized());
  return 1;
}



int script_vec4_magnitude(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushnumber(L, v.magnitude());
  return 1;
}



int script_vec4_length(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushnumber(L, v.length());
  return 1;
}



int script_vec4_difference(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.difference(lua_tovec4(L, 2)));;
  return 1;
}



int script_vec4_subtract(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.subtract(lua_tovec4(L, 2));
  lua_settop(L, 1);
  return 1;
}



int script_vec4_sum(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.sum(lua_tovec4(L, 2)));;
  return 1;
}



int script_vec4_add(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.add(lua_tovec4(L, 2));
  lua_settop(L, 1);
  return 1;
}



int script_vec4_scaled(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    lua_pushvec4(L, v.scaled(lua_tonumber(L, 2)));
    return 1;
  case LUA_TUSERDATA:
    lua_pushvec4(L, v.scaled(lua_tovec4(L, 2)));;
    return 1;
  default:
    return luaL_error(L, "vec4:scaled - Expected number or vec4, got %s",
               lua_typename(L, type));
  }
}



int script_vec4_scale(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    lua_pushvec4(L, v.scaled(lua_tonumber(L, 2)));
    break;
  case LUA_TUSERDATA:
    lua_pushvec4(L, v.scaled(lua_tovec4(L, 2)));;
    break;
  default:
    return luaL_error(L, "vec4:scaled - Expected number or vec4, got %s",
               lua_typename(L, type));
  }
  lua_settop(L, 1);
  return 1;
}



int script_vec4_negated(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.negated());
  return 1;
}



int script_vec4_negate(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.negate();
  lua_settop(L, 1);
  return 1;
}



int script_vec4_inverse(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.inverse());
  return 1;
}



int script_vec4_invert(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.invert();
  lua_settop(L, 1);
  return 1;
}



int script_vec4_dot_product(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushnumber(L, v.dot_product(lua_tovec4(L, 2)));
  return 1;
}



int script_vec4_rotate_elems(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  v.rotate_elems();
  lua_settop(L, 1);
  return 1;
}



int script_vec4_rotated_elems(lua_State *L)
{
  vec4f_t &v = lua_tovec4(L, 1);
  lua_pushvec4(L, v.rotated_elems());
  return 1;
}



int script_vec4(lua_State *L)
{
  float x, y, z, w;
  x = luaL_optnumber(L, 1, 0.0);
  y = luaL_optnumber(L, 2, x);
  z = luaL_optnumber(L, 3, (x == y) ? x : 0.0);
  w = luaL_optnumber(L, 4, (x == y) ? x : 1.0);
  lua_pushvec4(L, {x, y, z, w});
  return 1;
}



void script_push_vec4_metatable(lua_State *L)
{
  if (luaL_newmetatable(L, METATABLE_NAME)) {
    lua_pushcfunction(L, script_gc_vec4);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, script_push_const_int<4>);
    lua_setfield(L, -2, "__len");
    lua_createtable(L, 0, 19); // method table
    lua_pushcfunction(L, script_vec4_normalize);
    lua_setfield(L, -2, "normalize");
    lua_pushcfunction(L, script_vec4_normalized);
    lua_setfield(L, -2, "normalized");
    lua_pushcfunction(L, script_vec4_magnitude);
    lua_setfield(L, -2, "magnitude");
    lua_pushcfunction(L, script_vec4_length);
    lua_setfield(L, -2, "length");
    lua_pushcfunction(L, script_vec4_difference);
    lua_setfield(L, -2, "difference");
    lua_pushcfunction(L, script_vec4_subtract);
    lua_setfield(L, -2, "subtract");
    lua_pushcfunction(L, script_vec4_sum);
    lua_setfield(L, -2, "sum");
    lua_pushcfunction(L, script_vec4_add);
    lua_setfield(L, -2, "add");
    lua_pushcfunction(L, script_vec4_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_vec4_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_vec4_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_vec4_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_vec4_negated);
    lua_setfield(L, -2, "negated");
    lua_pushcfunction(L, script_vec4_negate);
    lua_setfield(L, -2, "negate");
    lua_pushcfunction(L, script_vec4_inverse);
    lua_setfield(L, -2, "inverse");
    lua_pushcfunction(L, script_vec4_invert);
    lua_setfield(L, -2, "invert");
    lua_pushcfunction(L, script_vec4_dot_product);
    lua_setfield(L, -2, "dot_product");
    lua_pushcfunction(L, script_vec4_rotate_elems);
    lua_setfield(L, -2, "rotate_elems");
    lua_pushcfunction(L, script_vec4_rotated_elems);
    lua_setfield(L, -2, "rotated_elems");
    lua_pushcclosure(L, script_index_vec4, 1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, script_newindex_vec4, 0);
    lua_setfield(L, -2, "__newindex");
  }
}


} // namespace <anon>



/*==============================================================================
  lua_bind_vec4

    Binds the vec4 script API to the Lua state.
==============================================================================*/
void lua_bind_vec4(lua_State *L)
{
  script_push_vec4_metatable(L);
  lua_pop(L, 1);
  lua_register(L, "vec4", script_vec4);
}



void lua_pushvec4(lua_State *L, const vec4f_t &m)
{
  const vec4_pool_t::index_t pool_index = g_vec4_pool.reserve_with(m);
  vec4_pool_t::index_t *obj_box;
  obj_box = (vec4_pool_t::index_t *)lua_newuserdata(L, sizeof(pool_index));
  *obj_box = pool_index;
  luaL_setmetatable(L, METATABLE_NAME);
}



vec4f_t &lua_tovec4(lua_State *L, int index)
{
  return g_vec4_pool[extract_vec4_index(L, index)];
}



bool lua_isvec4(lua_State *L, int index)
{
  return (luaL_checkudata(L, index, METATABLE_NAME) != NULL);
}


} // namespace snow
