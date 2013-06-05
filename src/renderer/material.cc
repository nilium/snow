#include "material.hh"
#include "gl_error.hh"
#include "texture.hh"
#include "program.hh"
#include "constants.hh"
#include <cassert>
#include <vector>

namespace snow {


enum : GLint
{
  /*! Uniform value for uniforms that should be looked up by name. */
  UNIFORM_NOT_TESTED = -2,
  /*! Uniforms that either do not exist or could not be found in a program. */
  NO_UNIFORM = -1,
};


using uniform_map_t = std::map<int, rcustom_uniform_t>;


namespace {

const rpass_t DEFAULT_PASS = {
  false, // skip
  { GL_ONE, GL_ZERO }, // blend
  { GL_LESS, GL_TRUE }, // depth
  { ~(GLuint)0, GL_ALWAYS, 0, ~(GLuint)0, GL_KEEP, GL_KEEP, GL_KEEP }, // stencil
  nullptr, // program
  {
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT },
    { nullptr, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT }
  } // textures[8]
};


rpass_t g_pass = DEFAULT_PASS;


const uniform_map_t g_default_uniforms {
  { UNIFORM_MODELVIEW,        rcustom_uniform_t( mat4f_t::identity      ) },
  { UNIFORM_PROJECTION,       rcustom_uniform_t( mat4f_t::identity      ) },
  { UNIFORM_TEXTURE_MATRIX,   rcustom_uniform_t( mat4f_t::identity      ) },
  { UNIFORM_TEXTURE0,         rcustom_uniform_t(GLint(0)    ) },
  { UNIFORM_TEXTURE1,         rcustom_uniform_t(GLint(1)) },
  { UNIFORM_TEXTURE2,         rcustom_uniform_t(GLint(2)) },
  { UNIFORM_TEXTURE3,         rcustom_uniform_t(GLint(3)) },
  { UNIFORM_TEXTURE4,         rcustom_uniform_t(GLint(4)) },
  { UNIFORM_TEXTURE5,         rcustom_uniform_t(GLint(5)) },
  { UNIFORM_TEXTURE6,         rcustom_uniform_t(GLint(6)) },
  { UNIFORM_TEXTURE7,         rcustom_uniform_t(GLint(7)) },
};


uniform_map_t g_uniforms = g_default_uniforms;


} // namespace <anon>



rcustom_uniform_t::rcustom_uniform_t() :
  count(1),
  kind(FLOAT),
  is_opaque(false),
  mat4(mat4f_t::identity)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(GLfloat value) :
  count(1),
  kind(FLOAT),
  is_opaque(false),
  float_value(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec2f_t &value) :
  count(1),
  kind(VEC2F),
  is_opaque(false),
  vec2f(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec3f_t &value) :
  count(1),
  kind(VEC3F),
  is_opaque(false),
  vec3f(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec4f_t &value) :
  count(1),
  kind(VEC4F),
  is_opaque(false),
  vec4f(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(GLint value) :
  count(1),
  kind(INT),
  is_opaque(false),
  int_value(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec2_t<GLint> &value) :
  count(1),
  kind(VEC2I),
  is_opaque(false),
  vec2i(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec3_t<GLint> &value) :
  count(1),
  kind(VEC3I),
  is_opaque(false),
  vec3i(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec4_t<GLint> &value) :
  count(1),
  kind(VEC4I),
  is_opaque(false),
  vec4i(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(GLuint value) :
  count(1),
  kind(UNSIGNED),
  is_opaque(false),
  unsigned_value(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec2_t<GLuint> &value) :
  count(1),
  kind(VEC2U),
  is_opaque(false),
  vec2u(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec3_t<GLuint> &value) :
  count(1),
  kind(VEC3U),
  is_opaque(false),
  vec3u(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const vec4_t<GLuint> &value) :
  count(1),
  kind(VEC4U),
  is_opaque(false),
  vec4u(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const mat3f_t &value) :
  count(1),
  kind(MAT3F),
  is_opaque(false),
  mat3(value)
{
  /* nop */
}



rcustom_uniform_t::rcustom_uniform_t(const mat4f_t &value) :
  count(1),
  kind(MAT4F),
  is_opaque(false),
  mat4(value)
{
  /* nop */
}



void rcustom_uniform_t::apply(const rprogram_t &program, int name, GLint uniform_location_hint) const
{
  const GLint uniform_location =
    (uniform_location_hint > NO_UNIFORM)
    ? uniform_location_hint
    : program.uniform_location(name);

  if (uniform_location == NO_UNIFORM) {
    return;
  }

  assert(count >= 1);

  #define PTR_OR_VAL(NAME, TYPE) (const TYPE *)(is_opaque ? opaque : &NAME)

  switch (kind) {
  case FLOAT:
    glUniform1fv(uniform_location, count, PTR_OR_VAL(float_value, GLfloat));
    break;
  case VEC2F:
    glUniform2fv(uniform_location, count, PTR_OR_VAL(vec2f, GLfloat));
    break;
  case VEC3F:
    glUniform3fv(uniform_location, count, PTR_OR_VAL(vec3f, GLfloat));
    break;
  case VEC4F:
    glUniform4fv(uniform_location, count, PTR_OR_VAL(vec4f, GLfloat));
    break;
  case INT:
    glUniform1iv(uniform_location, count, PTR_OR_VAL(int_value, GLint));
    break;
  case VEC2I:
    glUniform2iv(uniform_location, count, PTR_OR_VAL(vec2i, GLint));
    break;
  case VEC3I:
    glUniform3iv(uniform_location, count, PTR_OR_VAL(vec3i, GLint));
    break;
  case VEC4I:
    glUniform4iv(uniform_location, count, PTR_OR_VAL(vec4i, GLint));
    break;
  case UNSIGNED:
    glUniform1uiv(uniform_location, count, PTR_OR_VAL(unsigned_value, GLuint));
    break;
  case VEC2U:
    glUniform2uiv(uniform_location, count, PTR_OR_VAL(vec2u, GLuint));
    break;
  case VEC3U:
    glUniform3uiv(uniform_location, count, PTR_OR_VAL(vec3u, GLuint));
    break;
  case VEC4U:
    glUniform4uiv(uniform_location, count, PTR_OR_VAL(vec4u, GLuint));
    break;
  case MAT2F:
    glUniformMatrix2fv(uniform_location, count, GL_FALSE, PTR_OR_VAL(mat3, GLfloat));
    break;
  case MAT3F:
    glUniformMatrix3fv(uniform_location, count, GL_FALSE, PTR_OR_VAL(mat3, GLfloat));
    break;
  case MAT4F:
    glUniformMatrix4fv(uniform_location, count, GL_FALSE, PTR_OR_VAL(mat4, GLfloat));
    break;
  default:
    s_throw(std::runtime_error, "Invalid uniform type");
    return;
  // case MAT23F:
  // case MAT24F:
  // case MAT32F:
  // case MAT42F:
  // case MAT34F:
  // case MAT43F:
  }

  assert_gl("Setting uniform");

  #undef PTR_OR_VAL
}



void rpass_t::apply() const
{
  const bool blend_func_changed = (
    blend.sfactor != g_pass.blend.sfactor ||
    blend.dfactor != g_pass.blend.dfactor
    );
  if (blend_func_changed) {
    glBlendFunc(blend.sfactor, blend.dfactor);
    g_pass.blend = blend;
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
  g_pass.depth = depth;

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
  g_pass.stencil = stencil;

  if (!program) {
    glUseProgram(0);
    g_pass.program = nullptr;
    return;
  } else if (program != g_pass.program) {
    program->use();
    g_pass.program = program;
  }

  for (GLuint index = 0; index < MAX_TEXTURE_UNITS; ++index) {
    rtexture_t *texture = textures[index].texture;

    if (texture != g_pass.textures[index].texture) {
      glActiveTexture(GL_TEXTURE0 + index);
      assert_gl("Setting active texture unit");
      if (texture) {
        textures[index].texture->bind();
      }
    }

    if (!texture) {
      // If there's no texture in this unit, all further units must be empty
      // as well (or the pass is malformed)
      break;
    }

    texture->set_filters(textures[index].mag_filter, textures[index].min_filter);
    texture->set_wrapping(textures[index].x_wrap, textures[index].y_wrap);

    g_pass.textures[index] = textures[index];

    assert_gl("Binding texture uniform");
  }

  // Because there's no diff for uniform state (yet), apply them per-pass
  rmaterial_t::apply_uniforms(*program);

  #ifndef NDEBUG
  if (!program->validate()) {
    s_throw(std::runtime_error, "Shader failed validation: %s", program->error_string().c_str());
  }
  #endif
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
  set_uniform(UNIFORM_PROJECTION, rcustom_uniform_t(proj));
  // g_projection = proj;
}



void rmaterial_t::set_modelview(const mat4f_t &mv)
{
  set_uniform(UNIFORM_MODELVIEW, rcustom_uniform_t(mv));
  // g_modelview = mv;
}



void rmaterial_t::set_texture_matrix(const mat4f_t &tm)
{
  set_uniform(UNIFORM_TEXTURE_MATRIX, rcustom_uniform_t(tm));
  // g_texture_matrix = tm;
}



void rmaterial_t::set_uniform(int name, const rcustom_uniform_t &uniform)
{
  g_uniforms[name] = uniform;
}



void rmaterial_t::unset_uniform(int name)
{
  const uniform_map_t::const_iterator found_name = g_uniforms.find(name);
  if (found_name != g_uniforms.cend()) {
    g_uniforms.erase(found_name);
  }
}



void rmaterial_t::clear_uniforms()
{
  g_uniforms = g_default_uniforms;
}



void rmaterial_t::apply_uniforms(const rprogram_t &program)
{
  uniform_map_t::const_iterator found_name;
  const uniform_map_t::const_iterator end = g_uniforms.cend();
  for (const auto &binding : program.bound_uniforms()) {
    found_name = g_uniforms.find(binding.first);
    if (found_name != end) {
      found_name->second.apply(program, binding.first, binding.second.first);
    }
  }
}


} // namespace snow
