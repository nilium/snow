/*
  transform.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "transform.hh"
#include "../gameobject.hh"
#include <cmath>

namespace snow {


DEFINE_COMPONENT_CTOR_DTOR(transform_t);



void transform_t::move_relative(const vec3f_t &t)
{
  set_translation(translation_ + rotation_ * t);
}



void transform_t::translate(const vec3f_t &t)
{
  set_translation(translation_ + t);
}



void transform_t::scale(const vec3f_t &s)
{
  set_scale(scale_ * s);
}


void transform_t::rotate(const mat3f_t &mat)
{
  set_rotation(rotation_ * mat);
}



void transform_t::rotate_quat(const quatf_t &quat)
{
  set_rotation(rotation_ * mat3f_t::from_quat(quat));
}



void transform_t::rotate_euler(float pitch, float yaw, float roll)
{
  set_rotation(quatf_t::from_angle_axis(this->yaw() + yaw, vec3f_t::pos_Y) *
               quatf_t::from_angle_axis(this->pitch() + pitch, vec3f_t::pos_X) *
               quatf_t::from_angle_axis(this->roll() + roll, vec3f_t::pos_Z));
}



void transform_t::rotate_euler(const vec3f_t &angles)
{
  rotate_euler(angles.x, angles.y, angles.z);
}



void transform_t::set_translation(const vec3f_t &t)
{
  translation_ = t;
}



void transform_t::set_scale(const vec3f_t &s)
{
  scale_ = s;
}



void transform_t::set_rotation(const mat3f_t &mat)
{
  rotation_ = mat;
}



void transform_t::set_rotation_quat(const quatf_t &quat)
{
  set_rotation(mat3f_t::from_quat(quat));
}



void transform_t::set_rotation_euler(float pitch, float yaw, float roll)
{
  set_rotation(quatf_t::from_angle_axis(yaw, vec3f_t::pos_Y) *
               quatf_t::from_angle_axis(pitch, vec3f_t::pos_X) *
               quatf_t::from_angle_axis(roll, vec3f_t::pos_Z));
}



void transform_t::set_rotation_euler(const vec3f_t &angles)
{
  set_rotation_euler(angles.x, angles.y, angles.z);
}



auto transform_t::translation() const -> const vec3f_t &
{
  return translation_;
}



auto transform_t::scale() const -> const vec3f_t &
{
  return scale_;
}



auto transform_t::rotation() const -> const mat3f_t &
{
  return rotation_;
}



auto transform_t::rotation_euler() const -> vec3f_t
{
  return {
    pitch(),
    yaw(),
    roll()
  };
}



auto transform_t::pitch() const -> float
{
  return std::atan2(rotation_.t.y,
                    std::sqrt(rotation_.t.x * rotation_.t.x +
                              rotation_.t.z * rotation_.t.z));
}



auto transform_t::yaw() const -> float
{
  return -std::atan2(rotation_.t.x, rotation_.t.z);
}



auto transform_t::roll() const -> float
{
  return std::atan2(rotation_.r.y, rotation_.s.y);
}



transform_t transform_t::transformed(const transform_t &other) const
{
  transform_t r;
  r.set_rotation(rotation_ * other.rotation_);
  r.set_translation(rotation_ * other.translation_ + translation_);
  r.set_scale(scale_ * other.scale_);
  return r;
}



transform_t &transform_t::transform(const transform_t &other)
{
  set_rotation(rotation_ * other.rotation_);
  set_translation(rotation_ * other.translation_ + translation_);
  return *this;
}



auto transform_t::local_mat4() const -> mat4f_t
{
  return mat4f_t::look_at(vec3f_t::zero, vec3f_t::zero, vec3f_t::zero);
  // return (mat4f_t::translation(translation_) *
          // mat4f_t::scaling(scale_) *
          // (mat4f_t)rotation_);
}



auto transform_t::world_mat4() const -> mat4f_t
{
  mat4f_t result = local_mat4();
  game_object_t *parent = game_object ? game_object->parent() : nullptr;

  while (parent) {
    const transform_t *parent_tform = parent->get_component<transform_t>();
    if (parent_tform) {
      result = parent_tform->local_mat4() * result;
    }
  }

  return result;
}


} // namespace snow
