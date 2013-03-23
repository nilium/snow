#include "gl_state.hh"
#include "gl_error.hh"
#include <iostream>

namespace snow {
namespace renderer {


namespace {

/*==============================================================================
  extract_version_pair

    Extracts a major/minor version pair from a GL version string. If the version
    string can't be parsed, it returns {-1, -1}.
==============================================================================*/
gl_state_t::version_t extract_version_pair(const string &version_str)
{
  gl_state_t::version_t result = {-1, -1};
  try {
    const auto first_dot = version_str.find('.');
    auto second_dot = version_str.find('.', first_dot + 1);
    if (second_dot == string::npos)
      second_dot = version_str.find(' ', first_dot + 1);

    if (first_dot == string::npos || first_dot == 0)
      return result;

    result.first = (GLint)std::stoi(version_str.substr(0, first_dot));

    if (second_dot == string::npos)
      result.second = (GLint)std::stoi(version_str.substr(first_dot + 1));
    else
      result.second = (GLint)std::stoi(version_str.substr(first_dot + 1, second_dot - first_dot));
  }
  catch (std::exception &e) {
    result.first = -1;
    result.second = -1;
  }

  return result;
}

} // namespace <anon>



/*==============================================================================
  acquire

    If the GL state object is out of sync with the actual OpenGL state, this
    can be called to acquire the current GL state. This will basically result
    in a lot of glGet* calls. Use it sparingly.

    This is not called by the constructor but must be called at least once to
    initialize the state object.
==============================================================================*/
void gl_state_t::acquire()
{
  #define SGL_ACQUIRE_ERROR(FUNC, LIT_MSG)      \
    try {                                       \
      FUNC;                                     \
    } catch (gl_error_t &error) {               \
      std::clog << LIT_MSG << std::endl;        \
      throw;                                    \
    }

  SGL_ACQUIRE_ERROR(acquire_system_info(),   "Acquiring system information");
  SGL_ACQUIRE_ERROR(acquire_shader_state(),  "Acquiring shader state");
  SGL_ACQUIRE_ERROR(acquire_attrib_state(),  "Resetting vertex attrib state");
  SGL_ACQUIRE_ERROR(acquire_texture_state(), "Acquiring texture state");
  SGL_ACQUIRE_ERROR(acquire_buffer_state(),  "Acquiring buffer state");
  SGL_ACQUIRE_ERROR(acquire_blend_state(),   "Acquiring blending state");

  #undef SGL_ACQUIRE_ERROR
}



/*==============================================================================
  acquire_system_info

    TODO: Description
==============================================================================*/
void gl_state_t::acquire_system_info()
{
  const char *string_out;

  string_out = (const char *)glGetString(GL_VERSION);
  assert_gl("Getting GL_VERSION");
  version_str_ = string(string_out);

  string_out = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
  assert_gl("Getting GL_SHADING_LANGUAGE_VERSION");
  glsl_version_str_ = string(string_out);

  string_out = (const char *)glGetString(GL_VENDOR);
  assert_gl("Getting GL_VENDOR");
  vendor_ = string(string_out);

  string_out = (const char *)glGetString(GL_RENDERER);
  assert_gl("Getting GL_RENDERER");
  renderer_ = string(string_out);

  GLint num_extensions = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
  extensions_.clear();
  for (GLuint ext_index = 0; ext_index < num_extensions; ++ext_index) {
    string_out = (const char *)glGetStringi(GL_EXTENSIONS, ext_index);
    assert_gl("Getting string from GL_EXTENSIONS");
    extensions_.emplace(string_out);
  }

  glGetIntegerv(GL_MAJOR_VERSION, &version_.first);
  assert_gl("Getting GL_MAJOR_VERSION");
  glGetIntegerv(GL_MINOR_VERSION, &version_.second);
  assert_gl("Getting GL_MINOR_VERSION");

  glsl_version_ = extract_version_pair(glsl_version_str_);

  #include "sgl_extension_test.tcc"
}



/*==============================================================================
  vendor

    TODO: Description
==============================================================================*/
const string &gl_state_t::vendor() const
{
  return vendor_;
}



/*==============================================================================
  renderer

    TODO: Description
==============================================================================*/
const string &gl_state_t::renderer() const
{
  return renderer_;
}



/*==============================================================================
  version_string

    TODO: Description
==============================================================================*/
const string &gl_state_t::version_string() const
{
  return version_str_;
}



/*==============================================================================
  version

    TODO: Description
==============================================================================*/
auto gl_state_t::version() const -> const version_t &
{
  return version_;
}



/*==============================================================================
  glsl_version_string

    TODO: Description
==============================================================================*/
const string &gl_state_t::glsl_version_string() const
{
  return glsl_version_str_;
}



/*==============================================================================
  glsl_version

    TODO: Description
==============================================================================*/
auto gl_state_t::glsl_version() const -> const version_t &
{
  return glsl_version_;
}



/*==============================================================================
  extensions

    TODO: Description
==============================================================================*/
const std::set<string> &gl_state_t::extensions() const
{
  return extensions_;
}



/*==============================================================================
  has_extension

    TODO: Description
==============================================================================*/
bool gl_state_t::has_extension(const sgl_extension_t extension) const
{
  if (SGL_EXTENSION_COUNT <= extension)
    throw std::invalid_argument("Invalid extension name");

  return has_extension_[extension];
}



/*==============================================================================
  has_extension

    TODO: Description
==============================================================================*/
bool gl_state_t::has_extension(const string &extension) const
{
  return (extensions_.find(extension) != extensions_.end());
}



/*==============================================================================
  acquire_attrib_state

    Resets the attribute buffer state to default values. This might contradict
    GL state if acquired in the middle of program execution, but otherwise is
    probably correct. The point is more to just to reduce future state changes
    overall.
==============================================================================*/
void gl_state_t::acquire_attrib_state()
{
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs_);
  assert_gl("Getting GL_MAX_VERTEX_ATTRIBS");

  array_object_ = 0;
}



/*==============================================================================
  max_vertex_attribs

    TODO: Description
==============================================================================*/
int gl_state_t::max_vertex_attribs() const
{
  return max_vertex_attribs_;
}



/*==============================================================================
  is_attrib_array_enabled

    TODO: Description
==============================================================================*/
bool gl_state_t::is_attrib_array_enabled(const GLuint index) const
{
  if (index < max_vertex_attribs_) {
    GLint enabled = GL_FALSE;
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    assert_gl("Checking if vertex attrib array is enabled");
    return enabled != GL_FALSE;
  } else {
    throw std::out_of_range("Invalid attribute index - out of range");
  }
}



/*==============================================================================
  set_attrib_array_enabled

    TODO: Description
==============================================================================*/
void gl_state_t::set_attrib_array_enabled(const GLuint index, const bool enabled)
{
  if (max_vertex_attribs_ <= index)
    throw std::out_of_range("Invalid attribute index - out of range");

  if (enabled) {
    glEnableVertexAttribArray(index);
    assert_gl("Enabling vertex attrib array");
  } else {
    glDisableVertexAttribArray(index);
    assert_gl("Disabling vertex attrib array");
  }
}



/*==============================================================================
  vertex_array

    Gets the currently bound vertex array object.
==============================================================================*/
GLuint gl_state_t::vertex_array() const
{
  return array_object_;
}



/*==============================================================================
  bind_vertex_array

    Sets the currently bound vertex array object.
==============================================================================*/
void gl_state_t::bind_vertex_array(const GLuint vao)
{
  if (array_object_ != vao) {
    glBindVertexArray(vao);
    assert_gl("Binding vertex array object");
    array_object_ = vao;
  }
}



/*==============================================================================
  acquire_texture_state

    TODO: Description
==============================================================================*/
void gl_state_t::acquire_texture_state()
{
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  assert_gl("Getting GL_MAX_TEXTURE_SIZE");
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units_);
  assert_gl("Getting GL_MAX_TEXTURE_IMAGE_UNITS");
  glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *)&active_texture_);
  assert_gl("Getting GL_ACTIVE_TEXTURE");

  texture_bindings_.resize(max_texture_units_);

  for (int index = 0; index < max_texture_units_; ++index) {
    for (int target = 0; target < SGL_TEXTURE_TARGET_COUNT; ++target) {
      GLenum gl_target = sgl_texture_target_to_gl_binding(target);
      if (gl_target != 0) {
        glActiveTexture(GL_TEXTURE0 + index);
        assert_gl("Tempoarily changing active texture unit");
        glGetIntegerv(gl_target, (GLint *)&texture_bindings_[index].binding[target]);
        assert_gl("Getting texture bindings");
      } else {
        texture_bindings_[index].binding[target] = 0;
      }
    }
  }
  glActiveTexture(active_texture_);
  assert_gl("Resetting active texture unit");
}



/*==============================================================================
  max_texture_units

    TODO: Description
==============================================================================*/
GLint gl_state_t::max_texture_units() const
{
  return max_texture_units_;
}



/*==============================================================================
  active_texture

    TODO: Description
==============================================================================*/
GLenum gl_state_t::active_texture() const
{
  return active_texture_;
}



/*==============================================================================
  set_active_texture

    TODO: Description
==============================================================================*/
void gl_state_t::set_active_texture(const GLenum unit)
{
  if (active_texture_ == unit)
    return;

  glActiveTexture(unit);
  assert_gl("Setting active texture unit");
  active_texture_ = unit;
}



/*==============================================================================
  texture_binding

    TODO: Description
==============================================================================*/
GLuint gl_state_t::texture_binding(const unsigned target) const
{
  const unsigned sgl_target = sgl_texture_target_to_gl(target);
  if (SGL_TEXTURE_TARGET_COUNT <= sgl_target)
    throw std::invalid_argument("Invalid texture target");
  return texture_bindings_[active_texture_].binding[sgl_target];
}



/*==============================================================================
  bind_texture

    TODO: Description
==============================================================================*/
void gl_state_t::bind_texture(const unsigned target, const GLuint texture)
{
  const unsigned sgl_target = sgl_texture_target_to_gl(target);
  if (SGL_TEXTURE_TARGET_COUNT <= sgl_target)
    throw std::invalid_argument("Invalid texture target");

  const GLuint last = texture_bindings_[active_texture_].binding[sgl_target];
  if (last != texture) {
    glBindTexture(target, texture);
    assert_gl("Setting texture binding");
    texture_bindings_[active_texture_].binding[sgl_target] = texture;
  }
}



/*==============================================================================
  max_texture_size

    TODO: Description
==============================================================================*/
GLint gl_state_t::max_texture_size() const
{
  return max_texture_size_;
}



/*==============================================================================
  can_create_texture

    TODO: Description
==============================================================================*/
bool gl_state_t::can_create_texture(const GLint level, const GLint internalFormat,
                                    const GLsizei width, const GLsizei height)
{
  GLint out_iformat = 0;
  glTexImage2D(GL_PROXY_TEXTURE_2D, level, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  assert_gl("Checking for texture support using 2D proxy texture");
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &out_iformat);
  assert_gl("Getting GL_TEXTURE_INTERNAL_FORMAT for 2D proxy texture");
  return (out_iformat == GL_RGBA);
}



/*==============================================================================
  acquire_shader_state

    TODO: Description
==============================================================================*/
void gl_state_t::acquire_shader_state()
{
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&program_);
  assert_gl("Getting GL_CURRENT_PROGRAM");
}



/*==============================================================================
  shader_program

    TODO: Description
==============================================================================*/
GLuint gl_state_t::shader_program() const
{
  return program_;
}



/*==============================================================================
  use_program

    TODO: Description
==============================================================================*/
void gl_state_t::use_program(const GLuint program)
{
  if (program_ != program) {
    glUseProgram(program);
    assert_gl("Setting new shader program");
    program_ = program;
  }
}



/*==============================================================================
  acquire_buffer_state

    TODO: Description
==============================================================================*/
void gl_state_t::acquire_buffer_state()
{
  // renderbuffer
  glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint *)&renderbuffer_);
  assert_gl("Getting GL_RENDERBUFFER_BINDING");

  // framebuffer
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint *)&fb_read_);
  assert_gl("Getting GL_READ_FRAMEBUFFER_BINDING");
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&fb_draw_);
  assert_gl("Getting GL_DRAW_FRAMEBUFFER_BINDING");
}



/*==============================================================================
  binding_for_target

    TODO: Description
==============================================================================*/
GLuint &gl_state_t::binding_for_target(GLenum target)
{
  switch (target) {
  case GL_READ_FRAMEBUFFER: return fb_read_;
  case GL_DRAW_FRAMEBUFFER: return fb_draw_;
  default: throw std::invalid_argument("Invalid framebuffer target");
  }
}



/*==============================================================================
  buffer_binding

    TODO: Description
==============================================================================*/
GLuint gl_state_t::buffer_binding(const unsigned target) const
{
  GLuint binding = 0;
  #define binding_ptr_ ((GLint *)&binding)
  switch (target) {
  case GL_ARRAY_BUFFER:
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, binding_ptr_);
    break;
  case GL_ELEMENT_ARRAY_BUFFER:
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, binding_ptr_);
    break;
  case GL_PIXEL_PACK_BUFFER:
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, binding_ptr_);
    break;
  case GL_PIXEL_UNPACK_BUFFER:
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, binding_ptr_);
    break;
#if GL_VERSION_3_0
  case GL_TRANSFORM_FEEDBACK_BUFFER:
    glGetIntegerv(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, binding_ptr_);
    break;
#endif
#if GL_VERSION_3_1 || GL_ARB_uniform_buffer_object
  case GL_UNIFORM_BUFFER:
    glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, binding_ptr_);
    break;
#endif
#if GL_VERSION_4_0 || GL_ARB_draw_indirect
  case GL_DRAW_INDIRECT_BUFFER:
    glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, binding_ptr_);
    break;
#endif
#if GL_VERSION_4_2 || GL_ARB_shader_atomic_counters
  case GL_ATOMIC_COUNTER_BUFFER:
    glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, binding_ptr_);
    break;
#endif
#if GL_VERSION_4_3 || GL_ARB_compute_shader
  case GL_DISPATCH_INDIRECT_BUFFER:
    glGetIntegerv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, binding_ptr_);
    break;
#endif
#if GL_VERSION_4_3 || GL_ARB_shader_storage_buffer_object
  case GL_SHADER_STORAGE_BUFFER:
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, binding_ptr_);
    break;
#endif
  default: break;
  }
  #undef binding_ptr_
  return binding;
}



/*==============================================================================
  bind_buffer

    TODO: Description
==============================================================================*/
void gl_state_t::bind_buffer(const unsigned target, const GLuint buffer)
{
  const unsigned sgl_target = sgl_buffer_target_from_gl(target);
  if (SGL_BUFFER_TARGET_COUNT <= sgl_target)
    throw std::invalid_argument("Invalid buffer target");

  glBindBuffer(target, buffer);
  assert_gl("Binding buffer");
}



/*==============================================================================
  renderbuffer

    TODO: Description
==============================================================================*/
GLuint gl_state_t::renderbuffer(const GLenum target) const
{
  if (target != GL_RENDERBUFFER)
    throw std::invalid_argument("Invalid renderbuffer target");
  return renderbuffer_;
}



/*==============================================================================
  bind_renderbuffer

    TODO: Description
==============================================================================*/
void gl_state_t::bind_renderbuffer(const GLenum target, const GLuint buffer)
{
  if (target != GL_RENDERBUFFER)
    throw std::invalid_argument("Invalid renderbuffer target");
  if (renderbuffer_ != buffer) {
    glBindRenderbuffer(target, buffer);
    assert_gl("Binding renderbuffer object");
    renderbuffer_ = buffer;
  }
}



/*==============================================================================
  framebuffer

    TODO: Description
==============================================================================*/
GLuint gl_state_t::framebuffer(const GLenum target) const
{
  switch (target) {
  case GL_READ_FRAMEBUFFER: return fb_read_;
  case GL_DRAW_FRAMEBUFFER: return fb_draw_;
  default: throw std::invalid_argument("Invalid framebuffer target");
  }
}



/*==============================================================================
  bind_framebuffer

    TODO: Description
==============================================================================*/
void gl_state_t::bind_framebuffer(const GLenum target, const GLuint buffer)
{
  if (target != GL_FRAMEBUFFER) {
    GLuint &binding = binding_for_target(target);

    if (binding != buffer) {
      glBindFramebuffer(target, buffer);
      assert_gl("Binding framebuffer object (separate)");
      binding = buffer;
    }
  } else if (buffer != fb_draw_ || buffer != fb_read_) {
    glBindFramebuffer(target, buffer);
    assert_gl("Binding framebuffer object (combined)");
    fb_draw_ = fb_read_ = buffer;
  }
}



/*==============================================================================
  acquire_blend_state

    TODO: Description
==============================================================================*/
void gl_state_t::acquire_blend_state()
{
  glGetIntegerv(GL_BLEND_SRC_RGB,        (GLint *)&blendfunc_.src_color);
  assert_gl("Getting GL_BLEND_SRC_RGB");
  glGetIntegerv(GL_BLEND_SRC_ALPHA,      (GLint *)&blendfunc_.src_alpha);
  assert_gl("Getting GL_BLEND_SRC_ALPHA");
  glGetIntegerv(GL_BLEND_DST_RGB,        (GLint *)&blendfunc_.dst_color);
  assert_gl("Getting GL_BLEND_DST_RGB");
  glGetIntegerv(GL_BLEND_DST_ALPHA,      (GLint *)&blendfunc_.dst_alpha);
  assert_gl("Getting GL_BLEND_DST_ALPHA");
  glGetIntegerv(GL_BLEND_EQUATION_RGB,   (GLint *)&blendeq_.color);
  assert_gl("Getting GL_BLEND_EQUATION_RGB");
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&blendeq_.alpha);
  assert_gl("Getting GL_BLEND_EQUATION_ALPHA");
}



/*==============================================================================
  blend_func

    TODO: Description
==============================================================================*/
void gl_state_t::blend_func(GLenum &src_color, GLenum &src_alpha,
                            GLenum &dst_color, GLenum &dst_alpha) const
{
  src_color = blendfunc_.src_color;
  src_alpha = blendfunc_.src_alpha;
  dst_color = blendfunc_.dst_color;
  dst_alpha = blendfunc_.dst_alpha;
}



/*==============================================================================
  set_blend_func

    TODO: Description
==============================================================================*/
void gl_state_t::set_blend_func(GLenum src_both, GLenum dst_both)
{
  if (src_both != blendfunc_.src_color ||
      src_both != blendfunc_.src_alpha ||
      dst_both != blendfunc_.dst_color ||
      dst_both != blendfunc_.dst_alpha) {
    glBlendFunc(src_both, dst_both);
    assert_gl("Setting blend function (combined)");
    blendfunc_.src_color = src_both;
    blendfunc_.src_alpha = src_both;
    blendfunc_.dst_color = dst_both;
    blendfunc_.dst_alpha = dst_both;
  }
}



/*==============================================================================
  set_blend_func

    TODO: Description
==============================================================================*/
void gl_state_t::set_blend_func(GLenum src_color, GLenum src_alpha,
                                GLenum dst_color, GLenum dst_alpha)
{
  if (src_color != blendfunc_.src_color ||
      src_alpha != blendfunc_.src_alpha ||
      dst_color != blendfunc_.dst_color ||
      dst_alpha != blendfunc_.dst_alpha) {
    glBlendFuncSeparate(src_color, src_alpha, dst_color, dst_alpha);
    assert_gl("Setting blend function (separate)");
    blendfunc_.src_color = src_color;
    blendfunc_.src_alpha = src_alpha;
    blendfunc_.dst_color = dst_color;
    blendfunc_.dst_alpha = dst_alpha;
  }
}



/*==============================================================================
  blend_equation

    TODO: Description
==============================================================================*/
void gl_state_t::blend_equation(GLenum &color, GLenum &alpha) const
{
  color = blendeq_.color;
  alpha = blendeq_.alpha;
}



/*==============================================================================
  set_blend_equation

    TODO: Description
==============================================================================*/
void gl_state_t::set_blend_equation(GLenum mode)
{
  if (mode != blendeq_.color ||
      mode != blendeq_.alpha) {
    glBlendEquation(mode);
    assert_gl("Setting blend equation (combined)");
    blendeq_.color = mode;
    blendeq_.alpha = mode;
  }
}



/*==============================================================================
  set_blend_equation

    TODO: Description
==============================================================================*/
void gl_state_t::set_blend_equation(GLenum color, GLenum alpha)
{
  if (color != blendeq_.color ||
      alpha != blendeq_.alpha) {
    glBlendEquationSeparate(color, alpha);
    assert_gl("Setting blend equation (separate)");
    blendeq_.color = color;
    blendeq_.alpha = alpha;
  }
}



} // namespace renderer
} // namespace snow
