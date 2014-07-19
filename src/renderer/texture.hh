/*
  texture.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__TEXTURE_HH__
#define __SNOW__TEXTURE_HH__

#include "../config.hh"
#include "sgl.hh"
#include <snow-ext/stb_image.h>


namespace snow {


struct rtexture_t
{
  rtexture_t(); // target = GL_TEXTURE_2D
  rtexture_t(GLenum target);

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

  /*! Sets the texture mag and min filters.

  Does not take effect until next binding. */
  void set_filters(GLint mag_filter, GLint min_filter);
  /*! Sets the texture wrapping parameters. Z optional -- defaults to GL_REPEAT.

  Does not take effect until next binding. */
  void set_wrapping(GLint wrap_x, GLint wrap_y, GLint wrap_z = GL_REPEAT);

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

  inline int width() const { return width_; }
  inline int height() const { return height_; }
  inline int depth() const { return depth_; }
  inline const char *source() const { return source_; }

  void set_source(const string &src);
  void set_source(const char *src);

private:
  static constexpr const size_t MAX_SOURCE_LENGTH = 64;
  void zero();

  char source_[MAX_SOURCE_LENGTH];
  GLuint name_;
  GLenum target_;
  int width_, height_, depth_;
  GLint mag_filter_ = GL_LINEAR;
  GLint min_filter_ = GL_LINEAR;
  GLint wrap_x_ = GL_REPEAT;
  GLint wrap_y_ = GL_REPEAT;
  GLint wrap_z_ = GL_REPEAT;
};


enum texture_components_t : int
{
  TEX_COMP_DEFAULT = STBI_default,
  TEX_COMP_GREY = STBI_grey,
  TEX_COMP_GREY_ALPHA = STBI_grey_alpha,
  TEX_COMP_RGB = STBI_rgb,
  TEX_COMP_RGBA = STBI_rgb_alpha
};


bool load_texture_2d(const string &path, rtexture_t &tex, bool gen_mipmaps = true,
  texture_components_t required_components = TEX_COMP_DEFAULT);


} // namespace snow

#endif /* end __SNOW__TEXTURE_HH__ include guard */
