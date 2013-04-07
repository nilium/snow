#ifndef __SNOW__GL_STATE_HH__
#define __SNOW__GL_STATE_HH__

#include <snow/config.hh>
#include "sgl.hh"
#include <vector>
#include <utility>
#include <array>
#include <set>

namespace snow {


/*==============================================================================

  GL state object, mainly used to do error handling and save some state client-
  side. Only handles a small subset of GL state.

==============================================================================*/
struct S_EXPORT gl_state_t
{
  gl_state_t() = default;
  ~gl_state_t() = default;

  gl_state_t(const gl_state_t &other) = delete;
  gl_state_t &operator = (const gl_state_t &other) = delete;

  gl_state_t(gl_state_t &&other) = delete;
  gl_state_t &operator = (gl_state_t &&other) = delete;


  inline bool operator == (const gl_state_t &other) const { return this == &other; }


  // Note: requires T and Q to be friends of gl_state_t.
  template <typename T, typename Q>
  static bool compatible(const T& lhs, const Q& rhs)
  {
    return lhs.state_ == rhs.state_;
  }

  // Note: requires T to be a friend of gl_state_t.
  template <typename T>
  static bool compatible(const T& lhs, const gl_state_t& rhs)
  {
    return lhs.state_ == rhs;
  }

  // Note: requires T to be a friend of gl_state_t.
  template <typename T>
  static bool compatible(const gl_state_t& lhs, const T& rhs)
  {
    return lhs == rhs.state_;
  }

  // Note: requires T to be a friend of gl_state_t.
  inline static bool compatible(const gl_state_t& lhs, const gl_state_t& rhs)
  {
    return lhs == rhs;
  }


  // Call to initialize the state object.
  void      acquire();


/*******************************************************************************
*                                 System Info                                  *
*******************************************************************************/
public:
  using version_t = std::pair<GLint, GLint>;

private:
  string version_str_;
  string vendor_;
  string renderer_;
  std::set<string> extensions_;
  string glsl_version_str_;
  version_t version_ { 1, 0 };
  version_t glsl_version_ { 1, 0 };
  std::array<bool, SGL_EXTENSION_COUNT> has_extension_;

  void      acquire_system_info();

public:

  // All using trailing return type because the return types are rather long
  // GL_VENDOR
  auto      vendor() const              -> const string &;
  // GL_RENDERER
  auto      renderer() const            -> const string &;
  // GL_VERSION
  auto      version_string() const      -> const string &;
  // GL_MAJOR_VERSION, GL_MINOR_VERSION
  auto      version() const             -> const version_t &;
  // GL_SHADING_LANGUAGE_VERSION
  auto      glsl_version_string() const -> const string &;
  // GL_SHADING_LANGUAGE_VERSION x.y
  auto      glsl_version() const        -> const version_t &;
  // set of GL_EXTENSIONS results
  auto      extensions() const          -> const std::set<string> &;

  // GL_STENCIL_BITS
  // auto      stencil_bits() const        -> GLint;


/*******************************************************************************
*                              Extension checking                              *
*******************************************************************************/
  // Fast GL extension check (uses extension constant)
  auto      has_extension(const sgl_extension_t extension) const -> bool;
  // Slower GL extension check (searches set). Useful only if the extension is
  // not part of sgl_extension_t already.
  auto      has_extension(const string &extension) const -> bool;


/*******************************************************************************
*                               Attribute Arrays                               *
*******************************************************************************/
private:
  GLint max_vertex_attribs_ = 16;
  GLuint array_object_ = 0;

  void      acquire_attrib_state();

public:
  GLint     max_vertex_attribs() const;
  // Note: uses glGetVertexAttrib to check if an array is enabled. This should
  // only be used in debugging.
  bool      is_attrib_array_enabled(const GLuint index) const;
  void      set_attrib_array_enabled(const GLuint index, const bool enabled);

  GLuint    vertex_array() const;
  void      bind_vertex_array(const GLuint vao);


/*******************************************************************************
*                                Texture State                                 *
*******************************************************************************/
private:
  GLint max_texture_size_ = 1024;
  GLint max_texture_units_ = 16;
  GLenum active_texture_ = GL_TEXTURE0;
  struct texbinding_t { std::array<GLuint, SGL_TEXTURE_TARGET_COUNT> binding; };
  std::vector<texbinding_t> texture_bindings_;

  void      acquire_texture_state();

public:
  GLint     max_texture_units() const;
  GLenum    active_texture() const;
  void      set_active_texture(const GLenum unit);

  GLuint    texture_binding(const GLenum target ) const;
  void      bind_texture(const GLenum target, const GLuint texture);

  GLint     max_texture_size() const;

  static
  bool      can_create_texture(const GLint level, const GLint internalFormat,
                               const GLsizei width, const GLsizei height);



/*******************************************************************************
*                                 Shader State                                 *
*******************************************************************************/
private:
  GLuint program_ = 0;

  void      acquire_shader_state();

public:
  GLuint    shader_program() const;
  void      use_program(const GLuint program);



/*******************************************************************************
*                                 Buffer State                                 *
*******************************************************************************/
private:
  GLuint renderbuffer_ = 0;
  GLuint fb_read_ = 0;
  GLuint fb_draw_ = 0;

  void      acquire_buffer_state();
  GLuint &  binding_for_target(GLenum target);

public:
  // Note: uses glGet* for all bindings that're valid. This means that if there
  // is no _BINDING equivalent for the target, this function returns zero.
  GLuint    buffer_binding(const GLenum target) const;
  void      bind_buffer(const GLenum target, const GLuint buffer);

  GLuint    renderbuffer(const GLenum target) const;
  void      bind_renderbuffer(const GLenum target, const GLuint buffer);

  // Note: framebuffer() will throw an exception if you do not request either
  // the draw or read framebuffers.
  GLuint    framebuffer(const GLenum target) const;
  void      bind_framebuffer(const GLenum target, const GLuint buffer);



/*******************************************************************************
*                                 Blend State                                  *
*******************************************************************************/
private:
  struct blendfunc_t {
    GLenum src_color;
    GLenum src_alpha;
    GLenum dst_color;
    GLenum dst_alpha;
  } blendfunc_;

  struct blendeq_t {
    GLenum color;
    GLenum alpha;
  } blendeq_;

  void      acquire_blend_state();

public:
  void      blend_func(GLenum &src_color, GLenum &src_alpha,
                       GLenum &dst_color, GLenum &dst_alpha) const;

  void      set_blend_func(GLenum src_both, GLenum dst_both);
  void      set_blend_func(GLenum src_color, GLenum src_alpha,
                           GLenum dst_color, GLenum dst_alpha);

  void      blend_equation(GLenum &color, GLenum &alpha) const;

  void      set_blend_equation(GLenum mode);
  void      set_blend_equation(GLenum color, GLenum alpha);
};


} // namespace snow

#endif /* end __SNOW__GL_STATE_HH__ include guard */
