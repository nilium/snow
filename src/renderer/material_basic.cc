#include "material_basic.hh"
#include "texture.hh"
#include "program.hh"
#include "gl_state.hh"
#include "gl_error.hh"


namespace snow {


rmaterial_basic_t::rmaterial_basic_t(gl_state_t &gl) :
  rmaterial_t(gl),
  modelview_(mat4f_t::identity),
  projection_(mat4f_t::identity),
  program_(nullptr),
  modelview_loc_(-1),
  projection_loc_(-1)
{}



rmaterial_basic_t::~rmaterial_basic_t()
{
}



bool rmaterial_basic_t::valid() const
{
  return program_ != nullptr;
}



int rmaterial_basic_t::passes() const
{
  return 1;
}



void rmaterial_basic_t::prepare_pass(int pass)
{
  if (pass != 0) {
    s_throw(std::invalid_argument, "Invalid pass for basic material - pass must be 0");
  }

  if (valid()) {
    program_->use();

    if (modelview_loc_ != -1) {
      glUniformMatrix4fv(modelview_loc_, 1, GL_FALSE, modelview_);
      assert_gl("Setting modelview matrix");
    }
    if (projection_loc_ != -1) {
      glUniformMatrix4fv(projection_loc_, 1, GL_FALSE, projection_);
      assert_gl("Setting projection matrix");
    }
    state_.set_active_texture(GL_TEXTURE0);
    if (texture_ && diffuse_loc_ != -1) {
      texture_->bind();
      glUniform1i(diffuse_loc_, 0);
      assert_gl("Setting diffuse uniform");
    } else {
      state_.bind_texture(GL_TEXTURE_2D, 0);
    }
  } else {
    s_throw(std::runtime_error, "Material is invalid");
  }
}



void rmaterial_basic_t::set_program(rprogram_t *program,
                                    const string &projection_name,
                                    const string &modelview_name,
                                    const string &diffuse_name)
{
  program_ = program;
  if (program) {
    projection_loc_ = program->uniform_location(projection_name);
    modelview_loc_ = program->uniform_location(modelview_name);
    diffuse_loc_ = program->uniform_location(diffuse_name);
  } else {
    projection_loc_ = -1;
    modelview_loc_ = -1;
    diffuse_loc_ = -1;
  }
}



void rmaterial_basic_t::set_program(rprogram_t *program,
                                    int projection_key, int modelview_key,
                                    int diffuse_key)
{
  program_ = program;
  if (program) {
    projection_loc_ = program->uniform_location(projection_key);
    modelview_loc_ = program->uniform_location(modelview_key);
    diffuse_loc_ = program->uniform_location(diffuse_key);
  } else {
    projection_loc_ = -1;
    modelview_loc_ = -1;
    diffuse_loc_ = -1;
  }
}



void rmaterial_basic_t::set_projection(const mat4f_t &proj)
{
  projection_ = proj;
}



void rmaterial_basic_t::set_modelview(const mat4f_t &mv)
{
  modelview_ = mv;
}



void rmaterial_basic_t::set_texture(rtexture_t *texture)
{
  texture_ = texture;
}


} // namespace snow
