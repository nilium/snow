#include "vertex_array.hh"
#include "gl_error.hh"

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


} // namespace snow
