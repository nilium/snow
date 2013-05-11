#ifndef __SNOW__MATERIAL_HH__
#define __SNOW__MATERIAL_HH__

#include "../config.hh"
#include "sgl.hh"
#include <snow/math/math3d.hh>

#include <functional>
#include <map>


namespace snow {


struct rtexture_t;
struct rprogram_t;


struct rpass_t
{
  static constexpr const size_t MAX_TEXTURE_UNITS = 8;

  bool skip;

  struct {
    GLenum sfactor;
    GLenum dfactor;
  } blend;

  struct {
    GLenum func;
    GLboolean write;
  } depth;

  struct {
    GLuint mask;
    GLenum func;
    GLuint ref;
    GLint ref_mask;
    GLenum fail;
    GLenum depth_fail;
    GLenum depth_pass;
  } stencil;

  rprogram_t *program;
  mutable GLint modelview_;
  mutable GLint projection_;

  // Corresponds to 8 texture units
  struct {
    rtexture_t *texture;
    GLint min_filter;  // Defaults to GL_LINEAR
    GLint mag_filter;  // Defaults to GL_LINEAR
    GLint x_wrap;
    GLint y_wrap;
    mutable GLint uniform_; // determined on apply, defaults to -2 (if -1, skipped)
  } textures[MAX_TEXTURE_UNITS];

  void apply() const;
  static const rpass_t &defaults();
  static void reset_pass_state();
};


struct rmaterial_t
{
  rmaterial_t();
  virtual ~rmaterial_t();

  // Must return true if all passes can be prepared, false otherwise. The
  // default implementation returns true.
  bool valid() const;

  // Returns the number of passes the material requires. The default
  // implementation returns 0.
  size_t num_passes() const;
  virtual void set_num_passes(size_t num);
  // Prepares to draw a given pass in the material. Returns true if the pass
  // should go ahead, otherwise returns false.
  bool prepare_pass(size_t passnum) const;

  // Direct access to passes, either for creating materials or to inspect a pass.
  rpass_t &pass(size_t pass);
  const rpass_t &pass(size_t pass) const;

  // Sets the projection matrix for all materials
  static void set_projection(const mat4f_t &proj);
  // Sets the modelview matrix for all materials
  static void set_modelview(const mat4f_t &mv);

private:
  static constexpr const size_t MAX_PASSES = 4;
  size_t num_passes_ = 0;
  rpass_t passes_[MAX_PASSES] = {
    rpass_t::defaults(),
    rpass_t::defaults(),
    rpass_t::defaults(),
    rpass_t::defaults()
  };
};


} // namespace snow

#endif /* end __SNOW__MATERIAL_HH__ include guard */
