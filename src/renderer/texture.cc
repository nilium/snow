#include "texture.hh"
#include "gl_state.hh"
#include "gl_error.hh"


namespace snow {


rtexture_t::rtexture_t(gl_state_t &state, GLenum target) :
  state_(state),
  name_(0),
  target_(target)
{
}



rtexture_t::rtexture_t(rtexture_t &&other) :
  state_(other.state_),
  name_(other.name_),
  target_(other.target_)
{
  other.zero();
}



rtexture_t &rtexture_t::operator = (rtexture_t &&other)
{
  if (&other != this) {
    if (!gl_state_t::compatible(state_, other)) {
      throw std::invalid_argument("Cannot move texture: GL states are incompatible");
    }
    name_ = other.name_;
    target_ = other.target_;
    other.zero();
  }
  return *this;
}



rtexture_t::~rtexture_t()
{
  unload();
}


void rtexture_t::bind()
{
  if (!generated()) {
    glGenTextures(1, &name_);
    assert_gl("Generating texture object");
  }

  state_.bind_texture(target_, name_);
}



void rtexture_t::tex_image_1d(GLint level, GLint internal_format, GLsizei width,
  GLenum format, GLenum type, const void *data)
{
  bind();
  glTexImage1D(target_, level, internal_format, width, 0, format, type,
    (const GLvoid *)data);
  assert_gl("Specifying texture image (1D)");
  width_  = (int)width;
  height_ = 1;
  depth_  = 1;
}



void rtexture_t::tex_image_2d(GLint level, GLint internal_format, GLsizei width,
  GLsizei height, GLenum format, GLenum type, const void *data)
{
  bind();
  glTexImage2D(target_, level, internal_format, width, height, 0, format, type,
    (const GLvoid *)data);
  assert_gl("Specifying texture image (2D)");
  width_  = (int)width;
  height_ = (int)height;
  depth_  = 1;
}



void rtexture_t::tex_image_3d(GLint level, GLint internal_format, GLsizei width,
  GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data)
{
  bind();
  glTexImage3D(target_, level, internal_format, width, height, depth, 0, format,
    type, (const GLvoid *)data);
  assert_gl("Specifying texture image (3D)");
  width_  = (int)width;
  height_ = (int)height;
  depth_  = (int)depth;
}



void rtexture_t::tex_image_1d_compressed(GLint level, GLint internal_format,
  GLsizei width, GLsizei data_size, const void *data)
{
  bind();
  glCompressedTexImage1D(target_, level, internal_format, 0, width, data_size,
    (const GLvoid *)data);
  assert_gl("Specifying compressed texture contents (1D)");
  width_  = (int)width;
  height_ = 1;
  depth_  = 1;
}



void rtexture_t::tex_image_2d_compressed(GLint level, GLint internal_format,
  GLsizei width, GLsizei height, GLsizei data_size, const void *data)
{
  bind();
  glCompressedTexImage2D(target_, level, internal_format, 0, width, height,
    data_size, (const GLvoid *)data);
  assert_gl("Specifying compressed texture contents (2D)");
  width_  = (int)width;
  height_ = (int)height;
  depth_  = 1;
}



void rtexture_t::tex_image_3d_compressed(GLint level, GLint internal_format,
  GLsizei width, GLsizei height, GLsizei depth, GLsizei data_size,
  const void *data)
{
  bind();
  glCompressedTexImage3D(target_, level, internal_format, 0, width, height,
    depth, data_size, (const GLvoid *)data);
  assert_gl("Specifying compressed texture contents (3D)");
  width_  = (int)width;
  height_ = (int)height;
  depth_  = (int)depth;
}



void rtexture_t::tex_subimage_1d(GLint level, GLint xoff, GLsizei width,
  GLenum format, GLenum type, const void *data)
{
  bind();
  glTexSubImage1D(target_, level, xoff, width, format, type,
    (const GLvoid *)data);
  assert_gl("Specifying texture contents for subimage (1D)");
}



void rtexture_t::tex_subimage_2d(GLint level, GLint xoff, GLint yoff,
  GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data)
{
  bind();
  glTexSubImage2D(target_, level, xoff, yoff, width, height, format, type,
    (const GLvoid *)data);
  assert_gl("Specifying texture contents for subimage (2D)");
}



void rtexture_t::tex_subimage_3d(GLint level, GLint xoff, GLint yoff,
  GLint zoff, GLsizei width, GLsizei height, GLsizei depth, GLenum format,
  GLenum type, const void *data)
{
  bind();
  glTexSubImage3D(target_, level, xoff, yoff, zoff, width, height, depth,
    format, type, (const GLvoid *)data);
  assert_gl("Specifying texture contents for subimage (3D)");
}



void rtexture_t::invalidate(GLint level)
{
#if GL_VERSION_4_3 || GL_ARB_invalidate_subdata
  if (!generated()) {
    glInvalidateTexImage(name_, level);
    assert_gl("Invalidating texture storage");
  }
#endif
}



void rtexture_t::invalidate_subimage(GLint level,
  GLint xoff, GLint yoff, GLint zoff,
  GLsizei width, GLsizei height, GLsizei depth)
{
#if GL_VERSION_4_3 || GL_ARB_invalidate_subdata
  if (!generated()) {
    glInvalidateTexSubImage(name_, level, xoff, yoff, zoff,
      width, height, depth);
    assert_gl("Invalidating texture storage");
  }
#endif
}



void rtexture_t::unload()
{
  if (generated()) {
    glDeleteTextures(1, &name_);
    assert_gl("Deleting texture object");
    name_ = 0;
  }
}



void rtexture_t::zero()
{
  name_ = 0;
  target_ = 0;
}


} // namespace snow
