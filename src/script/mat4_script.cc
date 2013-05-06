#include "mat4_script.hh"
#include "vec3_script.hh"
#include "vec4_script.hh"
// #include "quat_script.hh"
#include "math_metatables.hh"
#include <snow/types/object_pool.hh>
#include <vector>

namespace snow {


namespace {


using mat4_pool_t   = object_pool_t<mat4f_t>;
mat4_pool_t         g_mat4_pool;
const char *        METATABLE_NAME = MAT4_METATABLE_NAME;


// extract mat4 pool index from udata at index
inline mat4_pool_t::index_t extract_mat4_index(lua_State *L, int index);
// mat4 metatable __gc function
int script_gc_mat4(lua_State *L);
// Script API
int script_mat4_translate(lua_State *L);
int script_mat4_translated(lua_State *L);
int script_mat4_transpose(lua_State *L);
int script_mat4_transposed(lua_State *L);
int script_mat4_rowvec4(lua_State *L);
int script_mat4_colvec4(lua_State *L);
int script_mat4_set_rowvec4(lua_State *L);
int script_mat4_set_colvec4(lua_State *L);
int script_mat4_rowvec3(lua_State *L);
int script_mat4_colvec3(lua_State *L);
int script_mat4_set_rowvec3(lua_State *L);
int script_mat4_set_colvec3(lua_State *L);
int script_mat4_negated(lua_State *L);
int script_mat4_negate(lua_State *L);
int script_mat4_sum(lua_State *L);
int script_mat4_sum(lua_State *L);
int script_mat4_add(lua_State *L);
int script_mat4_add(lua_State *L);
int script_mat4_difference(lua_State *L);
int script_mat4_difference(lua_State *L);
int script_mat4_subtract(lua_State *L);
int script_mat4_subtract(lua_State *L);
int script_mat4_scaled(lua_State *L);
int script_mat4_scale(lua_State *L);
int script_mat4_scaled(lua_State *L);
int script_mat4_scale(lua_State *L);
int script_mat4_scaled(lua_State *L);
int script_mat4_scale(lua_State *L);
int script_mat4_inverse_orthogonal(lua_State *L);
int script_mat4_inverse_affine(lua_State *L);
int script_mat4_inverse_general(lua_State *L);
int script_mat4_cofactor(lua_State *L);
int script_mat4_determinant(lua_State *L);
int script_mat4_adjoint(lua_State *L);
int script_mat4_product(lua_State *L);
int script_mat4_multiply(lua_State *L);
int script_mat4_multiply(lua_State *L);
int script_mat4_multiply(lua_State *L);
int script_mat4_rotate(lua_State *L);
int script_mat4_inverse_rotate(lua_State *L);
// Push mat4 metatable
void script_push_mat4_metatable(lua_State *L);



inline mat4_pool_t::index_t extract_mat4_index(lua_State *L, int index)
{
  const mat4_pool_t::index_t *mat_box;
#ifndef NDEBUG
  mat_box = (const mat4_pool_t::index_t *)luaL_checkudata(L, index, METATABLE_NAME);
#else
  mat_box = (const mat4_pool_t::index_t *)lua_touserdata(L, index);
#endif
  const mat4_pool_t::index_t mat_index = *mat_box;
  return mat_index;
}



int script_gc_mat4(lua_State *L)
{
  const mat4_pool_t::index_t mat_index = extract_mat4_index(L, 1);
  g_mat4_pool.collect(mat_index);
  return 0;
}



int script_mat4_translate(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.translate(lua_tovec3(L, 2));
  lua_settop(L, 1);
  return 1;
}



int script_mat4_translated(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushmat4(L, m.translated(lua_tovec3(L, 2)));
  return 1;
}



int script_mat4_transpose(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.transpose();
  lua_settop(L, 1);
  return 1;
}



int script_mat4_transposed(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushmat4(L, m.transposed());
  return 1;
}



int script_mat4_rowvec4(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushvec4(L, m.rowvec4(lua_tointeger(L, 2)));
  return 1;
}



int script_mat4_colvec4(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushvec4(L, m.colvec4(lua_tointeger(L, 2)));
  return 1;
}



int script_mat4_set_rowvec4(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.set_rowvec4(lua_tointeger(L, 2), lua_tovec4(L, 3));
  lua_settop(L, 1);
  return 1;
}



int script_mat4_set_colvec4(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.set_colvec4(lua_tointeger(L, 2), lua_tovec4(L, 3));
  lua_settop(L, 1);
  return 1;
}



int script_mat4_rowvec3(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushvec3(L, m.rowvec3(lua_tointeger(L, 2)));
  return 1;
}



int script_mat4_colvec3(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushvec3(L, m.colvec3(lua_tointeger(L, 2)));
  return 1;
}



int script_mat4_set_rowvec3(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.set_rowvec3(lua_tointeger(L, 2), lua_tovec3(L, 3));
  lua_settop(L, 1);
  return 1;
}



int script_mat4_set_colvec3(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.set_colvec3(lua_tointeger(L, 2), lua_tovec3(L, 3));
  lua_settop(L, 1);
  return 1;
}



int script_mat4_negated(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  lua_pushmat4(L, m.negated());
  return 1;
}



int script_mat4_negate(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.negate();
  lua_settop(L, 1);
  return 1;
}



int script_mat4_sum(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TUSERDATA:
    lua_pushmat4(L, m.sum(lua_tomat4(L, 2)));
    break;
  case LUA_TNUMBER:
    lua_pushmat4(L, m.sum(lua_tonumber(L, 2)));
    break;
  default:
    return luaL_error(L, "mat4:sum - Expected number or mat4, got %s", lua_typename(L, type));
  }
  return 1;
}



int script_mat4_add(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TUSERDATA:
    m.add(lua_tomat4(L, 2));
    break;
  case LUA_TNUMBER:
    m.add(lua_tonumber(L, 2));
    break;
  default:
    return luaL_error(L, "mat4:add - Expected number or mat4, got %s", lua_typename(L, type));
  }
  lua_settop(L, 1);
  return 1;
}



int script_mat4_difference(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TUSERDATA:
    lua_pushmat4(L, m.difference(lua_tomat4(L, 2)));
    break;
  case LUA_TNUMBER:
    lua_pushmat4(L, m.difference(lua_tonumber(L, 2)));
    break;
  default:
    return luaL_error(L, "mat4:difference - Expected number or mat4, got %s", lua_typename(L, type));
  }
  return 1;
}



int script_mat4_subtract(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TUSERDATA:
    m.subtract(lua_tomat4(L, 2));
    break;
  case LUA_TNUMBER:
    m.subtract(lua_tonumber(L, 2));
    break;
  default:
    return luaL_error(L, "mat4:subtract - Expected number or mat4, got %s", lua_typename(L, type));
  }
  lua_settop(L, 1);
  return 1;
}



int script_mat4_scaled(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    lua_pushmat4(L, m.scaled(lua_tonumber(L, 2)));
    break;
  case LUA_TUSERDATA:
    if (lua_ismat4(L, 2)) {
      lua_pushmat4(L, m.scaled(lua_tomat4(L, 2)));
      break;
    } else if (lua_isvec3(L, 2)) {
      lua_pushmat4(L, m.scaled(lua_tovec3(L, 2)));
      break;
    }
  default:
    return luaL_error(L, "mat4:scaled - Expected number, vec3, or mat4, got %s",
      lua_typename(L, type));
  }
  return 1;
}



int script_mat4_scale(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  const int type = lua_type(L, 2);
  switch (type) {
  case LUA_TNUMBER:
    m.scale(lua_tonumber(L, 2));
    break;
  case LUA_TUSERDATA:
    if (lua_ismat4(L, 2)) {
      m.scale(lua_tomat4(L, 2));
      break;
    } else if (lua_isvec3(L, 2)) {
      m.scale(lua_tovec3(L, 2));
      break;
    }
  default:
    return luaL_error(L, "mat4:scale - Expected number, vec3, or mat4, got %s",
      lua_typename(L, type));
  }
  lua_settop(L, 1);
  return 1;
}



#if 0
int script_mat4_scaled(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.scaled(const mat4_t &other) const
  return 1;
}



int script_mat4_scale(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.scale(const mat4_t &other)
  lua_settop(L, 1);
  return 1;
}



int script_mat4_scaled(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.scaled(const vec3 &vec) const
  return 1;
}



int script_mat4_scale(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.scale(const vec3 &vec)
  lua_settop(L, 1);
  return 1;
}



int script_mat4_inverse_orthogonal(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.inverse_orthogonal()
  return 1;
}



int script_mat4_inverse_affine(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.inverse_affine(mat4_t &out) const
  return 1;
}



int script_mat4_inverse_general(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.inverse_general(mat4_t &out) const
  return 1;
}



int script_mat4_cofactor(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.cofactor(int r0, int r1, int r2, int c0, int c1, int c2) const
  return 1;
}



int script_mat4_determinant(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.determinant() const
  return 1;
}



int script_mat4_adjoint(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.adjoint() const
  return 1;
}



int script_mat4_product(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.product(const mat4_t &other) const
  return 1;
}



int script_mat4_multiply(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.multiply(const mat4_t &other)
  lua_settop(L, 1);
  return 1;
}



int script_mat4_multiply(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.multiply(const vec4 &vec) const
  return 1;
}



int script_mat4_multiply(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.multiply(const vec3 &vec) const
  return 1;
}



int script_mat4_rotate(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.rotate(const vec3 &vec) const
  return 1;
}



int script_mat4_inverse_rotate(lua_State *L)
{
  mat4f_t &m = lua_tomat4(L, 1);
  m.inverse_rotate(const vec3 &vec) const
  return 1;
}

#endif



void script_push_mat4_metatable(lua_State *L)
{
  if (luaL_newmetatable(L, METATABLE_NAME)) {
    lua_pushcfunction(L, script_gc_mat4);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, script_push_const_int<16>);
    lua_setfield(L, -2, "__len");
    lua_createtable(L, 0, 40);
    lua_pushcfunction(L, script_mat4_translate);
    lua_setfield(L, -2, "translate");
    lua_pushcfunction(L, script_mat4_translated);
    lua_setfield(L, -2, "translated");
    lua_pushcfunction(L, script_mat4_transpose);
    lua_setfield(L, -2, "transpose");
    lua_pushcfunction(L, script_mat4_transposed);
    lua_setfield(L, -2, "transposed");
    lua_pushcfunction(L, script_mat4_rowvec4);
    lua_setfield(L, -2, "rowvec4");
    lua_pushcfunction(L, script_mat4_colvec4);
    lua_setfield(L, -2, "colvec4");
    lua_pushcfunction(L, script_mat4_set_rowvec4);
    lua_setfield(L, -2, "set_rowvec4");
    lua_pushcfunction(L, script_mat4_set_colvec4);
    lua_setfield(L, -2, "set_colvec4");
    lua_pushcfunction(L, script_mat4_rowvec3);
    lua_setfield(L, -2, "rowvec3");
    lua_pushcfunction(L, script_mat4_colvec3);
    lua_setfield(L, -2, "colvec3");
    lua_pushcfunction(L, script_mat4_set_rowvec3);
    lua_setfield(L, -2, "set_rowvec3");
    lua_pushcfunction(L, script_mat4_set_colvec3);
    lua_setfield(L, -2, "set_colvec3");
    lua_pushcfunction(L, script_mat4_negated);
    lua_setfield(L, -2, "negated");
    lua_pushcfunction(L, script_mat4_negate);
    lua_setfield(L, -2, "negate");
    lua_pushcfunction(L, script_mat4_sum);
    lua_setfield(L, -2, "sum");
#if 0
    lua_pushcfunction(L, script_mat4_add);
    lua_setfield(L, -2, "add");
    lua_pushcfunction(L, script_mat4_difference);
    lua_setfield(L, -2, "difference");
    lua_pushcfunction(L, script_mat4_subtract);
    lua_setfield(L, -2, "subtract");
    lua_pushcfunction(L, script_mat4_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_mat4_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_mat4_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_mat4_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_mat4_scaled);
    lua_setfield(L, -2, "scaled");
    lua_pushcfunction(L, script_mat4_scale);
    lua_setfield(L, -2, "scale");
    lua_pushcfunction(L, script_mat4_inverse_orthogonal);
    lua_setfield(L, -2, "inverse_orthogonal");
    lua_pushcfunction(L, script_mat4_inverse_affine);
    lua_setfield(L, -2, "inverse_affine");
    lua_pushcfunction(L, script_mat4_inverse_general);
    lua_setfield(L, -2, "inverse_general");
    lua_pushcfunction(L, script_mat4_cofactor);
    lua_setfield(L, -2, "cofactor");
    lua_pushcfunction(L, script_mat4_determinant);
    lua_setfield(L, -2, "determinant");
    lua_pushcfunction(L, script_mat4_adjoint);
    lua_setfield(L, -2, "adjoint");
    lua_pushcfunction(L, script_mat4_product);
    lua_setfield(L, -2, "product");
    lua_pushcfunction(L, script_mat4_multiply);
    lua_setfield(L, -2, "multiply");
    lua_pushcfunction(L, script_mat4_multiply);
    lua_setfield(L, -2, "multiply");
    lua_pushcfunction(L, script_mat4_multiply);
    lua_setfield(L, -2, "multiply");
    lua_pushcfunction(L, script_mat4_rotate);
    lua_setfield(L, -2, "rotate");
    lua_pushcfunction(L, script_mat4_inverse_rotate);
    lua_setfield(L, -2, "inverse_rotate");
#endif
    lua_setfield(L, -2, "__index");
  }
}


} // namespace <anon>



/*==============================================================================
  lua_bind_mat4

    Binds the mat4 script API to the Lua state.
==============================================================================*/
void lua_bind_mat4(lua_State *L)
{
  script_push_mat4_metatable(L);
  lua_pop(L, 1);
}



void lua_pushmat4(lua_State *L, const mat4f_t &m)
{
  const mat4_pool_t::index_t pool_index = g_mat4_pool.reserve(m);
  mat4_pool_t::index_t *obj_box;
  obj_box = (mat4_pool_t::index_t *)lua_newuserdata(L, sizeof(pool_index));
  *obj_box = pool_index;
  luaL_setmetatable(L, METATABLE_NAME);
}



mat4f_t &lua_tomat4(lua_State *L, int index)
{
  return g_mat4_pool[extract_mat4_index(L, index)];
}



bool lua_ismat4(lua_State *L, int index)
{
  return (luaL_checkudata(L, index, METATABLE_NAME) != NULL);
}


} // namespace snow
