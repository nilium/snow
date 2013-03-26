#ifndef __SNOW__TRANSFORM_HH__
#define __SNOW__TRANSFORM_HH__

#include <snow-common.hh>
#include <snow/math/math3d.hh>

namespace snow {


struct S_EXPORT transform_t
{
  using elem_t = float;
  using mat4_t = mat4f_t;
  using mat3_t = mat3f_t;
  using vec3_t = vec3f_t;
  using vec4_t = vec4f_t;
  using quat_t = quatf_t;

  transform_t() = default;
  transform_t(const transform_t &other) = default;
  transform_t &operator = (const transform_t &other) = default;
  virtual ~transform_t() = default;

/*******************************************************************************
*                               relative changes                               *
*******************************************************************************/

  virtual void  move_relative(const vec3_t &t);
  virtual void  translate(const vec3_t &t);
  virtual void  scale(const vec3_t &s);
  virtual void  rotate(const mat3_t &mat);
  virtual void  rotate_quat(const quat_t &quat);
  virtual void  rotate_euler(elem_t pitch, elem_t yaw, elem_t roll);
  virtual void  rotate_euler(const vec3_t &angles);

/*******************************************************************************
*                               absolute changes                               *
*******************************************************************************/

  virtual void  set_translation(const vec3_t &t);
  virtual void  set_scale(const vec3_t &s);
  virtual void  set_rotation(const mat3_t &mat);
  virtual void  set_rotation_quat(const quat_t &quat);
  virtual void  set_rotation_euler(elem_t pitch, elem_t yaw, elem_t roll);
  virtual void  set_rotation_euler(const vec3_t &angles);

/*******************************************************************************
*                                   getters                                    *
*******************************************************************************/

  virtual auto  translation() const -> vec3_t;
  virtual auto  scale() const -> vec3_t;
  virtual auto  rotation() const -> mat3_t;
  virtual auto  pitch() const -> elem_t;
  virtual auto  yaw() const   -> elem_t;
  virtual auto  roll() const  -> elem_t;
  virtual auto  rotation_euler() const -> vec3_t;

/*******************************************************************************
*                        special case getters/modifiers                        *
*******************************************************************************/

  virtual auto  transformed(const transform_t &other) const -> transform_t;
  virtual auto  transform(const transform_t &other)         -> transform_t &;
  virtual auto  to_matrix() const -> mat4_t;

private:
  mat3_t  rotation_;
  vec3_t  scale_;
  vec3_t  translation_;
};


} // namespace snow

#endif /* end __SNOW_COMMON__TRANSFORM_HH__ include guard */
