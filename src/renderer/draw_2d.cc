#include "draw_2d.hh"
#include "buffer.hh"
#include "gl_error.hh"
#include "material.hh"
#include "vertex_array.hh"
#include <cstring>


namespace snow {


constexpr size_t DEFAULT_FACE_CAPACITY = 64;
constexpr size_t DEFAULT_VERTEX_CAPACITY = DEFAULT_FACE_CAPACITY * 2;



/*!
  Constructor
*/
rdraw_2d_t::rdraw_2d_t() :
  transform_(mat3f_t::identity),
  scale_(vec2f_t::one),
  origin_(vec2f_t::zero),
  handle_(vec2f_t::zero),
  screen_size_({800, 600}),
  rotation_(0),
  transform_dirty_(false),
  vertices_(),
  faces_(),
  stages_()
{
  vertices_.reserve(DEFAULT_VERTEX_CAPACITY);
  faces_.reserve(DEFAULT_FACE_CAPACITY);
}



/*!
  Move constructor
*/
rdraw_2d_t::rdraw_2d_t(rdraw_2d_t &&other) :
  transform_(other.transform_),
  scale_(other.scale_),
  origin_(other.origin_),
  handle_(other.handle_),
  screen_size_(other.screen_size_),
  rotation_(other.rotation_),
  transform_dirty_(other.transform_dirty_),
  vertices_(std::move(other.vertices_)),
  faces_(std::move(other.faces_)),
  stages_(std::move(other.stages_))
{
  other.reset_and_clear();
}



/*!
  Copy-assignment operator
*/
rdraw_2d_t &rdraw_2d_t::operator = (const rdraw_2d_t &other)
{
  if (&other != this) {
    transform_ = other.transform_;
    scale_ = other.scale_;
    origin_ = other.origin_;
    handle_ = other.handle_;
    screen_size_ = other.screen_size_;
    rotation_ = other.rotation_;
    transform_dirty_ = other.transform_dirty_;
    vertices_ = other.vertices_;
    faces_ = other.faces_;
    stages_ = other.stages_;
  }
  return *this;
}



/*!
  Move-assignment operator
*/
rdraw_2d_t &rdraw_2d_t::operator = (rdraw_2d_t &&other)
{
  if (&other != this) {
    transform_ = other.transform_;
    scale_ = other.scale_;
    origin_ = other.origin_;
    handle_ = other.handle_;
    screen_size_ = other.screen_size_;
    rotation_ = other.rotation_;
    transform_dirty_ = other.transform_dirty_;
    vertices_ = std::move(other.vertices_);
    faces_ = std::move(other.faces_);
    stages_ = std::move(other.stages_);

    other.reset_and_clear();
  }
  return *this;
}



/*!
  Draws the contents of the 2D drawer using the given vertex array object. The
  VAO must have been previously built using build_vertex_array.

  \param in vao The vertex array object. This must have been ubilt using
  build_vertex_array.
  \param in ib_where The offset into the element array buffer associated with
  the vertex array object that the index data is.
*/
void rdraw_2d_t::draw_with_vertex_array(rvertex_array_t &vao, GLintptr ib_where)
{
  constexpr float Z_MIN = -10;
  constexpr float Z_MAX = 10;

  vao.bind();

  int index = 0;
  const auto length = stages_.size();
  rmaterial_t *cur_material = nullptr;
  rmaterial_t::set_projection(
    mat4f_t::orthographic(0, screen_size_.x, screen_size_.y, 0, Z_MIN, Z_MAX));

  for (; index < length; ++index) {
    #ifdef NDEBUG
    const draw_stage_t &current = stages_[index];
    #else
    const draw_stage_t &current = stages_.at(index);
    #endif

    cur_material = current.material;
    if (!cur_material) {
      s_log_note("Skipping empty material draw");
      continue;
    }

    const size_t passes = cur_material->num_passes();
    for (int pass = 0; pass < passes; ++pass) {
      cur_material->prepare_pass(pass);
      GLvoid *offset = (GLvoid *)(ib_where + (current.base_index * sizeof(uint16_t)));
      glDrawElementsBaseVertex(GL_TRIANGLES, current.num_indices,
                               GL_UNSIGNED_SHORT, offset, current.base_vertex);
      assert_gl("Drawing 2D elements");
    }
  }

  rvertex_array_t::unbind();
  assert_gl("Unbinding vertex array object");
}



/*!
  Buffers the vertex data into a buffer object at the given location.

  \param in buffer The buffer object to store the vertex data.
  \param in vb_where The location in the buffer object to place the vertex data.
*/
void rdraw_2d_t::buffer_vertices(rbuffer_t &buffer, GLintptr vb_where)
{
  const GLsizeiptr vb_size = vertex_buffer_size();
  const GLintptr vb_max  = vb_where + vb_size;

  if (vb_max > buffer.size()) {
    buffer.resize(vb_max, true);
  }

  // Buffer vertex data:
  buffer.bind_as(GL_ARRAY_BUFFER);
  // positions
  glBufferSubData(GL_ARRAY_BUFFER, vb_where, vb_size, vertices_.data());
  assert_gl("Buffering 2D vertices");
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assert_gl("Unbinding vertex array object");
}



/*!
  Buffers the index data into a buffer object at the given location.

  \param in buffer The buffer object to store the index data.
  \param in ib_where The location in the buffer object to place the index data.
*/
void rdraw_2d_t::buffer_indices(rbuffer_t &buffer, GLintptr ib_where)
{
  // Buffer indices:
  const GLsizeiptr ib_size = index_buffer_size();
  const GLintptr ib_max = ib_where + ib_size;

  if (ib_max > buffer.size()) {
    buffer.resize(ib_max, true);
  }

  buffer.bind_as(GL_ELEMENT_ARRAY_BUFFER);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, ib_where, ib_size, faces_.data());
  assert_gl("Buffering 2D indices");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  assert_gl("Unbinding element array object");
}



rvertex_array_t rdraw_2d_t::build_vertex_array(const GLuint position_attrib,
  const GLuint texcoord_attrib, const GLuint color_attrib, rbuffer_t &vertices,
  const GLintptr vb_where, rbuffer_t &indices)
{
  rvertex_array_t vao;

  // Build vao without initializer

  indices.bind_as(GL_ELEMENT_ARRAY_BUFFER);
  vao.bind();
  vertices.bind_as(GL_ARRAY_BUFFER);

  // Enable attribute arrays
  vao.enable_attrib(position_attrib);
  vao.enable_attrib(color_attrib);
  vao.enable_attrib(texcoord_attrib);

  const GLintptr position_offset = vb_where + offsetof(vertex_t, position);
  const GLintptr texcoord_offset = vb_where + offsetof(vertex_t, texcoord);
  const GLintptr color_offset    = vb_where + offsetof(vertex_t, color);

  // Bind vertex attributes to buffer offsets
  vao.bind_attrib(position_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), position_offset);
  vao.bind_attrib(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), texcoord_offset);
  vao.bind_attrib(color_attrib,    4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), color_offset);

  rvertex_array_t::unbind();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assert_gl("Unbinding array buffer");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  assert_gl("Unbinding element array buffer");

  return vao;
}



GLsizeiptr rdraw_2d_t::vertex_buffer_size() const
{
  return vertices_.size() * sizeof(vertex_t);
}



GLsizeiptr rdraw_2d_t::index_buffer_size() const
{
  return faces_.size() * sizeof(face_buffer_t::value_type);
}



void rdraw_2d_t::clear_buffers()
{
  vertices_.clear();
  faces_.clear();
}



void rdraw_2d_t::clear()
{
  vertices_.clear();
  faces_.clear();
  stages_.clear();
}



void rdraw_2d_t::reset()
{
  transform_       = mat3f_t::identity;
  scale_           = vec2f_t::one;
  origin_          = vec2f_t::zero;
  handle_          = vec2f_t::zero;
  screen_size_     = { 800, 600 }; // TODO: get actual screen size?
  rotation_        = 0;
  transform_dirty_ = false;
}



void rdraw_2d_t::reset_and_clear()
{
  reset();
  clear();
}



void rdraw_2d_t::draw_offset_rect(const vec2f_t &pos, const vec2f_t &size,
  const vec4f_t &color, rmaterial_t *material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_rect(offset_to_screen(pos), size, color, material, uv_min, uv_max);
}



void rdraw_2d_t::draw_offset_rect_raw(const vec2f_t &pos, const vec2f_t &size,
  const vec4f_t &color, rmaterial_t *material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_rect_raw(offset_to_screen(pos), size, color, material, uv_min, uv_max);
}



void rdraw_2d_t::draw_rect(const vec2f_t &pos, const vec2f_t &size,
  const vec4f_t &color, rmaterial_t *material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_stage_t &stage = push_draw_stage(material, 4);

  const mat3f_t &tform = vertex_transform();
  const vec2f_t fpos = origin_ + pos;

  const auto vb_length = vertices_.size();
  const uint16_t base_vertex = uint16_t(vb_length - stage.base_vertex);
  const uint16_t bv1 = base_vertex + 1;
  const uint16_t bv2 = base_vertex + 2;
  const uint16_t bv3 = base_vertex + 3;

  vertices_.reserve(vb_length + 4);

  const vec2f_t upleft = -(handle_ * size);
  const vec2f_t bottomright = size + upleft;

  // top-left
  vertices_.push_back({
    fpos + (tform * upleft),
    { uv_min.x, uv_max.y },
    color
  });
  // top right
  vertices_.push_back({
    fpos + (tform * vec2f_t::make(bottomright.x, upleft.y)),
    uv_max,
    color
  });
  // bottom right
  vertices_.push_back({
    fpos + (tform * bottomright),
    { uv_max.x, uv_min.y },
    color
  });
  // bottom left
  vertices_.push_back({
    fpos + (tform * vec2f_t::make(upleft.x, bottomright.y)),
    uv_min,
    color
  });

  faces_.reserve(faces_.size() + 2);

  faces_.push_back({ base_vertex, bv1, bv2 });
  faces_.push_back({ base_vertex, bv2, bv3 });

  stage.num_indices += 6;
  stage.num_vertices += 4;
}



void rdraw_2d_t::draw_rect_raw(const vec2f_t &pos, const vec2f_t &size,
  const vec4f_t &color, rmaterial_t *material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_stage_t &stage = push_draw_stage(material, 4);

  const auto vb_length = vertices_.size();
  const uint16_t base_vertex = uint16_t(vb_length - stage.base_vertex);
  const uint16_t bv1 = base_vertex + 1;
  const uint16_t bv2 = base_vertex + 2;
  const uint16_t bv3 = base_vertex + 3;

  vertices_.reserve(vb_length + 4);

  vertices_.push_back({
    pos,
    { uv_min.x, uv_max.y },
    color
  });
  vertices_.push_back({
    { pos.x + size.x, pos.y },
    uv_max,
    color
  });
  vertices_.push_back({
    { pos.x + size.x, pos.y + size.y },
    { uv_max.x, uv_min.y },
    color
  });
  vertices_.push_back({
    { pos.x, pos.y + size.y },
    uv_min,
    color
  });

  faces_.reserve(faces_.size() + 2);

  faces_.push_back({ base_vertex, bv1, bv2 });
  faces_.push_back({ base_vertex, bv2, bv3 });

  stage.num_indices += 6;
  stage.num_vertices += 4;
}



void rdraw_2d_t::draw_triangles(
  const vertex_t *verts, const GLint num_verts,
  const face_t *tris, const GLint num_tris,
  rmaterial_t *material)
{
  assert(num_verts > 0);
  assert(num_tris > 0);
  draw_stage_t &stage = push_draw_stage(material, num_verts);

  const auto vb_length = vertices_.size();
  const uint16_t base_vertex = uint16_t(vb_length - stage.base_vertex);
  vertices_.resize(vb_length + num_verts);
  faces_.reserve(faces_.size() + num_tris);

  std::memcpy(&vertices_[vb_length], verts, num_verts * sizeof(*verts));

  for (int tri_index = 0; tri_index < num_tris; ++tri_index) {
    face_t face = tris[tri_index];
    face.v0 += base_vertex;
    face.v1 += base_vertex;
    face.v2 += base_vertex;
    faces_.push_back(face);
  }

  stage.num_indices += num_tris * 3;
  stage.num_vertices += num_verts;
}



void rdraw_2d_t::draw_triangle(const vertex_t vertices[3], rmaterial_t *material)
{
  static const face_t face = { 0, 1, 2 };
  draw_triangles(vertices, 3, &face, 1, material);
}



void rdraw_2d_t::set_rotation(const float angle_deg)
{
  rotation_ = angle_deg;
  transform_dirty_ = true;
}



void rdraw_2d_t::set_scale(const vec2f_t &scale)
{
  scale_ = scale;
  transform_dirty_ = true;
}



void rdraw_2d_t::set_origin(const vec2f_t &origin)
{
  origin_ = origin;
}



void rdraw_2d_t::set_handle(const vec2f_t &handle)
{
  handle_ = handle;
}



void rdraw_2d_t::set_screen_size(const vec2f_t &size)
{
  screen_size_ = size;
}



vec2f_t rdraw_2d_t::screen_to_offset(const vec2f_t &v) const
{
  return v * screen_size_.inverse();
}



vec2f_t rdraw_2d_t::offset_to_screen(const vec2f_t &v) const
{
  return v * screen_size_;
}



auto rdraw_2d_t::push_draw_stage(rmaterial_t *material,
  const GLint vertices_needed) -> draw_stage_t &
{
  if (material == NULL) {
    s_throw(std::invalid_argument, "Material cannot be NULL");
  }
  if (!stages_.empty()) {
    // keep inside if-block since the current stage will be invalidated if it's
    // not valid given the conditions, and I don't want an invalid reference
    // hanging around.
    draw_stage_t &current = stages_.back();
    if (material == current.material &&
        (current.num_vertices + vertices_needed) < UINT16_MAX) {
      return current;
    }
  }

  stages_.emplace_back(*this, material);
  return stages_.back();
}



const mat3f_t &rdraw_2d_t::vertex_transform()
{
  if (transform_dirty_) {
    transform_ = mat3f_t::scaling({scale_.x, scale_.y, 1})
                 .multiply(mat3f_t::rotation(rotation_, vec3f_t::neg_Z));
    transform_dirty_ = false;
  }
  return transform_;
}



/*******************************************************************************
*                            rdraw_2d_t::draw_stage_t                            *
*******************************************************************************/

rdraw_2d_t::draw_stage_t::draw_stage_t(const rdraw_2d_t &draw, rmaterial_t *mat)
: material(mat),
  base_index((GLint)draw.faces_.size() * 3),
  base_vertex((GLint)draw.vertices_.size()),
  num_vertices(0),
  num_indices(0)
{}


} // namespace snow
