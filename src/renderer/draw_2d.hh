/*
  draw_2d.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__DRAW_2D_HH__
#define __SNOW__DRAW_2D_HH__

#include "../config.hh"
#include <snow/math/math3d.hh>
#include "sgl.hh"
#include <vector>


namespace snow {


struct rbuffer_t;
struct rmaterial_t;
struct rvertex_array_t;


struct rdraw_2d_t
{
  struct alignas(uint16_t) face_t
  {
    uint16_t v0, v1, v2;
  };

  struct alignas(float) vertex_t
  {
    vec2f_t         position;
    vec2f_t         texcoord;
    vec4f_t color;
  };



  rdraw_2d_t();
  ~rdraw_2d_t() = default;

  rdraw_2d_t(const rdraw_2d_t &other) = default;
  rdraw_2d_t &operator = (const rdraw_2d_t &other);

  rdraw_2d_t(rdraw_2d_t &&other);
  rdraw_2d_t &operator = (rdraw_2d_t &&other);


/*******************************************************************************
*                          2D stage-draw / buffer ops                          *
*******************************************************************************/

  void draw_with_vertex_array(rvertex_array_t &vao, const GLintptr ib_where = 0);

  void buffer_vertices(rbuffer_t &buffer, const GLintptr vb_where = 0);
  void buffer_indices(rbuffer_t &buffer, const GLintptr ib_where = 0);

  /*
    Builds a vertex array object with the 2D drawing's data and returns it.
    If either buffer cannot contain its respective data given the offset, the
    buffer will be resized to at least fit it exactly. There will be no VAO
    bound on return.
  */
  rvertex_array_t build_vertex_array(const GLuint position_attrib,
    const GLuint texcoord_attrib, const GLuint color_attrib,
    rbuffer_t &vertices, const GLintptr vb_where,
    rbuffer_t &indices);

  GLsizeiptr vertex_buffer_size() const;
  GLsizeiptr index_buffer_size() const;

  void clear_buffers(); // Clears the buffers but doesn't clear the drawing stages
  void clear(); // clears internal buffers, does not reset drawing state
  void reset(); // resets drawing state
  void reset_and_clear();


/*******************************************************************************
*                                2D drawing ops                                *
*******************************************************************************/

  // Rect drawing

  // draw_offset_* methods take a 0-1 position that is scaled to the current
  // screen size as set by set_screen_size(..). A position at 0,0 is the bottom
  // left, 1,1 the top right. Width and height are still measured in whatever
  // units are relevant to the screen resolution.
  // With tform
  void draw_offset_rect(const vec2f_t &pos, const vec2f_t &size,
    const vec4f_t &color, rmaterial_t *material,
    const vec2f_t &uv_min = vec2f_t::zero, const vec2f_t &uv_max = vec2f_t::one);

  // No tform
  void draw_offset_rect_raw(const vec2f_t &pos, const vec2f_t &size,
    const vec4f_t &color, rmaterial_t *material,
    const vec2f_t &uv_min = vec2f_t::zero, const vec2f_t &uv_max = vec2f_t::one);

  void draw_rect(const vec2f_t &pos, const vec2f_t &size,
                 const vec4f_t &color, rmaterial_t *material,
                 const vec2f_t &uv_min = vec2f_t::zero,
                 const vec2f_t &uv_max = vec2f_t::one);

  // Draws a rectangle without applying any of the transformations/state
  // handled by the 2D state.
  void draw_rect_raw(const vec2f_t &pos, const vec2f_t &size,
                     const vec4f_t &color, rmaterial_t *material,
                     const vec2f_t &uv_min = vec2f_t::zero,
                     const vec2f_t &uv_max = vec2f_t::one);


  // This does not attempt to transform the vertices at all
  void draw_triangles(const vertex_t *verts, GLint num_verts,
                      const face_t *tris, GLint num_tris,
                      rmaterial_t *material);

  void draw_triangle(const vertex_t vertices[3], rmaterial_t *material);


/*******************************************************************************
*                                   2D state                                   *
*******************************************************************************/

  // Set the rotation of drawing operations.
  void set_rotation(const float angle_deg);
  // Set the scale of drawing operations.
  void set_scale(const vec2f_t &scale);
  // Set the drawing origin. This is translation applied to all drawing before
  // rotation and scale are applied.
  void set_origin(const vec2f_t &origin);
  // Set the handle for drawing operations. Specifies an offset relative to the
  // position and size of all drawing operations. E.g., for a rect of 20,20, a
  // handle of 0.5,0.5 will cause it to be drawn and rotated about its center.
  // A handle of 1,1 will cause the object to be drawn from its bottom-right
  // corner.
  void set_handle(const vec2f_t &handle);
  // Used to set the projection matrix.
  void set_screen_size(const vec2f_t &size);
  // Converts a screen vector to an offset vector.
  vec2f_t screen_to_offset(const vec2f_t &v) const;
  // Converts an offset vector to a screen vector.
  vec2f_t offset_to_screen(const vec2f_t &v) const;


  inline const vec2f_t &handle() const { return handle_; }
  inline const vec2f_t &origin() const { return origin_; }
  inline const vec2f_t &scale() const { return scale_; }
  inline const vec2f_t &screen_size() const { return screen_size_; }
  inline float rotation() const { return rotation_; }


private:

  struct draw_stage_t {
    draw_stage_t(const rdraw_2d_t &draw, rmaterial_t *mat);

    rmaterial_t *   material;    // may not be nullptr
    GLint           base_index;
    GLint           base_vertex; // = vertices_.size() pre-insert
    // must be able to check for num_vertices + N > UINT16_MAX
    uint32_t        num_vertices;
    // keep in mind, three per face
    uint32_t        num_indices;
  };


  // Check that vertex components are packed
  static_assert(offsetof(vertex_t, position) + sizeof(vertex_t::position)
                == offsetof(vertex_t, texcoord),
                "Vertex has padding between position and texcoord");
  static_assert(offsetof(vertex_t, texcoord) + sizeof(vertex_t::texcoord)
                == offsetof(vertex_t, color),
                "Vertex has padding between texcoord and color");


  using vertex_buffer_t = std::vector<vertex_t>;
  using face_buffer_t = std::vector<face_t>;
  using stage_buffer_t = std::vector<draw_stage_t>;


  draw_stage_t &push_draw_stage(rmaterial_t *material, const GLint vertices_needed);
  const mat3f_t &vertex_transform();


  mat3f_t             transform_;
  vec2f_t             scale_;
  vec2f_t             origin_;
  vec2f_t             handle_;
  vec2f_t             screen_size_;
  float               rotation_;
  bool                transform_dirty_;
  vertex_buffer_t           vertices_;
  face_buffer_t           faces_;
  stage_buffer_t           stages_;
};


} // namespace snow

#endif /* end __SNOW__DRAW_2D_HH__ include guard */
