#ifndef __SNOW__TRANSFORM_HH__
#define __SNOW__TRANSFORM_HH__

#include "../../config.hh"
#include <snow/math/math3d.hh>
#include "component.hh"

namespace snow {


struct game_object_t;


struct S_EXPORT transform_t : public component_t<transform_t, TRANSFORM_COMPONENT>
{
  transform_t() = default;
  transform_t(game_object_t *obj);

/*******************************************************************************
*                               relative changes                               *
*******************************************************************************/

  void  move_relative(const vec3f_t &t);
  void  translate(const vec3f_t &t);
  void  scale(const vec3f_t &s);
  void  rotate(const mat3f_t &mat);
  void  rotate_quat(const quatf_t &quat);
  void  rotate_euler(float pitch, float yaw, float roll);
  void  rotate_euler(const vec3f_t &angles);

/*******************************************************************************
*                               absolute changes                               *
*******************************************************************************/

  void  set_translation(const vec3f_t &t);
  void  set_scale(const vec3f_t &s);
  void  set_rotation(const mat3f_t &mat);
  void  set_rotation_quat(const quatf_t &quat);
  void  set_rotation_euler(float pitch, float yaw, float roll);
  void  set_rotation_euler(const vec3f_t &angles);

/*******************************************************************************
*                                   getters                                    *
*******************************************************************************/

  auto  translation() const -> vec3f_t;
  auto  scale() const -> vec3f_t;
  auto  rotation() const -> const mat3f_t &;
  auto  pitch() const -> float;
  auto  yaw() const   -> float;
  auto  roll() const  -> float;
  auto  rotation_euler() const -> vec3f_t;

/*******************************************************************************
*                        special case getters/modifiers                        *
*******************************************************************************/

  auto  transformed(const transform_t &other) const -> transform_t;
  auto  transform(const transform_t &other)         -> transform_t &;

  auto  local_mat4() const -> mat4f_t;
  auto  world_mat4() const -> mat4f_t;

private:
  mat3f_t  rotation_;
  vec3f_t  scale_;
  vec3f_t  translation_;
};


} // namespace snow

#endif /* end __SNOW_COMMON__TRANSFORM_HH__ include guard */
