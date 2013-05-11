#include "material.hh"
#include "gl_error.hh"
#include "texture.hh"
#include "program.hh"
#include "constants.hh"
#include <cassert>

namespace snow {

constexpr const GLint UNIFORM_NOT_TESTED = -2;
constexpr const GLint NO_UNIFORM = -1;

static const rpass_t DEFAULT_PASS = {
  false, // skip
  { GL_ONE, GL_ZERO }, // blend
  { GL_LESS, GL_TRUE }, // depth
  { ~(GLuint)0, GL_ALWAYS, ~(GLuint)0, 0, GL_KEEP, GL_KEEP, GL_KEEP }, // stencil
  nullptr, // program
  UNIFORM_NOT_TESTED, // modelview_
  UNIFORM_NOT_TESTED, // projection_
  {
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, UNIFORM_NOT_TESTED }
  } // textures[8]
};

static rpass_t g_pass = DEFAULT_PASS;

static mat4f_t g_projection = mat4f_t::identity;
static mat4f_t g_modelview = mat4f_t::identity;



void rpass_t::apply() const
{
  const bool blend_func_changed = (
    blend.sfactor != g_pass.blend.sfactor ||
    blend.dfactor != g_pass.blend.dfactor
    );
  if (blend_func_changed) {
    glBlendFunc(blend.sfactor, blend.dfactor);
    assert_gl("Setting blend function");
  }

  if (depth.func != g_pass.depth.func) {
    glDepthFunc(depth.func);
    assert_gl("Setting depth function");
  }

  if (depth.write != g_pass.depth.write) {
    glDepthMask(depth.write);
    assert_gl("Setting depth write");
  }

  if (stencil.mask != g_pass.stencil.mask) {
    glStencilMask(stencil.mask);
    assert_gl("Setting stencil mask");
  }

  const bool stencil_func_changed = (
    stencil.ref_mask != g_pass.stencil.ref_mask ||
    stencil.func != g_pass.stencil.func ||
    stencil.ref != g_pass.stencil.ref
    );
  if (stencil_func_changed) {
    glStencilFunc(stencil.func, stencil.ref, stencil.ref_mask);
    assert_gl("Setting stencil function");
  }

  const bool stencil_op_changed = (
    stencil.fail != g_pass.stencil.fail ||
    stencil.depth_fail != g_pass.stencil.depth_fail ||
    stencil.depth_pass != g_pass.stencil.depth_pass
    );
  if (stencil_op_changed) {
    glStencilOp(stencil.fail, stencil.depth_fail, stencil.depth_pass);
    assert_gl("Setting stencil op");
  }

  if (!program) {
    goto pass_finished;
  }

  if (program != g_pass.program) {
    program->use();
    if (modelview_ == UNIFORM_NOT_TESTED) {
      modelview_ = program->uniform_location(UNIFORM_MODELVIEW);
    }
    if (modelview_ != -1) {
      glUniformMatrix4fv(modelview_, 1, GL_FALSE, g_modelview);
    }
    if (projection_ == UNIFORM_NOT_TESTED) {
      projection_ = program->uniform_location(UNIFORM_PROJECTION);
    }
    if (projection_ != -1) {
      glUniformMatrix4fv(projection_, 1, GL_FALSE, g_projection);
    }
  }

  for (GLuint index = 0; index < MAX_TEXTURE_UNITS; ++index) {
    glActiveTexture(GL_TEXTURE0 + index);
    GLint &uniform = textures[index].uniform_;

    if (uniform == UNIFORM_NOT_TESTED) {
      uniform = program->uniform_location(UNIFORM_TEXTURE0 + index);
    }

    if (uniform == NO_UNIFORM) {
      continue;
    }

    const GLenum target = textures[index].texture->target();
    rtexture_t *texture = textures[index].texture;

    if (texture != g_pass.textures[index].texture) {
      if (texture) {
        textures[index].texture->bind();
      }
    }

    if (!texture) {
      continue;
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, textures[index].min_filter);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, textures[index].mag_filter);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, textures[index].x_wrap);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, textures[index].y_wrap);

    glUniform1i(uniform, index);
  }

pass_finished:
  g_pass = *this;
}



const rpass_t &rpass_t::defaults()
{
  return DEFAULT_PASS;
}



void rpass_t::reset_pass_state()
{
  g_pass = defaults();
}



rmaterial_t::rmaterial_t()
{
}



rmaterial_t::~rmaterial_t()
{
}



bool rmaterial_t::valid() const
{
  return true;
}



size_t rmaterial_t::num_passes() const
{
  return num_passes_;
}



void rmaterial_t::set_num_passes(size_t num)
{
  assert(num < MAX_PASSES);
  num_passes_ = num;
}



bool rmaterial_t::prepare_pass(size_t passnum) const
{
  assert(passnum < num_passes_);
  const rpass_t &pass = passes_[passnum];
  if (pass.skip) {
    return false;
  }

  pass.apply();

  return true;
}



rpass_t &rmaterial_t::pass(size_t pass)
{
  assert(pass < num_passes_);
  return passes_[pass];
}



const rpass_t &rmaterial_t::pass(size_t pass) const
{
  assert(pass < num_passes_);
  return passes_[pass];
}



void rmaterial_t::set_projection(const mat4f_t &proj)
{
  g_projection = proj;
}



void rmaterial_t::set_modelview(const mat4f_t &mv)
{
  g_modelview = mv;
}






} // namespace snow
