#include "vertex_array.hh"
#include "gl_state.hh"
#include "gl_error.hh"

namespace snow {
namespace renderer {



/*==============================================================================
  constructor (no initializer)

    Description
==============================================================================*/
rvertex_array_t::rvertex_array_t(gl_state_t &gl)
: state_(gl), initfn_(nullptr), inited_(false), name_(0)
{
}



/*==============================================================================
  constructor (with initializer)

    Description
==============================================================================*/
rvertex_array_t::rvertex_array_t(gl_state_t &gl, init_fn_t &&initfn)
: state_(gl), initfn_(initfn), inited_(false), name_(0)
{
}



/*==============================================================================
  move constructor

    Description
==============================================================================*/
rvertex_array_t::rvertex_array_t(rvertex_array_t &&other)
: state_(other.state_), initfn_(std::move(other.initfn_)),
  inited_(other.inited_), name_(other.name_)
{
  other.zero();
}



/*==============================================================================
  move assignment

    Description
==============================================================================*/
rvertex_array_t &rvertex_array_t::operator = (rvertex_array_t &&other)
{
  if (&other != this) {
    if (&state_ != &other.state_) {
      throw std::invalid_argument("Cannot move vertex array - GL states do not match");
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

    Description
==============================================================================*/
rvertex_array_t::~rvertex_array_t()
{
  unload();
}




/*==============================================================================
  set_initializer(nullptr_t)

    Description
==============================================================================*/
void rvertex_array_t::set_initializer(std::nullptr_t np)
{
  unload();
  initfn_ = nullptr;
}




/*==============================================================================
  set_initializer(non-null function)

    Description
==============================================================================*/
void rvertex_array_t::set_initializer(init_fn_t &&initfn)
{
  unload();
  initfn_ = initfn;
}



/*==============================================================================
  load

    Description
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

    Description
==============================================================================*/
void rvertex_array_t::bind()
{
  if (name_ == 0 || !inited_) {
    force_load();
    state_.bind_vertex_array(0);
  }

  state_.bind_vertex_array(name_);
}



/*==============================================================================
  unload

    Description
==============================================================================*/
void rvertex_array_t::unload()
{
  if (name_ != 0) {
    inited_ = false;
    glDeleteVertexArrays(1, &name_);
    assert_gl("Deleting vertex array object");
  }
}



/*==============================================================================
  zero

    Description
==============================================================================*/
void rvertex_array_t::zero()
{
  initfn_ = nullptr;
  inited_ = false;
  name_ = 0;
}


/*==============================================================================
  force_load

    Description
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
      throw std::runtime_error("Initializing vertex array object failed");
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



} // namespace renderer
} // namespace snow
