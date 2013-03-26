#include "buffer.hh"
#include "gl_state.hh"
#include "gl_error.hh"
#include <vector>

namespace snow {


rbuffer_t::rbuffer_t(gl_state_t &state, GLuint target, GLenum usage, GLsizeiptr size)
: state_(state), size_(size), target_(target), buffer_(0), usage_(usage)
{
}



rbuffer_t::rbuffer_t(rbuffer_t &&buf)
: state_(buf.state_), size_(buf.size_), target_(buf.target_), buffer_(buf.buffer_), usage_(buf.usage_)
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
    if (&state_ != &buf.state_)
      throw std::invalid_argument("Unable to move buffer: GL state objects differ");

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
    throw std::runtime_error("Called resize on invalid buffer");
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
    throw std::runtime_error("Called set_usage on invalid buffer");
  usage_ = usage;
  resize(size(), true);
}



void rbuffer_t::get_buffer(void *data, GLintptr offset, GLsizeiptr length)
{
  if (!valid())
    throw std::runtime_error("Called get_buffer on invalid buffer");
  else if (!data)
    throw std::invalid_argument("get_buffer: data ptr is null");
  else if (length <= 0)
    throw std::invalid_argument("get_buffer: length is <= 0");

  if (buffer_) {
    bind();
    glGetBufferSubData(target_, offset, length, data);
    assert_gl("Failed to retrieve data from buffer");
  }
}



void rbuffer_t::bind()
{
  if (!valid())
    throw std::runtime_error("Called bind on invalid buffer");

  if (!generated() && size_ > 0) {
    glGenBuffers(1, &buffer_);
    assert_gl("Failed to generate GL buffer.");
    state_.bind_buffer(target_, buffer_);
    glBufferData(target_, size_, NULL, usage_);
    assert_gl("Failed to initialize GL buffer");
  } else {
    state_.bind_buffer(target_, buffer_);
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
