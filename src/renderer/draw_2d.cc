#include "draw_2d.hh"
#include "buffer.hh"
#include "gl_error.hh"
#include "gl_state.hh"
#include "material.hh"
#include "vertex_array.hh"

namespace snow {


constexpr size_t DEFAULT_FACE_CAPACITY = 64;
constexpr size_t DEFAULT_VERTEX_CAPACITY = DEFAULT_FACE_CAPACITY * 2;



rdraw_2d::rdraw_2d(gl_state_t &gl)
: state_(gl),
  transform_(mat3f_t::identity),
  scale_(vec2f_t::one),
  origin_(vec2f_t::zero),
  handle_(vec2f_t::zero),
  screen_size_({800, 600}),
  rotation_(0),
  transform_dirty_(false),
  vpositions_(),
  vtexcoords_(),
  vcolors_(),
  faces_(),
  stages_()
{
  vpositions_.reserve(DEFAULT_VERTEX_CAPACITY);
  vtexcoords_.reserve(DEFAULT_VERTEX_CAPACITY);
  vcolors_.reserve(DEFAULT_VERTEX_CAPACITY);
  faces_.reserve(DEFAULT_FACE_CAPACITY);
}



rdraw_2d::rdraw_2d(rdraw_2d &&other) :
  state_(other.state_),
  transform_(other.transform_),
  scale_(other.scale_),
  origin_(other.origin_),
  handle_(other.handle_),
  screen_size_(other.screen_size_),
  rotation_(other.rotation_),
  transform_dirty_(other.transform_dirty_),
  vpositions_(std::move(other.vpositions_)),
  vtexcoords_(std::move(other.vtexcoords_)),
  vcolors_(std::move(other.vcolors_)),
  faces_(std::move(other.faces_)),
  stages_(std::move(other.stages_))
{
  other.reset_and_clear();
}



rdraw_2d &rdraw_2d::operator = (const rdraw_2d &other)
{
  if (&other != this) {
    transform_ = other.transform_;
    scale_ = other.scale_;
    origin_ = other.origin_;
    handle_ = other.handle_;
    screen_size_ = other.screen_size_;
    rotation_ = other.rotation_;
    transform_dirty_ = other.transform_dirty_;
    vpositions_ = other.vpositions_;
    vtexcoords_ = other.vtexcoords_;
    vcolors_ = other.vcolors_;
    faces_ = other.faces_;
    stages_ = other.stages_;
  }
  return *this;
}



rdraw_2d &rdraw_2d::operator = (rdraw_2d &&other)
{
  if (&other != this) {
    if (!gl_state_t::compatible(*this, other)) {
      throw std::invalid_argument("Cannot move 2D drawer: GL states are incompatible");
    }

    transform_ = other.transform_;
    scale_ = other.scale_;
    origin_ = other.origin_;
    handle_ = other.handle_;
    screen_size_ = other.screen_size_;
    rotation_ = other.rotation_;
    transform_dirty_ = other.transform_dirty_;
    vpositions_ = std::move(other.vpositions_);
    vtexcoords_ = std::move(other.vtexcoords_);
    vcolors_ = std::move(other.vcolors_);
    faces_ = std::move(other.faces_);
    stages_ = std::move(other.stages_);

    other.reset_and_clear();
  }
  return *this;
}



void rdraw_2d::draw_with_vertex_array(rvertex_array_t &vao, rbuffer_t &indices, GLintptr ib_where)
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
        throw std::runtime_error("Material is null");
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
}



void rdraw_2d::build_vertex_array(rvertex_array_t &vao,
                                  GLuint pos_attrib, GLuint tex_attrib,
                                  GLuint col_attrib, rbuffer_t &vertices,
                                  GLintptr vb_where, rbuffer_t &indices,
                                  GLintptr ib_where, bool restore_vao)
{
  const GLsizeiptr pb_size = positions_buffer_size();
  const GLsizeiptr tb_size = texcoords_buffer_size();
  const GLsizeiptr cb_size = colors_buffer_size();
  const GLsizeiptr ib_size = index_buffer_size();
  const GLintptr tb_where  = vb_where + pb_size;
  const GLintptr cb_where  = tb_where + tb_size;

  // Make sure GL states are compatible
  if (!(gl_state_t::compatible(indices, state_) &&
        gl_state_t::compatible(vertices, state_) &&
        gl_state_t::compatible(vao, state_))) {
    throw std::invalid_argument("Incompatible GL states for buffer objects");
  }

  if (cb_where + cb_size > vertices.size()) {
    vertices.resize(cb_where + cb_size, true);
  }

  if (ib_where + ib_size > indices.size()) {
    indices.resize(ib_where + ib_size, true);
  }

  // Buffer vertex data:
  vertices.bind_as(GL_ARRAY_BUFFER);
  // positions
  glBufferSubData(GL_ARRAY_BUFFER, vb_where, pb_size, vpositions_.data());
  assert_gl("Buffering 2D vertex positions");
  // texcoords
  glBufferSubData(GL_ARRAY_BUFFER, tb_where, tb_size, vtexcoords_.data());
  assert_gl("Buffering 2D vertex texture coords");
  // colors
  glBufferSubData(GL_ARRAY_BUFFER, cb_where, cb_size, vcolors_.data());
  assert_gl("Buffering 2D vertex colors");

  // Buffer indices:
  indices.bind_as(GL_ELEMENT_ARRAY_BUFFER);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, ib_where, ib_size, faces_.data());
  assert_gl("Buffering 2D indices");

  // Build vao without initializer
  auto previous_vao = (restore_vao ? state_.vertex_array() : 0);
  vao.bind();

  // Enable attribute arrays
  state_.set_attrib_array_enabled(pos_attrib, true);
  state_.set_attrib_array_enabled(col_attrib, true);
  state_.set_attrib_array_enabled(tex_attrib, true);

  // Bind vertex attributes to buffer offsets
  glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)vb_where);
  assert_gl("Setting vertex position attrib");
  glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)tb_where);
  assert_gl("Setting vertex texture coords attrib");
  glVertexAttribPointer(col_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (const GLvoid *)cb_where);
  assert_gl("Setting vertex color attrib");

  // reset VAO binding
  if (restore_vao)
    state_.bind_vertex_array(previous_vao);
}



GLsizeiptr rdraw_2d::positions_buffer_size() const
{
  return vpositions_.size() * sizeof(vbuffer_t::value_type);
}



GLsizeiptr rdraw_2d::colors_buffer_size() const
{
  return vcolors_.size() * sizeof(cbuffer_t::value_type);
}



GLsizeiptr rdraw_2d::texcoords_buffer_size() const
{
  return vtexcoords_.size() * sizeof(vbuffer_t::value_type);
}



GLsizeiptr rdraw_2d::vertex_buffer_size() const
{
  return positions_buffer_size() + texcoords_buffer_size();
}



GLsizeiptr rdraw_2d::index_buffer_size() const
{
  return faces_.size() * sizeof(fbuffer_t::value_type);
}



void rdraw_2d::clear()
{
  vpositions_.clear();
  vtexcoords_.clear();
  faces_.clear();
  stages_.clear();
}



void rdraw_2d::reset()
{
  transform_       = mat3f_t::identity;
  scale_           = vec2f_t::one;
  origin_          = vec2f_t::zero;
  handle_          = vec2f_t::zero;
  screen_size_     = { 800, 600 }; // TODO: get actual screen size?
  rotation_        = 0;
  transform_dirty_ = false;
}



void rdraw_2d::draw_rect(const vec2f_t &pos, const vec2f_t &size,
                         const vec4_t<uint8_t> &color, rmaterial_t *material,
                         const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_stage_t &stage = push_draw_stage(material, 4);

  const mat3f_t &tform = vertex_transform();
  const vec2f_t fpos = origin_ + pos;

  auto vb_length = vpositions_.size();
  const uint16_t base_vertex = uint16_t(vb_length - stage.base_vertex);
  const uint16_t bv1 = base_vertex + 1;
  const uint16_t bv2 = base_vertex + 2;
  const uint16_t bv3 = base_vertex + 3;

  vpositions_.reserve(vb_length + 4);
  vtexcoords_.reserve(vb_length + 4);
  vcolors_.reserve(vb_length + 4);

  vec2f_t upleft = -handle_;
  vec2f_t bottomright = size + upleft;

  vpositions_.push_back(fpos + (tform * upleft));
  vpositions_.push_back(fpos + (tform * vec2f_t::make(bottomright.x, upleft.y)));
  vpositions_.push_back(fpos + (tform * bottomright));
  vpositions_.push_back(fpos + (tform * vec2f_t::make(upleft.x, bottomright.y)));

  vtexcoords_.push_back({ uv_min.x, uv_max.y });
  vtexcoords_.push_back(uv_max);
  vtexcoords_.push_back({ uv_max.x, uv_min.y });
  vtexcoords_.push_back(uv_min);

  vcolors_.insert(vcolors_.end(), 4, color);

  faces_.reserve(faces_.size() + 2);

  faces_.push_back({ base_vertex, bv1, bv2 });
  faces_.push_back({ base_vertex, bv2, bv3 });

  stage.num_indices += 6;
}



void rdraw_2d::draw_rect_raw(const vec2f_t &pos, const vec2f_t &size,
                             const vec4_t<uint8_t> &color, rmaterial_t *material,
                             const vec2f_t &uv_min, const vec2f_t &uv_max)
{
  draw_stage_t &stage = push_draw_stage(material, 4);

  const auto vb_length = vpositions_.size();
  const uint16_t base_vertex = uint16_t(vb_length - stage.base_vertex);
  const uint16_t bv1 = base_vertex + 1;
  const uint16_t bv2 = base_vertex + 2;
  const uint16_t bv3 = base_vertex + 3;

  vpositions_.reserve(vb_length + 4);
  vtexcoords_.reserve(vb_length + 4);
  vcolors_.reserve(vb_length + 4);

  vpositions_.push_back(pos);
  vpositions_.push_back({ pos.x + size.x, pos.y });
  vpositions_.push_back({ pos.x + size.x, pos.y + size.y });
  vpositions_.push_back({ pos.x, pos.y + size.y });

  vtexcoords_.push_back({ uv_min.x, uv_max.y });
  vtexcoords_.push_back(uv_max);
  vtexcoords_.push_back({ uv_max.x, uv_min.y });
  vtexcoords_.push_back(uv_min);

  vcolors_.insert(vcolors_.end(), 4, color);

  faces_.reserve(faces_.size() + 2);

  faces_.push_back({ base_vertex, bv1, bv2 });
  faces_.push_back({ base_vertex, bv2, bv3 });

  stage.num_indices += 6;
}



void rdraw_2d::set_rotation(float r)
{
  rotation_ = r;
  transform_dirty_ = true;
}



void rdraw_2d::set_scale(const vec2f_t &scale)
{
  scale_ = scale;
  transform_dirty_ = true;
}



void rdraw_2d::set_origin(const vec2f_t &origin)
{
  origin_ = origin;
}



void rdraw_2d::set_handle(const vec2f_t &handle)
{
  handle_ = handle;
}



void rdraw_2d::set_screen_size(const vec2_t<uint16_t> &size)
{
  screen_size_ = size;
}



auto rdraw_2d::push_draw_stage(rmaterial_t *material, GLint vertices_needed) -> draw_stage_t &
{
  if (material == NULL) {
    throw std::invalid_argument("Material cannot be NULL");
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



const mat3f_t &rdraw_2d::vertex_transform()
{
  if (transform_dirty_) {
    transform_ = mat3f_t::scaling({scale_.x, scale_.y, 1})
                 .multiply(mat3f_t::rotation(rotation_, vec3f_t::neg_Z));
    transform_dirty_ = false;
  }
  return transform_;
}



/*******************************************************************************
*                            rdraw_2d::draw_stage_t                            *
*******************************************************************************/

rdraw_2d::draw_stage_t::draw_stage_t(const rdraw_2d &draw, rmaterial_t *mat)
: material(mat),
  base_index(draw.faces_.size() * 3),
  base_vertex(draw.vpositions_.size()),
  screen_size(draw.screen_size_),
  num_vertices(0),
  num_indices(0)
{}


} // namespace snow
