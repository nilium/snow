#include "gl_error.hh"
#include <sstream>

namespace {

std::string sn_gl_error_string__(const char *msg, size_t line, const char *file, const char *func)
{
  std::stringstream stream;
  stream << '[' << file << ':'
         << func << ':'
         << line << "] "
         << msg;
  return stream.str();
}

}

void sn_assert_gl__(const char *msg, size_t line, const char *file, const char *func)
{
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::string error_str = sn_gl_error_string__(msg, line, file, func);
    throw snow::renderer::gl_error_t(error_str, error);
  }
}

std::string gl_error_string(GLenum error)
{
  switch (error) {
  case GL_NO_ERROR: return "GL_NO_ERROR";
  case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#if 0
  case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
  case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
#endif
  default: {
      std::stringstream stream;
      stream << '<' << std::hex << std::uppercase << error << " (UNKNOWN)>";
      return stream.str();
    }
  }
}

namespace snow {
namespace renderer {

namespace {

std::string what_with_error(const std::string &what, GLenum error)
{
  std::stringstream stream;
  stream << gl_error_string(error);
  stream << ": " << what;
  return stream.str();
}

}

gl_error_t::gl_error_t(const std::string &what)
: std::runtime_error(what_with_error(what, glGetError()))
{
}

gl_error_t::gl_error_t(const std::string &what, GLenum error)
: std::runtime_error(what_with_error(what, error))
{
}

gl_error_t::~gl_error_t()
{
}

} // namespace renderer
} // namespace snow
