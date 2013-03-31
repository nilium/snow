#ifndef __SNOW__SHADER_HH__
#define __SNOW__SHADER_HH__

#include <snow/config.hh>
#include "sgl.hh"

namespace snow {


struct gl_state_t;


struct S_EXPORT rshader_t
{
  friend struct rprogram_t;
  friend struct gl_state_t;

  rshader_t(gl_state_t &state, GLenum kind);
  rshader_t(rshader_t &&other);
  rshader_t &operator = (rshader_t &&other);
  ~rshader_t();

  rshader_t(const rshader_t &) = delete;
  rshader_t &operator = (const rshader_t &) = delete;

  inline GLenum kind() const { return kind_; }

  // Sets the source code string for the shader.
  void load_source(const string &source);
  // Returns true if compilation succeeded, false if there was an error.
  // Check error_string if there's an error.
  bool compile();

  inline bool valid() const { return shader_ != 0; }
  inline bool compiled() const { return compiled_; }
  inline bool usable() const { return valid() && compiled(); }

  void unload();

  inline bool has_error() const { return error_str_.length() != 0; }
  inline string error_string() const { return error_str_; }

private:
  S_HIDDEN void zero();

  gl_state_t &state_;
  GLenum kind_;
  GLuint shader_;
  bool compiled_;
  string error_str_;
};


} // namespace snow

#endif /* end __SNOW__SHADER_HH__ include guard */
