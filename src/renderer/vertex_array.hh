#ifndef __SNOW__VERTEX_ARRAY_HH__
#define __SNOW__VERTEX_ARRAY_HH__

#include <snow/config.hh>
#include "sgl.hh"
#include <functional>

namespace snow {


struct gl_state_t;

struct S_EXPORT rvertex_array_t
{
  using init_fn_t = std::function<bool(gl_state_t &)>;

  rvertex_array_t(gl_state_t &gl);
  rvertex_array_t(gl_state_t &gl, init_fn_t &&initfn);
  rvertex_array_t(rvertex_array_t &&);
  rvertex_array_t &operator = (rvertex_array_t &&);
  ~rvertex_array_t();

  rvertex_array_t(const rvertex_array_t &)              = delete;
  rvertex_array_t &operator = (const rvertex_array_t &) = delete;

  void          set_initializer(init_fn_t &&initfn);
  void          set_initializer(std::nullptr_t);

  inline bool   initialized() const { return name_ != 0 && inited_; }
  void          bind();
  // Forces the VAO to be initialized. This will result in a call to bind, then
  // will re-bind the previously-bound VAO.
  void          load();
  // Releases GL resources used by the VAO. If bound again later, it will be
  // recreated and the init function will be called again, unless the init
  // function is set to nullptr.
  void          unload();

private:
  void          zero();
  void          force_load();

  gl_state_t &  state_;
  init_fn_t     initfn_;
  bool          inited_;
  GLuint        name_;
};


} // namespace snow

#endif /* end __SNOW__VERTEXARRAY_HH__ include guard */
