#ifndef __SNOW__BUFFER_HH__
#define __SNOW__BUFFER_HH__

#include "../config.hh"
#include "sgl.hh"

namespace snow {


struct gl_state_t;


/*******************************************************************************
*                     general-purpose GL buffer structure                      *
*******************************************************************************/
struct S_EXPORT rbuffer_t
{
  friend struct gl_state_t;

  /*****************************************************************************
  * After calling 'bind', you are expected to use glBufferSubData to load
  * what you need into the buffer. If not already created, the buffer will be
  * generated as a buffer of the specified size with no contents.
  *****************************************************************************/

  rbuffer_t(gl_state_t &state, unsigned target, GLenum usage, GLsizeiptr size);
  ~rbuffer_t();

  // If moved, the new buffer will delete its current buffer (meaning the name
  // changes and previous calls to 'bind' are no longer valid), take over the
  // other buffer's data, and invalidate the previous buffer. The previous
  // buffer will be unusable afterward unless a new buffer is moved into it.
  rbuffer_t(rbuffer_t &&);
  rbuffer_t &operator = (rbuffer_t &&);

  rbuffer_t(const rbuffer_t &) = delete;
  rbuffer_t &operator = (const rbuffer_t &) = delete;

  // All mutators imply calls to bind()

  inline auto   target() const -> unsigned { return target_; }

  inline auto   size() const -> GLsizeiptr { return size_; }
  // resize(size()) will re-buffer data. Avoid calling unless absolutely necessary.
  void          resize(GLsizeiptr new_size, bool save_data);

  inline auto   usage() const -> GLenum
  {
    return usage_;
  }

  // Changes the intended usage of this buffer. Implies resize(size()) in order
  // to rebuild the buffer.
  void          set_usage(GLenum usage);

  inline bool   valid() const
  {
    return target_ != 0 && usage_ != 0 && size_ > 0;
  }

  inline bool   generated() const
  {
    return buffer_ != 0;
  }

  // Binds the current buffer to its target. Does not attempt to optimize away
  // binding the buffer if it's already bound.
  // If the buffer has not already been generated, it will do so and resize the
  // buffer to the requested size.
  inline void   bind() { bind_as(target_); }

  // Binds the current buffer to a specific target, rather than the one the
  // buffer was created with. If the buffer was not yet created, it will be
  // created with this target rather than its initial target.
  void          bind_as(GLenum alt_target);

  // Gets data from the buffer at offset of length and places it in data.
  // Implies bind if the buffer has been generated (generated() == true),
  // otherwise is essentially a no-op after verifying input.
  // This does not verify that the offset and length are in range, though will
  // throw an exception if it causes a GL error because they are out of range
  // (when compiled in debug mode).
  void          get_buffer(void *data, GLintptr offset, GLsizeiptr length);

  // Will delete the buffer if it has already been generated. If not, this
  // method does nothing.
  void          unload();

protected:
  S_HIDDEN
  void          zero();

  gl_state_t &state_;
  GLsizeiptr size_;
  unsigned target_;
  GLuint buffer_;
  GLenum usage_;
};


} // namespace snow

#endif /* end __SNOW__BUFFER_HH__ include guard */
