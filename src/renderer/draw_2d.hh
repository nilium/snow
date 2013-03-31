#ifndef __SNOW__DRAW_2D_HH__
#define __SNOW__DRAW_2D_HH__

#include <snow/config.hh>
#include <snow/math/math3d.hh>
#include "sgl.hh"
#include <vector>


namespace snow {


struct gl_state_t;
struct rbuffer_t;
struct rmaterial_t;
struct rvertex_array_t;


struct rdraw_2d
{
  friend gl_state_t;

  rdraw_2d(gl_state_t &state);
  ~rdraw_2d() = default;

  rdraw_2d(const rdraw_2d &other) = default;
  rdraw_2d &operator = (const rdraw_2d &other);

  rdraw_2d(rdraw_2d &&other);
  rdraw_2d &operator = (rdraw_2d &&other);


/*******************************************************************************
*                          2D stage-draw / buffer ops                          *
*******************************************************************************/

  void draw_with_vertex_array(rvertex_array_t &vao, rbuffer_t &indices, GLintptr ib_where);

  /*
    Builds a vertex array object with the 2D drawing's data and returns it.
    If either buffer cannot contain its respective data given the offset, the
    buffer will be resized to at least fit it exactly.
    If restore_vao is true and there was a VAO bound prior to calling this
    method, it will be restored. If not, the new VAO will remain bound after
    the call ends.
  */
  void build_vertex_array(rvertex_array_t &vao,
                          GLuint pos_attrib, GLuint tex_attrib,
                          GLuint col_attrib, rbuffer_t &vertices,
                          GLintptr vb_where, rbuffer_t &indices,
                          GLintptr ib_where, bool restore_vao = false
                          );

  GLsizeiptr positions_buffer_size() const;
  GLsizeiptr texcoords_buffer_size() const;
  GLsizeiptr colors_buffer_size() const;
  GLsizeiptr vertex_buffer_size() const;
  GLsizeiptr index_buffer_size() const;

  void clear(); // clears internal buffers, does not reset drawing state
  void reset(); // resets drawing state
  void reset_and_clear() { reset(); clear(); }


/*******************************************************************************
*                                2D drawing ops                                *
*******************************************************************************/

  void draw_rect(const vec2f_t &pos, const vec2f_t &size,
                 const vec4_t<uint8_t> &color, rmaterial_t *material,
                 const vec2f_t &uv_min = vec2f_t::zero,
                 const vec2f_t &uv_max = vec2f_t::one);

  // Draws a rectangle without applying any of the transformations/state
  // handled by the 2D state.
  void draw_rect_raw(const vec2f_t &pos, const vec2f_t &size,
                     const vec4_t<uint8_t> &color, rmaterial_t *material,
                     const vec2f_t &uv_min = vec2f_t::zero,
                     const vec2f_t &uv_max = vec2f_t::one);


/*******************************************************************************
*                                   2D state                                   *
*******************************************************************************/

  // Set the rotation of drawing operations.
  void set_rotation(float angle_deg);
  // Set the scale of drawing operations.
  void set_scale(const vec2f_t &scale);
  // Set the drawing origin. This is translation applied to all drawing before
  // rotation and scale are applied.
  void set_origin(const vec2f_t &origin);
  // Set the handle for drawing operations. Specifies an offset relative to the
  // position of all drawing operations. E.g., for a rect of 20,20, a handle of
  // 10,10 will cause it to be drawn and rotated about its center.
  void set_handle(const vec2f_t &handle);
  // Used to set the projection matrix.
  void set_screen_size(const vec2_t<uint16_t> &size);


private:

  struct draw_stage_t {
    draw_stage_t(const rdraw_2d &draw, rmaterial_t *mat);

    rmaterial_t *   material;    // may not be nullptr
    GLint           base_index;
    GLint           base_vertex; // = vertices_.size() pre-insert
    vec2_t<uint16_t> screen_size;
    // must be able to check for num_vertices + N > UINT16_MAX
    uint32_t        num_vertices;
    // keep in mind, three per face
    uint32_t        num_indices;
  };


  using face_t    = vec3_t<uint16_t>;
  using vbuffer_t = std::vector<vec2f_t>;
  using cbuffer_t = std::vector<vec4_t<uint8_t>>;
  using fbuffer_t = std::vector<face_t>;
  using sbuffer_t = std::vector<draw_stage_t>;


  draw_stage_t &push_draw_stage(rmaterial_t *material, GLint vertices_needed);
  const mat3f_t &vertex_transform();


  gl_state_t &        state_;
  mat3f_t             transform_;
  vec2f_t             scale_;
  vec2f_t             origin_;
  vec2f_t             handle_;
  vec2_t<uint16_t>    screen_size_;
  float               rotation_;
  bool                transform_dirty_;
  vbuffer_t           vpositions_;
  vbuffer_t           vtexcoords_;
  cbuffer_t           vcolors_;
  fbuffer_t           faces_;
  sbuffer_t           stages_;
};


} // namespace snow

#endif /* end __SNOW__DRAW_2D_HH__ include guard */
