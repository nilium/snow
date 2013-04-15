#include "vertex_array.hh"
#include "gl_state.hh"
#include "gl_error.hh"

namespace snow {


/*==============================================================================
  constructor (no initializer)

    Creates a VAO without an initializer function.
==============================================================================*/
rvertex_array_t::rvertex_array_t(gl_state_t &gl)
: state_(gl), initfn_(nullptr), inited_(false), name_(0)
{
}



/*==============================================================================
  constructor (with initializer)

    Creates a VAO with an initializer function.
==============================================================================*/
rvertex_array_t::rvertex_array_t(gl_state_t &gl, init_fn_t &&initfn)
: state_(gl), initfn_(initfn), inited_(false), name_(0)
{
}



/*==============================================================================
  move constructor

    Moves a VAO to this object.
    The moved VAO is reset and may be reused as needed or safely destroyed.
==============================================================================*/
rvertex_array_t::rvertex_array_t(rvertex_array_t &&other)
: state_(other.state_), initfn_(std::move(other.initfn_)),
  inited_(other.inited_), name_(other.name_)
{
  other.zero();
}



/*==============================================================================
  move assignment

    Moves a VAO to this object, unloading any existing VAO held by this one.
    The moved VAO is reset and may be reused as needed or safely destroyed.
==============================================================================*/
rvertex_array_t &rvertex_array_t::operator = (rvertex_array_t &&other)
{
  if (&other != this) {
    if (&state_ != &other.state_) {
      s_throw(std::invalid_argument, "Cannot move vertex array - GL states do not match");
    }
    unload();

    initfn_ = std::move(other.initfn_);
    inited_ = other.inited_;
    name_ = other.name_;

    other.zero();
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
  set_initializer(nullptr_t)

    Removes the initializer function for the VAO. If the VAO was already loaded,
    unload() will be called.
==============================================================================*/
void rvertex_array_t::set_initializer(std::nullptr_t np)
{
  unload();
  initfn_ = nullptr;
}




/*==============================================================================
  set_initializer(non-null function)

    Sets the initializer function for the VAO. If the VAO was already loaded,
    unload() will be called.
==============================================================================*/
void rvertex_array_t::set_initializer(init_fn_t &&initfn)
{
  unload();
  initfn_ = initfn;
}



/*==============================================================================
  load

    Initializes the vertex array object and then restores whatever previously
    bound vertex array there was.
==============================================================================*/
void rvertex_array_t::load()
{
  if (!inited_) {
    GLuint prev_binding = state_.vertex_array();
    force_load();
    state_.bind_vertex_array(prev_binding);
  }
}



/*==============================================================================
  bind

    Binds the current vertex array object. On first binding, the VAO will be
    loaded using the initializer function if one was provided.
==============================================================================*/
void rvertex_array_t::bind()
{
  if (name_ == 0 || !inited_) {
    force_load();
  } else {
    state_.bind_vertex_array(name_);
  }
}



/*==============================================================================
  unload

    Releases any GL resources used by the VAO and marks the VAO as
    uninitialized. The VAO will be re-initialized on next binding.
==============================================================================*/
void rvertex_array_t::unload()
{
  if (name_ != 0) {
    inited_ = false;
    if (state_.vertex_array() == name_) {
      state_.bind_vertex_array(0);
    }
    glDeleteVertexArrays(1, &name_);
    assert_gl("Deleting vertex array object");
  }
}



/*==============================================================================
  zero

    Zeroes out the VAO. This _does not_ unload resources used by the VAO. Do
    not call it unless the VAO has been first unloaded or its resources are
    being managed by another object (e.g., moved to another rvertex_array_t).
==============================================================================*/
void rvertex_array_t::zero()
{
  initfn_ = nullptr;
  inited_ = false;
  name_ = 0;
}


/*==============================================================================
  force_load

    Generates the VAO, binds it, and loads it using the initializer function if
    one was provided.
==============================================================================*/
void rvertex_array_t::force_load()
{
  if (name_ == 0) {
    glGenVertexArrays(1, &name_);
    assert_gl("Generating vertex array object");
  }

  state_.bind_vertex_array(name_);

  if (initfn_) {
    inited_ = initfn_(state_);
    if (!inited_) {
      s_throw(std::runtime_error, "Initializing vertex array object failed");
    }
    // In case the init function doesn't do error checking, which it should.
    // Check after inited_ though, since the init function might be reporting
    // that it failed because of an error it found.
    assert_gl("Initializing vertex array object");
  } else {
    // Assume initialization occurs without a provided function.
    inited_ = true;
  }
}


} // namespace snow
