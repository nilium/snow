/*
  gl_error.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__GL_ERROR_HH__
#define __SNOW__GL_ERROR_HH__

#include "../config.hh"
#include "sgl.hh"
#include <stdexcept>

namespace snow {


struct S_EXPORT gl_error_t : public std::runtime_error
{
  gl_error_t(const std::string &what);
  gl_error_t(const std::string &what, GLenum error);
  ~gl_error_t() override;
};


std::string gl_error_string(GLenum error);
void sn_assert_gl__(const char *msg, size_t line, const char *file, const char *func);


} // namespace snow

#ifdef NDEBUG
#define assert_gl(MSGLIT) { GLenum sgl__temp_error__ = glGetError(); if (sgl__temp_error__ != GL_NO_ERROR) { s_log_error("GL Error[%x %s]: %s", sgl__temp_error__, gl_error_string(sgl__temp_error__).c_str() ,MSGLIT); }}
#else
#define assert_gl(MSGLIT) snow::sn_assert_gl__(MSGLIT, __LINE__, __FILE__, __FUNCTION__)
#endif

#endif /* end __SNOW__GL_ERROR_HH__ include guard */
