#include "program.hh"
#include "gl_state.hh"
#include "gl_error.hh"
#include "shader.hh"
#include <iostream>

namespace snow {


namespace {

/*==============================================================================
  get_program_info_log

    Gets the program's info log as a string and returns it.
==============================================================================*/
string get_program_info_log(GLuint program)
{
  GLint log_length = 0;
  std::vector<GLchar> log_temp;

  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  assert_gl("Getting program info log length");

  log_temp.resize(log_length);

  glGetProgramInfoLog(program, log_length, NULL, log_temp.data());

  return string((const char *)log_temp.data(), log_length);
}

} // namespace <anon>



/*==============================================================================
  constructor

    Description
==============================================================================*/
rprogram_t::rprogram_t()
: program_(glCreateProgram()), linked_(false)
{
}




/*==============================================================================
 constructor

 Description
 ==============================================================================*/
rprogram_t::~rprogram_t()
{
  unload();
}



/*==============================================================================
  move constructor

    Description
==============================================================================*/
rprogram_t::rprogram_t(rprogram_t &&program)
: program_(program.program_), linked_(program.linked_),
  uniforms_(std::move(program.uniforms_)), names_(std::move(program.names_)),
  error_str_(std::move(program.error_str_))
{
  program.zero();
}



/*==============================================================================
    assignment operator

    Description
==============================================================================*/
rprogram_t &rprogram_t::operator = (rprogram_t &&program)
{
  if (this != &program) {
    unload();

    program_ = program.program_;
    linked_ = program.linked_;
    uniforms_ = std::move(program.uniforms_);
    names_ = std::move(program.names_);
    error_str_ = std::move(program.error_str_);

    program.zero();
  }

  return *this;
}



/*==============================================================================
    use

    Description
==============================================================================*/
void rprogram_t::use()
{
  if (!usable())
    s_throw(std::runtime_error, "Shader program is not in a usable state");

  glUseProgram(program_);
}



/*==============================================================================
    bind_uniform

    Description
==============================================================================*/
void rprogram_t::bind_uniform(int key, const string &name)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to bind uniform location: "
                             "program is invalid");
  }

  if (uniforms_.find(key) != uniforms_.end()) {
    s_throw(std::invalid_argument, "Uniform key already bound to program");
  }

  auto ins_result = names_.insert(name);
  uniform_loc_t loc { -1, ins_result.first };

  if (linked()) {
    load_uniform(loc);
  }

  uniforms_.emplace(key, loc);
}



/*==============================================================================
    uniform_location

    Description
==============================================================================*/
GLint rprogram_t::uniform_location(int key) const
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to get uniform location (int): "
                             "program is invalid");
  } else if (!linked()) {
    return -1;
  }

  uniforms_t::const_iterator res = uniforms_.find(key);
  if (res != uniforms_.end()) {
    return res->second.first;
  } else {
    return -1;
  }
}



/*==============================================================================
    uniform_location

    Description
==============================================================================*/
GLint rprogram_t::uniform_location(const string &name) const
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to get uniform location (string): "
                             "program is invalid");
  } else if (!linked()) {
    return -1;
  }

  GLint loc = glGetUniformLocation(program_, name.c_str());
  assert_gl("Getting uniform location by name");
  return loc;
}



/*==============================================================================
    bind_frag_out

    Description
==============================================================================*/
void rprogram_t::bind_frag_out(GLuint colorNumber, const string &name)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to bind fragment output (no index): "
                             "program is invalid");
  }

  glBindFragDataLocation(program_, colorNumber, name.c_str());
  assert_gl("Binding fragment data location (no index)");
}



/*==============================================================================
    bind_frag_out

    Description
==============================================================================*/
#if GL_VERSION_3_3 || GL_ARB_blend_func_extended
void rprogram_t::bind_frag_out(GLuint colorNumber, GLuint index, const string &name)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to bind fragment output (indexed): "
                             "program is invalid");
  }

  glBindFragDataLocationIndexed(program_, colorNumber, index, name.c_str());
  assert_gl("Binding fragment data location (indexed)");
}
#endif



/*==============================================================================
    bind_attrib

    Description
==============================================================================*/
void rprogram_t::bind_attrib(GLuint location, const string &name)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to bind attribute: program is invalid");
  }

  glBindAttribLocation(program_, location, name.c_str());
  assert_gl("Binding attribute");
}



/*==============================================================================
    attach_shader

    Description
==============================================================================*/
void rprogram_t::attach_shader(const rshader_t &shader)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to attach shader: program is invalid");
  } else if (!shader.valid()) {
    s_throw(std::invalid_argument, "Unable to attach shader: shader is invalid");
  }

  glAttachShader(program_, shader.shader_);
  assert_gl("Attaching shader to program object");
}



/*==============================================================================
    detach_shader

    Description
==============================================================================*/
void rprogram_t::detach_shader(const rshader_t &shader)
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to attach shader: program is invalid");
  } else if (!shader.valid()) {
    s_throw(std::invalid_argument, "Unable to attach shader: shader is invalid");
  }

  glDetachShader(program_, shader.shader_);
  assert_gl("Detaching shader from program object");
}



/*==============================================================================
    link

    Description
==============================================================================*/
bool rprogram_t::link()
{
  if (!valid()) {
    s_throw(std::runtime_error, "Unable to link: program is invalid");
  }

  glLinkProgram(program_);
  assert_gl("Linking program object");

  GLint link_status = 0;

  glGetProgramiv(program_, GL_LINK_STATUS, &link_status);
  assert_gl("Getting GL_LINK_STATUS");

  if (link_status != GL_TRUE) {
    error_str_ = get_program_info_log(program_);
  } else {
    error_str_.clear();
    load_uniforms();
  }

  linked_ = (link_status == GL_TRUE);
  return linked_;
}



/*==============================================================================
    validate

    Description
==============================================================================*/
bool rprogram_t::validate()
{
  if (!linked()) {
    s_throw(std::runtime_error, "Unable to get uniform location: "
                             "program is not linked");
  }

  GLint validate_status = 0;
  glValidateProgram(program_);
  assert_gl("Validating program object");
  glGetProgramiv(program_, GL_VALIDATE_STATUS, &validate_status);
  assert_gl("Getting GL_VALIDATE_STATUS");

  if (validate_status != GL_TRUE) {
    error_str_ = get_program_info_log(program_);
  } else {
    error_str_.clear();
  }

  return (validate_status == GL_TRUE);
}



/*==============================================================================
    unload

    Description
==============================================================================*/
void rprogram_t::unload()
{
  if (program_ != 0) {
    glDeleteProgram(program_);
    assert_gl("Deleting shader program");
  }
  zero();
}



/*==============================================================================
    zero

    Description
==============================================================================*/
void rprogram_t::zero()
{
  program_ = 0;
  linked_ = false;
  uniforms_.clear();
  names_.clear();
  error_str_.clear();
}



/*==============================================================================
    load_uniforms

    Description
==============================================================================*/
void rprogram_t::load_uniforms()
{
  for (auto &kvpair : uniforms_) {
    load_uniform(kvpair.second);
  }
}



/*==============================================================================
    load_uniform

    Description
==============================================================================*/
void rprogram_t::load_uniform(uniform_loc_t &loc)
{
  const GLchar *name_cstr = (const GLchar *)loc.second->c_str();
  loc.first = glGetUniformLocation(program_, name_cstr);
  assert_gl("Getting uniform location");
}


} // namespace snow
