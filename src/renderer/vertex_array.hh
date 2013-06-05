#ifndef __SNOW__VERTEX_ARRAY_HH__
#define __SNOW__VERTEX_ARRAY_HH__

#include "../config.hh"
#include "sgl.hh"
#include <functional>

namespace snow {


struct S_EXPORT rvertex_array_t
{
  rvertex_array_t();
  rvertex_array_t(rvertex_array_t &&);
  rvertex_array_t &operator = (rvertex_array_t &&);
  ~rvertex_array_t();

  rvertex_array_t(const rvertex_array_t &)              = delete;
  rvertex_array_t &operator = (const rvertex_array_t &) = delete;

  void          bind();
  // Releases GL resources used by the VAO. If bound again later, it will be
  // recreated and the init function will be called again, unless the init
  // function is set to nullptr.
  void          unload();

  bool          generated() const { return name_ != 0; }

private:
  void          zero();
  void          force_load();

  GLuint        name_;
};


} // namespace snow

#endif /* end __SNOW__VERTEXARRAY_HH__ include guard */
