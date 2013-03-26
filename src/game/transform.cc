#include "transform.hh"
#include <cmath>

namespace snow {


void transform_t::move_relative(const vec3_t &t)
{
  set_translation(translation_ + rotation_ * t);
}



void transform_t::translate(const vec3_t &t)
{
  set_translation(translation_ + t);
}



void transform_t::scale(const vec3_t &s)
{
  set_scale(scale_ * s);
}


void transform_t::rotate(const mat3_t &mat)
{
  set_rotation(rotation_ * mat);
}



void transform_t::rotate_quat(const quat_t &quat)
{
  set_rotation(rotation_ * mat3_t::from_quat(quat));
}



void transform_t::rotate_euler(elem_t pitch, elem_t yaw, elem_t roll)
{
  set_rotation(quat_t::from_angle_axis(this->yaw() + yaw, vec3_t::pos_Y) *
               quat_t::from_angle_axis(this->pitch() + pitch, vec3_t::pos_X) *
               quat_t::from_angle_axis(this->roll() + roll, vec3_t::pos_Z));
}



void transform_t::rotate_euler(const vec3_t &angles)
{
  rotate_euler(angles.x, angles.y, angles.z);
}



void transform_t::set_translation(const vec3_t &t)
{
  translation_ = t;
}



void transform_t::set_scale(const vec3_t &s)
{
  scale_ = s;
}



void transform_t::set_rotation(const mat3_t &mat)
{
  rotation_ = mat;
}



void transform_t::set_rotation_quat(const quat_t &quat)
{
  set_rotation(mat3_t::from_quat(quat));
}



void transform_t::set_rotation_euler(elem_t pitch, elem_t yaw, elem_t roll)
{
  set_rotation(quat_t::from_angle_axis(yaw, vec3_t::pos_Y) *
               quat_t::from_angle_axis(pitch, vec3_t::pos_X) *
               quat_t::from_angle_axis(roll, vec3_t::pos_Z));
}



void transform_t::set_rotation_euler(const vec3_t &angles)
{
  set_rotation_euler(angles.x, angles.y, angles.z);
}



auto transform_t::translation() const -> vec3_t
{
  return translation_;
}



auto transform_t::scale() const -> vec3_t
{
  return scale_;
}



auto transform_t::rotation() const -> mat3_t
{
  return rotation_;
}



auto transform_t::rotation_euler() const -> vec3_t
{
  return {
    pitch(),
    yaw(),
    roll()
  };
}



auto transform_t::pitch() const -> elem_t
{
  return std::atan2(rotation_.t.y,
                    std::sqrt(rotation_.t.x * rotation_.t.x +
                              rotation_.t.z * rotation_.t.z));
}



auto transform_t::yaw() const -> elem_t
{
  return -std::atan2(rotation_.t.x, rotation_.t.z);
}



auto transform_t::roll() const -> elem_t
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



auto transform_t::to_matrix() const -> mat4_t
{
  return (mat4_t::translation(translation_) *
          mat4_t::scaling(scale_) *
          (mat4_t)rotation_);
}


} // namespace snow
