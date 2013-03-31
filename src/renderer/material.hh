#ifndef __SNOW__MATERIAL_HH__
#define __SNOW__MATERIAL_HH__

#include <snow/config.hh>
#include <snow/math/math3d.hh>

#include <functional>
#include <map>


namespace snow {


struct gl_state_t;


struct rmaterial_t
{
  using pass_fn_t = std::function<void(int pass)>;

  inline void do_per_pass(pass_fn_t &&fn)
  {
    if (!valid()) {
      throw std::invalid_argument("Material is not valid");
    }
    const int num_passes = passes();
    for (int pass = 0; pass < num_passes; ++pass) {
      prepare_pass(pass);
      fn(pass);
    }
  }

  friend struct gl_state_t;

  rmaterial_t(gl_state_t &gl);
  virtual ~rmaterial_t() = 0;


  // Must return true if all passes can be prepared, false otherwise. The
  // default implementation returns false.
  virtual bool valid() const = 0;


  // Returns the number of passes the material requires. The default
  // implementation returns 0.
  virtual int passes() const = 0;
  // Prepares to draw a given pass in the material. The default implementation
  // does nothing.
  virtual void prepare_pass(int pass) = 0;


  // Sets the projection matrix for the material
  virtual void set_projection(const mat4f_t &proj) = 0;
  // Sets the modelview matrix for the material
  virtual void set_modelview(const mat4f_t &mv) = 0;

protected:
  gl_state_t &state_;
};


} // namespace snow

#endif /* end __SNOW__MATERIAL_HH__ include guard */
