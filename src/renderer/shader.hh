/*
  shader.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__SHADER_HH__
#define __SNOW__SHADER_HH__

#include "../config.hh"
#include "sgl.hh"

namespace snow {


struct S_EXPORT rshader_t
{
  friend struct rprogram_t;

  rshader_t(GLenum kind);
  rshader_t(rshader_t &&other);
  rshader_t &operator = (rshader_t &&other);
  ~rshader_t();

  rshader_t(const rshader_t &) = delete;
  rshader_t &operator = (const rshader_t &) = delete;

  inline GLenum kind() const { return kind_; }

  // Sets the source code string for the shader.
  void load_source(const string &source);
  void load_source(const char *source, GLint length);
  // Returns true if compilation succeeded, false if there was an error.
  // Check error_string if there's an error.
  bool compile();

  inline bool valid() const { return shader_ != 0; }
  inline bool compiled() const { return compiled_; }
  inline bool usable() const { return valid() && compiled(); }

  void unload();

  inline bool has_error() const { return error_str_.size() != 0; }
  inline string error_string() const { return error_str_; }

private:
  S_HIDDEN void zero();

  GLenum kind_;
  GLuint shader_;
  bool compiled_;
  string error_str_;
};


} // namespace snow

#endif /* end __SNOW__SHADER_HH__ include guard */
