/*
  shader.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "shader.hh"
#include "gl_error.hh"

namespace snow {


rshader_t::rshader_t(GLenum kind)
: kind_(kind), shader_(glCreateShader(kind)), compiled_(false),
  error_str_()
{
}



rshader_t::rshader_t(rshader_t &&other)
: kind_(other.kind_), shader_(other.shader_),
  compiled_(other.compiled_), error_str_(std::move(other.error_str_))
{
  other.zero();
}



rshader_t &rshader_t::operator = (rshader_t &&other)
{
  if (this != &other) {
    unload();

    kind_ = other.kind_;
    shader_ = other.shader_;
    compiled_ = other.compiled_;
    error_str_ = std::move(other.error_str_);

    other.zero();
  }
  return *this;
}



rshader_t::~rshader_t()
{
  unload();
}



void rshader_t::load_source(const string &source)
{
  load_source(source.c_str(), (GLint)source.size());
}



void rshader_t::load_source(const char *source, GLint length)
{
  if (!valid())
    s_throw(std::runtime_error, "Attempt to load source for invalid shader "
                             "object");

  const char *shader_sources[1] = {
    source
  };

  GLint shader_source_lengths[1] = {
    length
  };

  glShaderSource(shader_, 1, shader_sources, shader_source_lengths);
  assert_gl("Loading shader source (what did you do to get this error?)");
}



bool rshader_t::compile()
{
  glCompileShader(shader_);
  assert_gl("Compiling shader");

  GLint compile_status = 0;
  glGetShaderiv(shader_, GL_COMPILE_STATUS, &compile_status);
  assert_gl("Getting shader compilation status");

  if (compile_status != GL_TRUE) {
    GLint log_length = 0;

    glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &log_length);
    assert_gl("Getting shader info log length");
    error_str_.resize(log_length);
    glGetShaderInfoLog(shader_, log_length, NULL, error_str_.data());
    assert_gl("Getting shader info log string");
  }

  compiled_ = (compile_status == GL_TRUE);

  return compiled_;
}



void rshader_t::unload()
{
  if (shader_) {
    glDeleteShader(shader_);
    assert_gl("Deleting shader object");
  }

  zero();
}



void rshader_t::zero()
{
  kind_ = 0;
  shader_ = 0;
  compiled_ = false;
  error_str_.clear();
}


} // snow
