/*
  buffer.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "buffer.hh"
#include "gl_error.hh"
#include <vector>

namespace snow {


rbuffer_t::rbuffer_t(GLuint target, GLenum usage, GLsizeiptr size)
: size_(size), target_(target), buffer_(0), usage_(usage)
{
}



rbuffer_t::rbuffer_t(rbuffer_t &&buf)
: size_(buf.size_), target_(buf.target_), buffer_(buf.buffer_), usage_(buf.usage_)
{
  buf.zero();
}



rbuffer_t::~rbuffer_t()
{
  if (valid() && generated())
    unload();
}



rbuffer_t &rbuffer_t::operator = (rbuffer_t &&buf)
{
  if (this != &buf) {
    if (valid() && generated())
      unload();

    buffer_ = buf.buffer_;
    target_ = buf.target_;
    size_ = buf.size_;
    usage_ = buf.usage_;

    buf.zero();
  }

  return *this;
}



void rbuffer_t::resize(GLsizeiptr new_size, bool save_data)
{
  if (!valid())
    s_throw(std::runtime_error, "Called resize on invalid buffer");
  else if (new_size < 1)
    unload();
  GLsizeiptr old_size = size_;
  size_ = new_size;

  if (size_ == 0)
    return;

  if (generated()) {
    if (save_data) {
      GLsizeiptr temp_size = std::min(new_size, old_size);
      std::vector<char> buf(temp_size);
      get_buffer(buf.data(), 0, temp_size);
      if (new_size < old_size) {
        glBufferData(target_, new_size, buf.data(), usage_);
        assert_gl("Failed to recreate buffer using existing data");
      } else {
        glBufferData(target_, new_size, NULL, usage_);
        assert_gl("Failed to recreate buffer prior to loading existing data");
        glBufferSubData(target_, 0, temp_size, buf.data());
        assert_gl("Failed to load existing data into buffer");
      }
    } else {
      // only call bind if not saving the data, since get_buffer has an implied bind
      bind();
      glBufferData(target_, size_, NULL, usage_);
      assert_gl("Failed to resize buffer without using existing data");
    }
  } else {
    // If the buffer hasn't been generated it, just bind it and that'll create
    // it and ergo set its size.
    bind();
  }
}



void rbuffer_t::set_usage(GLenum usage)
{
  if (!valid())
    s_throw(std::runtime_error, "Called set_usage on invalid buffer");
  usage_ = usage;
  resize(size(), true);
}



void rbuffer_t::get_buffer(void *data, GLintptr offset, GLsizeiptr length)
{
  if (!valid())
    s_throw(std::runtime_error, "Called get_buffer on invalid buffer");
  else if (!data)
    s_throw(std::invalid_argument, "get_buffer: data ptr is null");
  else if (length <= 0)
    s_throw(std::invalid_argument, "get_buffer: length is <= 0");

  if (buffer_) {
    bind();
    glGetBufferSubData(target_, offset, length, data);
    assert_gl("Failed to retrieve data from buffer");
  }
}



void rbuffer_t::bind_as(GLenum alt_target)
{
  if (!valid())
    s_throw(std::runtime_error, "Called bind on invalid buffer");

  if (!generated() && size_ > 0) {
    glGenBuffers(1, &buffer_);
    assert_gl("Failed to generate GL buffer.");
    glBindBuffer(alt_target, buffer_);
    assert_gl("Binding GL buffer");
    glBufferData(alt_target, size_, NULL, usage_);
    assert_gl("Failed to initialize GL buffer");
  } else {
    glBindBuffer(alt_target, buffer_);
    assert_gl("Binding GL buffer");
  }
}



void rbuffer_t::unload()
{
  if (generated()) {
    glDeleteBuffers(1, &buffer_);
    assert_gl("Deleting buffer object");
  }
}



void rbuffer_t::zero()
{
  buffer_ = 0;
  target_ = 0;
  size_ = 0;
  usage_ = 0;
}


} // namespace snow
