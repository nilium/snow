#ifndef __SNOW__MATERIAL_SINGLE_HH__
#define __SNOW__MATERIAL_SINGLE_HH__

#include "../config.hh"
#include <snow/math/math3d.hh>

#include "sgl.hh"
#include "material.hh"


namespace snow {


struct gl_state_t;
struct rmaterial_t;
struct rprogram_t;
struct rtexture_t;


struct rmaterial_basic_t : public rmaterial_t
{
  friend struct gl_state_t;


  rmaterial_basic_t(gl_state_t &gl);
  virtual ~rmaterial_basic_t();

  virtual bool valid() const;

  virtual int passes() const;

  virtual void prepare_pass(int pass);

  virtual void set_program(rprogram_t *program,
                           const string &projection_name = "projection",
                           const string &modelview_name = "modelview",
                           const string &texture_name = "diffuse");

  virtual void set_program(rprogram_t *program,
                           int projection_key, int modelview_key, int diffuse_key);

  virtual void set_projection(const mat4f_t &proj);
  virtual void set_modelview(const mat4f_t &mv);

  virtual void set_texture(rtexture_t *texture);

private:
  mat4f_t         modelview_;
  mat4f_t         projection_;

  rprogram_t *    program_;
  rtexture_t *    texture_;
  GLint           modelview_loc_;
  GLint           projection_loc_;
  GLint           diffuse_loc_;
};

} // namespace snow

#endif /* end __SNOW__MATERIAL_SINGLE_HH__ include guard */
