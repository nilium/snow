#include "texture.hh"
#include "gl_state.hh"
#include "gl_error.hh"
#include <physfs.h>


namespace snow {


namespace {


int r_pfs_stb_read(void *user, char *data, int size)
{
  PHYSFS_File *file = (PHYSFS_File *)user;
  auto result = PHYSFS_readBytes(file, data, (PHYSFS_uint64)size);
  if (result < 0) {
    s_log_error("Error reading from PhysFS using STBI callbacks: %s",
      PHYSFS_getLastError());
  }
  return (int)result;
}



void r_pfs_stb_skip(void *user, unsigned numbytes)
{
  PHYSFS_File *file = (PHYSFS_File *)user;
  auto rtell = PHYSFS_tell(file);
  if (rtell < 0 || !PHYSFS_seek(file, rtell + numbytes)) {
    s_log_error("Failed to seek in PhysFS file: %s",
      PHYSFS_getLastError());
  }
}



int r_pfs_stb_eof(void *user)
{
  PHYSFS_File *file = (PHYSFS_File *)user;
  return PHYSFS_eof(file);
}



stbi_io_callbacks &r_pfs_stb_io_callbacks()
{
  static stbi_io_callbacks stb_pfs_io = {
    r_pfs_stb_read,
    r_pfs_stb_skip,
    r_pfs_stb_eof
  };
  return stb_pfs_io;
}


} // namespace <anon>


rtexture_t::rtexture_t(GLenum target) :
  name_(0),
  target_(target)
{
}



rtexture_t::rtexture_t() :
  name_(0),
  target_(GL_TEXTURE_2D)
{
}



rtexture_t::rtexture_t(rtexture_t &&other) :
  name_(other.name_),
  target_(other.target_)
{
  other.zero();
}



rtexture_t &rtexture_t::operator = (rtexture_t &&other)
{
  if (&other != this) {
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

  glBindTexture(target_, name_);
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
  target_ = GL_TEXTURE_2D;
}



bool load_texture_2d(const string &path, rtexture_t &tex,
  bool gen_mipmaps, texture_components_t required_components)
{

  PHYSFS_File *file = PHYSFS_openRead(path.c_str());
  if (file == NULL) {
    s_log_error("Unable to open file for reading: %s", path.c_str());
    return false;
  }

  int width = 0;
  int height = 0;
  int actual_components = 0;
  stbi_uc *data = stbi_load_from_callbacks(&r_pfs_stb_io_callbacks(), file,
    &width, &height, &actual_components, required_components);

  PHYSFS_close(file);

  if (data == NULL) {
    s_log_error("Unable to read image %s: %s",
      path.c_str(), stbi_failure_reason());
    return false;
  } else if (required_components != TEX_COMP_DEFAULT &&
             required_components != actual_components) {
    s_log_warning("Required components (%d) != actual components (%d) for %s",
      required_components, actual_components, path.c_str());
  }

  GLint internal_format = GL_RGBA;
  switch (actual_components) {
    case TEX_COMP_GREY:       internal_format = GL_RED; break;
    case TEX_COMP_GREY_ALPHA: internal_format = GL_RG;  break;
    case TEX_COMP_RGB:        internal_format = GL_RGB; break;
    case TEX_COMP_RGBA:       // default: GL_RGBA
    default: break;
  }

  if (!gl_state_t::can_create_texture(0, internal_format, width, height)) {
    stbi_image_free(data);
    s_log_error("GL cannot create a texture for this image");
    return false;
  }

  tex.set_target(GL_TEXTURE_2D);
  tex.bind();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  assert_gl("Setting GL_TEXTURE_WRAP_S");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  assert_gl("Setting GL_TEXTURE_WRAP_T");

  if (gen_mipmaps)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  assert_gl("Setting GL_TEXTURE_MIN_FILTER");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  assert_gl("Setting GL_TEXTURE_MAG_FILTER");

  tex.tex_image_2d(0, internal_format, width, height, internal_format,
    GL_UNSIGNED_BYTE, data);

  if (gen_mipmaps) {
    glGenerateMipmap(GL_TEXTURE_2D);
    assert_gl("Generating mipmaps");
  }

  stbi_image_free(data);

  return true;
}


} // namespace snow
