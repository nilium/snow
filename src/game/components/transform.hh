/*
  transform.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__TRANSFORM_HH__
#define __SNOW__TRANSFORM_HH__

#include "../../config.hh"
#include <snow/math/math3d.hh>
#include "component.hh"

namespace snow {


struct game_object_t;


struct S_EXPORT transform_t : public component_t<transform_t, TRANSFORM_COMPONENT>
{
  DECL_COMPONENT_CTOR_DTOR(transform_t);

/*******************************************************************************
*                               relative changes                               *
*******************************************************************************/

  void            move_relative(const vec3f_t &t);
  void            translate(const vec3f_t &t);
  void            scale(const vec3f_t &s);
  void            rotate(const mat3f_t &mat);
  void            rotate_quat(const quatf_t &quat);
  void            rotate_euler(float pitch, float yaw, float roll);
  void            rotate_euler(const vec3f_t &angles);

/*******************************************************************************
*                               absolute changes                               *
*******************************************************************************/

  void            set_translation(const vec3f_t &t);
  void            set_scale(const vec3f_t &s);
  void            set_rotation(const mat3f_t &mat);
  void            set_rotation_quat(const quatf_t &quat);
  void            set_rotation_euler(float pitch, float yaw, float roll);
  void            set_rotation_euler(const vec3f_t &angles);

/*******************************************************************************
*                                   getters                                    *
*******************************************************************************/

  const vec3f_t & translation() const;
  const vec3f_t & scale() const;
  const mat3f_t & rotation() const;
  float           pitch() const;
  float           yaw() const  ;
  float           roll() const ;
  vec3f_t         rotation_euler() const;

/*******************************************************************************
*                        special case getters/modifiers                        *
*******************************************************************************/

  transform_t     transformed(const transform_t &other) const;
  transform_t &   transform(const transform_t &other);

  mat4f_t         local_mat4() const;
  mat4f_t         world_mat4() const;


  mat3f_t         rotation_;
  vec3f_t         scale_;
  vec3f_t         translation_;
};


} // namespace snow

#endif /* end __SNOW_COMMON__TRANSFORM_HH__ include guard */
