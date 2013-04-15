#ifndef __SNOW__PROGRAM_HH__
#define __SNOW__PROGRAM_HH__

#include "../config.hh"
#include "sgl.hh"
#include <map>
#include <set>

namespace snow {


struct gl_state_t;
struct rshader_t;


struct S_EXPORT rprogram_t
{
  friend struct gl_state_t;

  // For move semantics to work, program objects must share the same
  // gl_state_t, otherwise an exception will be thrown if assigning one shader
  // object to another when their state objects differ.

  rprogram_t(gl_state_t &state);
  rprogram_t(rprogram_t &&shader);
  rprogram_t &operator = (rprogram_t &&shader);
  ~rprogram_t();

  rprogram_t(const rprogram_t &)              = delete;
  rprogram_t &operator = (const rprogram_t &) = delete;

  // equiv. to glUseProgram
  // Will s_throw(std::runtime_error if usable, ) returns false.
  void            use();

  // Binds the key to the uniform name. This can be done either before or after
  // linking, attaching shaders, or any other stage of the program provided
  // valid() returns true. If valid() returns false, throws std::runtime_error.
  void            bind_uniform(int key, const string &name);
  // Returns the location of a previously-bound uniform's location. Returns -1
  // if no uniform location is found or the program is not linked.
  GLint           uniform_location(int key) const;
  // Slower alternative: looks for the uniform location based on its actual name.
  GLint           uniform_location(const string &name) const;

  void            bind_frag_out(GLuint colorNumber, const string &name);
  #if GL_VERSION_3_3 || GL_ARB_blend_func_extended
  void            bind_frag_out(GLuint colorNumber, GLuint index, const string &name);
  #endif

  // Binds a named attribute to a given location (the attribute index).
  // Attributes aren't bound until link is called. If an attribute is bound
  // after link is called, link must be called again. This is simply an error-
  // checked wrapped on top of glBindAttribLocation.
  void            bind_attrib(GLuint location, const string &name);

  inline bool     valid() const { return program_ != 0; }
  inline bool     linked() const { return linked_; }
  // Synonym for combined compiled() and linked() calls
  inline bool     usable() const { return valid() && linked(); }

  void            attach_shader(const rshader_t &shader);
  void            detach_shader(const rshader_t &shader);
  bool            link();
  // Probably shouldn't be used in release builds - will set the error string
  // with the program's info log if validation fails.
  bool            validate();

  // Deletes the shader program (if compiled), any intermediate data, and all
  // uniform bindings associated with this shader.
  void            unload();

  inline bool     has_error() const { return error_str_.length() != 0; }
  inline string   error_string() const { return error_str_; }

private:
  using name_set_t    = std::set<string>;
  using uniform_loc_t = std::pair<GLint, name_set_t::const_iterator>;
  using uniforms_t    = std::map<int, uniform_loc_t>;

  S_HIDDEN
  void            zero();

  S_HIDDEN
  void            load_uniforms();
  S_HIDDEN
  void            load_uniform(uniform_loc_t &loc);
  S_HIDDEN
  void            load_attribs();

  // GL state object, needed for use()
  gl_state_t &    state_;
  // Program object name
  GLuint          program_;
  // Whether the program has been linked yet
  bool            linked_;
  // Set of uniforms, their locations, and names iters
  uniforms_t      uniforms_;
  // Set of name strings for the program (TODO: use global name set)
  name_set_t      names_;
  // Error string, set after either link() or validate()
  string          error_str_;
};


} // namespace snow

#endif /* end __SNOW__PROGRAM_HH__ include guard */
