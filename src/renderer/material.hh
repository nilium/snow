/*
  material.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__MATERIAL_HH__
#define __SNOW__MATERIAL_HH__

#include "../config.hh"
#include "sgl.hh"
#include <snow/math/math3d.hh>


namespace snow {


struct rtexture_t;
struct rprogram_t;


enum : size_t
{
  MAX_CUSTOM_UNIFORMS = 4
};


/*! Value for a particular uniform. */
struct rcustom_uniform_t
{
  /*! The possible kinds of uniforms that can be loaded. */
  enum kind_t : unsigned {
    FLOAT,
    VEC2F,
    VEC3F,
    VEC4F,
    INT,
    VEC2I,
    VEC3I,
    VEC4I,
    UNSIGNED,
    VEC2U,
    VEC3U,
    VEC4U,
    MAT2F,
    MAT3F,
    MAT4F,
    // MAT23F,
    // MAT24F,
    // MAT32F,
    // MAT42F,
    // MAT34F,
    // MAT43F,
  };


  rcustom_uniform_t();
  explicit rcustom_uniform_t(GLfloat value);
  explicit rcustom_uniform_t(const vec2f_t &value);
  explicit rcustom_uniform_t(const vec3f_t &value);
  explicit rcustom_uniform_t(const vec4f_t &value);
  explicit rcustom_uniform_t(GLint value);
  explicit rcustom_uniform_t(const vec2_t<GLint> &value);
  explicit rcustom_uniform_t(const vec3_t<GLint> &value);
  explicit rcustom_uniform_t(const vec4_t<GLint> &value);
  explicit rcustom_uniform_t(GLuint value);
  explicit rcustom_uniform_t(const vec2_t<GLuint> &value);
  explicit rcustom_uniform_t(const vec3_t<GLuint> &value);
  explicit rcustom_uniform_t(const vec4_t<GLuint> &value);
  explicit rcustom_uniform_t(const mat3f_t &value);
  explicit rcustom_uniform_t(const mat4f_t &value);
  // FIXME: Remove opaque uniforms or come up with a safer solution to them
  /*! \warning If a uniform using this ctor is set, the data must be allocated
  and remain allocated for the duration of the uniform. The uniform object will
  not copy or free it for you. */
  explicit rcustom_uniform_t(kind_t kind, GLsizei count, void *data);

  GLsizei count;
  kind_t kind;
  bool is_opaque;
  union {
    void *opaque;
    GLfloat float_value;
    vec2f_t vec2f;
    vec3f_t vec3f;
    vec4f_t vec4f;
    GLint int_value;
    vec2_t<GLint> vec2i;
    vec3_t<GLint> vec3i;
    vec4_t<GLint> vec4i;
    GLuint unsigned_value;
    vec2_t<GLuint> vec2u;
    vec3_t<GLuint> vec3u;
    vec4_t<GLuint> vec4u;
    mat3f_t mat3;
    mat4f_t mat4;
  };

  void apply(const rprogram_t &program, int name, GLint uniform_location_hint) const;
};


/*!
  A description of a particular pass's GL state, including blend, depth, and
  stencil state as well as texture units, shader programs, and their uniforms.
*/
struct rpass_t
{
  static constexpr const size_t MAX_TEXTURE_UNITS = 8;

  bool skip;

  /* Note: never disables GL_BLEND. */
  struct blend_state_t {
    GLenum sfactor;
    GLenum dfactor;
  } blend;

  struct depth_state_t {
    GLenum func;
    GLboolean write;
  } depth;

  struct stencil_state_t {
    GLuint mask;
    GLenum func;
    GLint ref;
    GLuint ref_mask;
    GLenum fail;
    GLenum depth_fail;
    GLenum depth_pass;
  } stencil;

  rprogram_t *program;

  // Corresponds to 8 texture units
  struct texture_unit_state_t {
    rtexture_t *texture;
    GLint min_filter;  // Defaults to GL_LINEAR
    GLint mag_filter;  // Defaults to GL_LINEAR
    GLint x_wrap;
    GLint y_wrap;
  } textures[MAX_TEXTURE_UNITS];

  void apply() const;
  static const rpass_t &defaults();
  static void reset_pass_state();
};


struct rmaterial_t final
{
  rmaterial_t();
  ~rmaterial_t();

  // Must return true if all passes can be prepared, false otherwise. The
  // default implementation returns true.
  // FIXME: Remove use of valid()
  bool valid() const;

  size_t num_passes() const;
  void set_num_passes(size_t num);
  /*!
    Prepares to draw a given pass in the material. Returns true if the pass
    should go ahead, otherwise returns false.

    This isn't much different from getting the pass yourself and applying it.
  */
  bool prepare_pass(size_t passnum) const;

  // Direct access to passes, either for creating materials or to inspect any
  // of the material passes.
  rpass_t &pass(size_t pass);
  const rpass_t &pass(size_t pass) const;

  /*!
    \note set_projection, set_modelview, and set_texture_matrix are all just
    convenience functions around set_uniform. Check constants.hh for uniform
    names.
  */
  //! Sets the projection matrix for all materials.
  // \see set_uniform
  static void set_projection(const mat4f_t &proj);
  //! Sets the modelview matrix for all materials
  // \see set_uniform
  static void set_modelview(const mat4f_t &mv);
  //! Sets the texture matrix for all materials
  // \see set_uniform
  static void set_texture_matrix(const mat4f_t &tm);

  //! Copies the provided uniform and sets it as the uniform value for all
  // shaders using that uniform name.
  static void set_uniform(int name, const rcustom_uniform_t &uniform);
  //! Clears a uniform.
  static void unset_uniform(int name);
  static void clear_uniforms();
  //! For a given program, calls the appropriate glUniform* function for each
  // uniform the program recognizes.
  static void apply_uniforms(const rprogram_t &program);

private:
  // FIXME: consider doing two passes while parsing arrays and just allocating
  // the pass data at the end of the object.
  static constexpr const size_t MAX_PASSES = 4;
  size_t num_passes_ = 0;
  rpass_t passes_[MAX_PASSES] = {
    rpass_t::defaults(),
    rpass_t::defaults(),
    rpass_t::defaults(),
    rpass_t::defaults()
  };
};


} // namespace snow

#endif /* end __SNOW__MATERIAL_HH__ include guard */
