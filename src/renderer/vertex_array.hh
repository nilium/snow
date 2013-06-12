#ifndef __SNOW__VERTEX_ARRAY_HH__
#define __SNOW__VERTEX_ARRAY_HH__

#include "../config.hh"
#include "sgl.hh"
#include <functional>


namespace snow {


struct rbuffer_t;


struct S_EXPORT rvertex_array_t
{
  rvertex_array_t();
  rvertex_array_t(rvertex_array_t &&);
  rvertex_array_t &operator = (rvertex_array_t &&);
  ~rvertex_array_t();

  rvertex_array_t(const rvertex_array_t &)              = delete;
  rvertex_array_t &operator = (const rvertex_array_t &) = delete;

  void          bind();
  static void   unbind();
  // Releases GL resources used by the VAO. If bound again later, it will be
  // recreated and the init function will be called again, unless the init
  // function is set to nullptr.
  void          unload();

  bool          generated() const { return name_ != 0; }


  void          enable_attrib(GLuint index);
  void          disable_attrib(GLuint index);

  void          bind_attrib(GLuint index, GLint size, GLenum type,
                  GLboolean normalized, GLsizei stride, GLintptr offset);

private:
  #if GL_VERSION_3_0 || GL_ES_VERSION_3_0 || GL_OES_vertex_array_object
  GLuint        name_;
  #else
  #error No implementation defined for non-GL vertex arrays yet
  #endif
};


} // namespace snow

#endif /* end __SNOW__VERTEXARRAY_HH__ include guard */
