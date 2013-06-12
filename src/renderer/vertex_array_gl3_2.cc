/*
  vertex_array_gl3_2.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "vertex_array.hh"
#include "gl_error.hh"
#include "buffer.hh"

// If Snow can use GL's own vertex array object implementation, do so, otherwise
// don't compile this implementation.
#if GL_VERSION_3_0 || GL_ES_VERSION_3_0 || GL_OES_vertex_array_object

#if !GL_VERSION_3_0 && !GL_ES_VERSION_3_0 && GL_OES_vertex_array_object
// Make use of OES_vertex_array_object if neither GLES 3 nor GL 3.2 are provided
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#endif

namespace snow {


/*==============================================================================
  constructor

    Creates a VAO. The VAO is not generated until bound.
==============================================================================*/
rvertex_array_t::rvertex_array_t() :
  name_(0)
{
}



/*==============================================================================
  move constructor

    Moves a VAO to this object.
    The moved VAO is reset and may be reused as needed or safely destroyed.
==============================================================================*/
rvertex_array_t::rvertex_array_t(rvertex_array_t &&other) :
  name_(other.name_)
{
  other.name_ = 0;
}



/*==============================================================================
  move assignment

    Moves a VAO to this object, unloading any existing VAO held by this one.
    The moved VAO is reset and may be reused as needed or safely destroyed.
==============================================================================*/
rvertex_array_t &rvertex_array_t::operator = (rvertex_array_t &&other)
{
  if (&other != this) {
    unload();

    name_ = other.name_;
    other.name_ = 0;
  }
  return *this;
}



/*==============================================================================
  destructor

    Destroys the VAO.
==============================================================================*/
rvertex_array_t::~rvertex_array_t()
{
  unload();
}



/*==============================================================================
  bind

    Binds the current vertex array object. On first binding, the VAO will be
    generated.
==============================================================================*/
void rvertex_array_t::bind()
{
  if (name_ == 0) {
    glGenVertexArrays(1, &name_);
    assert_gl("Generating vertex array object");
  }
  glBindVertexArray(name_);
  assert_gl("Binding vertex array object");
}



void rvertex_array_t::unbind()
{
  glBindVertexArray(0);
  assert_gl("Unbinding vertex array object");
}



/*==============================================================================
  unload

    Releases any GL resources used by the VAO.
==============================================================================*/
void rvertex_array_t::unload()
{
  if (name_ != 0) {
    glDeleteVertexArrays(1, &name_);
    assert_gl("Deleting vertex array object");
    name_ = 0;
  }
}



void rvertex_array_t::enable_attrib(GLuint index)
{
  glEnableVertexAttribArray(index);
  assert_gl("Enabling vertex attrib array");
}



void rvertex_array_t::disable_attrib(GLuint index)
{
  glDisableVertexAttribArray(index);
  assert_gl("Disabling vertex attrib array");
}



void rvertex_array_t::bind_attrib(GLuint index, GLint size, GLenum type,
  GLboolean normalized, GLsizei stride, GLintptr offset)
{
  glVertexAttribPointer(index, size, type, normalized, stride, (const GLvoid *)offset);
  assert_gl("Setting vertex attrib pointer");
}


} // namespace snow

#endif // GL_VERSION_3_0 || GL_ES_VERSION_3_0 || GL_OES_vertex_array_object
