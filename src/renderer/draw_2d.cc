#include "draw_2d.hh"
#include "buffer.hh"
#include "gl_error.hh"
#include "gl_state.hh"
#include "material.hh"
#include "vertex_array.hh"
#include <cstring>


namespace snow {


constexpr size_t DEFAULT_FACE_CAPACITY = 64;
constexpr size_t DEFAULT_VERTEX_CAPACITY = DEFAULT_FACE_CAPACITY * 2;



rdraw_2d_t::rdraw_2d_t(gl_state_t &gl)
: state_(gl),
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



rdraw_2d_t::rdraw_2d_t(rdraw_2d_t &&other) :
  state_(other.state_),
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



rdraw_2d_t &rdraw_2d_t::operator = (rdraw_2d_t &&other)
{
  if (&other != this) {
    if (!gl_state_t::compatible(*this, other)) {
      s_throw(std::invalid_argument, "Cannot move 2D drawer: GL states are incompatible");
    }

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



void rdraw_2d_t::draw_with_vertex_array(rvertex_array_t &vao, rbuffer_t &indices, GLintptr ib_where)
{
  constexpr float Z_MIN = -10;
  constexpr float Z_MAX = 10;

  vao.bind();
  indices.bind();

  int index = 0;
  mat4f_t projection;
  vec2_t<int16_t> screen = {0, 0};
  bool set_proj = true;
  const auto length = stages_.size();
  const draw_stage_t *stage_data = stages_.data();
  rmaterial_t *cur_material = nullptr;

  for (; index < length; ++index) {
    const draw_stage_t &current = stage_data[index];

    if (current.screen_size != screen) {
      // rebuild projection matrix if needed
      screen = current.screen_size;
      projection = mat4f_t::orthographic(0, screen.x, screen.y, 0, Z_MIN, Z_MAX);
      set_proj = true;
    }

    if (cur_material != current.material) {
      cur_material = current.material;
      if (!cur_material)
        s_throw(std::runtime_error, "Material is null");
      cur_material->set_modelview(mat4f_t::identity);
      set_proj = true;
    }

    if (set_proj) {
      cur_material->set_projection(projection);
      set_proj = false;
    }

    const int passes = cur_material->passes();
    for (int pass = 0; pass < passes; ++pass) {
      cur_material->prepare_pass(pass);
      GLvoid *offset = (GLvoid *)(ib_where + (current.base_index * sizeof(uint16_t)));
      glDrawElementsBaseVertex(GL_TRIANGLES, current.num_indices,
                               GL_UNSIGNED_SHORT, offset, current.base_vertex);
      assert_gl("Drawing 2D elements");
    }
  }

  state_.bind_vertex_array(0);
}



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
}



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
}



rvertex_array_t rdraw_2d_t::build_vertex_array(const GLuint pos_attrib,
  const GLuint tex_attrib, const GLuint col_attrib, rbuffer_t &vertices,
  const GLintptr vb_where, const bool restore_vao)
{
  // Make sure GL states are compatible
  rvertex_array_t vao(state_);

  // Build vao without initializer
  auto previous_vao = (restore_vao ? state_.vertex_array() : 0);
  vao.bind();
  vertices.bind();

  // Enable attribute arrays
  state_.set_attrib_array_enabled(pos_attrib, true);
  state_.set_attrib_array_enabled(col_attrib, true);
  state_.set_attrib_array_enabled(tex_attrib, true);


  // Bind vertex attributes to buffer offsets
  glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
    (const GLvoid *)(vb_where + offsetof(vertex_t, position)));
  assert_gl("Setting vertex position attrib");
  glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t),
    (const GLvoid *)(vb_where + offsetof(vertex_t, texcoord)));
  assert_gl("Setting vertex texture coords attrib");
  glVertexAttribPointer(col_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t),
    (const GLvoid *)(vb_where + offsetof(vertex_t, color)));
  assert_gl("Setting vertex color attrib");

  // reset VAO binding
  if (restore_vao)
    state_.bind_vertex_array(previous_vao);

  return vao;
}



GLsizeiptr rdraw_2d_t::vertex_buffer_size() const
{
  return vertices_.size() * sizeof(vertex_t);
}



GLsizeiptr rdraw_2d_t::index_buffer_size() const
{
  return faces_.size() * sizeof(fbuffer_t::value_type);
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
  const vec4_t<uint8_t> &color, rmaterial_t *const material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_rect(offset_to_screen(pos), size, color, material, uv_min, uv_max);
}



void rdraw_2d_t::draw_offset_rect_raw(const vec2f_t &pos, const vec2f_t &size,
  const vec4_t<uint8_t> &color, rmaterial_t *const material,
  const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_rect_raw(offset_to_screen(pos), size, color, material, uv_min, uv_max);
}



void rdraw_2d_t::draw_rect(const vec2f_t &pos, const vec2f_t &size,
  const vec4_t<uint8_t> &color, rmaterial_t *const material,
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
}



void rdraw_2d_t::draw_rect_raw(const vec2f_t &pos, const vec2f_t &size,
  const vec4_t<uint8_t> &color, rmaterial_t *const material,
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
}



void rdraw_2d_t::draw_triangles(
  const vertex_t *const verts, const GLsizeiptr num_verts,
  const face_t *const tris, const GLsizeiptr num_tris,
  rmaterial_t *const material)
{
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



auto rdraw_2d_t::push_draw_stage(rmaterial_t *const material,
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
        screen_size_ == current.screen_size &&
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
  base_index(draw.faces_.size() * 3),
  base_vertex(draw.vertices_.size()),
  screen_size(draw.screen_size_),
  num_vertices(0),
  num_indices(0)
{}


} // namespace snow
