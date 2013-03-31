#ifndef __SNOW__TEXTURE_HH__
#define __SNOW__TEXTURE_HH__

#include <snow/config.hh>
#include "sgl.hh"


namespace snow {


struct gl_state_t;


struct rtexture_t
{
  friend struct gl_state_t;


  rtexture_t(gl_state_t &gl, GLenum target);

  rtexture_t(rtexture_t &&other);
  rtexture_t &operator = (rtexture_t &&other);

  rtexture_t(const rtexture_t &other) = delete;
  rtexture_t &operator = (const rtexture_t &other) = delete;

  ~rtexture_t();

  inline bool valid() const { return target_ != 0; }
  inline bool generated() const { return name_ != 0; }
  void bind();

  inline GLenum target() const { return target_; }
  inline void set_target(GLenum target)
  {
    unload();
    target_ = target;
  }

  void unload();

  void tex_image_1d(GLint level, GLint internal_format, GLsizei width,
    GLenum format, GLenum type, const void *data);
  void tex_image_2d(GLint level, GLint internal_format, GLsizei width,
    GLsizei height, GLenum format, GLenum type, const void *data);
  void tex_image_3d(GLint level, GLint internal_format, GLsizei width,
    GLsizei height, GLsizei depth, GLenum format, GLenum type,
    const void *data);

  void tex_image_1d_compressed(GLint level, GLint internal_format, GLsizei width,
    GLsizei data_size, const void *data);
  void tex_image_2d_compressed(GLint level, GLint internal_format, GLsizei width,
    GLsizei height, GLsizei data_size, const void *data);
  void tex_image_3d_compressed(GLint level, GLint internal_format, GLsizei width,
    GLsizei height, GLsizei depth, GLsizei data_size, const void *data);

  void tex_subimage_1d(GLint level, GLint xoff, GLsizei width, GLenum format,
    GLenum type, const void *data);
  void tex_subimage_2d(GLint level, GLint xoff, GLint yoff, GLsizei width,
    GLsizei height, GLenum format, GLenum type, const void *data);
  void tex_subimage_3d(GLint level, GLint xoff, GLint yoff, GLint zoff,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
    const void *data);

  // Note: if GL_ARB_invalidate_subdata or GL version 4.3 aren't available at
  // compile-time, these are no-ops.
  void invalidate(GLint level);
  void invalidate_subimage(GLint level, GLint xoff, GLint yoff, GLint zoff,
    GLsizei width, GLsizei height, GLsizei depth);

  inline GLuint name() const { return name_; }
  inline operator GLuint() const { return name_; }
  inline GLuint operator * () const { return name_; }

private:
  void zero();

  gl_state_t &state_;
  GLuint name_;
  GLenum target_;
  int width_, height_, depth_;
};


} // namespace snow

#endif /* end __SNOW__TEXTURE_HH__ include guard */
