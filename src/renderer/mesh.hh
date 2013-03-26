#ifndef __SNOW__MESH_HH__
#define __SNOW__MESH_HH__

#include <snow/config.hh>
#include <snow/math/math3d.hh>
#include <vector>
#include "sgl.hh"

namespace snow {


struct rbuffer_t;

using boneindices_t = vec3_t<unsigned short>;
using triangle_t    = vec3_t<unsigned short>;
using vertexcolor_t = vec4_t<unsigned char>;

// Intermediate vertex structure -- for convenience
struct vertex_in_t
{
  vec4f_t         position;
  vec3f_t         normal;
  vertexcolor_t   color;
  vec2f_t         texcoord0;
  vec2f_t         texcoord1;
  boneindices_t   bone_indices;
  vec3f_t         bone_weights;
};

enum rmesh_attrib_t : int
{
  ATTRIB_POSITION,
  ATTRIB_NORMAL,
  ATTRIB_BINORMAL,
  ATTRIB_COLOR,
  ATTRIB_TEXCOORD0,
  ATTRIB_TEXCOORD1,
  ATTRIB_BONE_INDICES,
  ATTRIB_BONE_WEIGHTS
};

struct rmesh_t
{
  rmesh_t();
  ~rmesh_t();

  size_t size_in_buffer() const;

  // Offsets, relative to the start of the mesh in a buffer
  inline auto positions_offset() const     -> GLintptr { return 0; }
  inline auto normals_offset() const       -> GLintptr
  {
    return positions_size();
  }
  inline auto tangents_offset() const       -> GLintptr
  {
    return normals_offset() + normals_size();
  }
  inline auto bitangents_offset() const     -> GLintptr
  {
    return tangents_offset() + tangents_size();
  }
  inline auto colors_offset() const        -> GLintptr
  {
    return bitangents_offset() + bitangents_size();
  }
  inline auto texcoord0_offset() const     -> GLintptr
  {
    return colors_offset() + colors_size();
  }
  inline auto texcoord1_offset() const     -> GLintptr
  {
    return texcoord0_offset() + texcoord0_size();
  }
  inline auto bone_weights_offset() const  -> GLintptr
  {
    return texcoord1_offset() + texcoord1_size();
  }
  inline auto bone_indices_offset() const  -> GLintptr
  {
    return bone_weights_offset() + bone_weights_size();
  }
  // Size, in buffer, of each attribute array
  inline auto positions_size() const       -> GLsizeiptr
  {
    return positions_.size() * sizeof(decltype(positions_)::value_type);
  }
  inline auto normals_size() const         -> GLsizeiptr
  {
    return normals_.size() * sizeof(decltype(normals_)::value_type);
  }
  inline auto tangents_size() const         -> GLsizeiptr
  {
    return tangents_.size() * sizeof(decltype(tangents_)::value_type);
  }
  inline auto bitangents_size() const       -> GLsizeiptr
  {
    return bitangents_.size() * sizeof(decltype(bitangents_)::value_type);
  }
  inline auto colors_size() const          -> GLsizeiptr
  {
    return colors_.size() * sizeof(decltype(colors_)::value_type);
  }
  inline auto texcoord0_size() const       -> GLsizeiptr
  {
    return texcoord0_.size() * sizeof(decltype(texcoord0_)::value_type);
  }
  inline auto texcoord1_size() const       -> GLsizeiptr
  {
    return texcoord1_.size() * sizeof(decltype(texcoord1_)::value_type);
  }
  inline auto bone_weights_size() const    -> GLsizeiptr
  {
    return weights_.size() * sizeof(decltype(weights_)::value_type);
  }
  inline auto bone_indices_size() const    -> GLsizeiptr
  {
    return indices_.size() * sizeof(decltype(indices_)::value_type);
  }

  // Stuff for statistics, not much use otherwise
  inline auto vertices_length() const      -> GLsizei { return (GLsizei)positions_.size(); }
  inline auto triangles_length() const     -> GLsizei { return (GLsizei)faces_.size(); }
  inline auto indices_length() const       -> GLsizei { return (GLsizei)triangles_length() * 3; }

  // Send vertex attribute arrays and indices to buffers.
  inline void buffer_vertices(rbuffer_t &buffer, GLintptr where)
  {
    if (!positions_.empty()) {
      buffer_positions(buffer, where);
      where += positions_size();
    }
    if (!normals_.empty()) {
      buffer_normals(buffer, where);
      where += normals_size();
    }
    if (!tangents_.empty()) {
      buffer_tangents(buffer, where);
      where += tangents_size();
    }
    if (!bitangents_.empty()) {
      buffer_bitangents(buffer, where);
      where += bitangents_size();
    }
    if (!colors_.empty()) {
      buffer_colors(buffer, where);
      where += colors_size();
    }
    if (!texcoord0_.empty()) {
      buffer_texcoord0(buffer, where);
      where += texcoord0_size();
    }
    if (!texcoord1_.empty()) {
      buffer_texcoord1(buffer, where);
      where += texcoord1_size();
    }
    if (!weights_.empty()) {
      buffer_bone_weights(buffer, where);
      where += bone_weights_size();
    }
    if (!indices_.empty()) {
      buffer_bone_indices(buffer, where);
    }
  }

  void buffer_positions(rbuffer_t &buffer, GLintptr where);
  void buffer_normals(rbuffer_t &buffer, GLintptr where);
  void buffer_tangents(rbuffer_t &buffer, GLintptr where);
  void buffer_bitangents(rbuffer_t &buffer, GLintptr where);
  void buffer_colors(rbuffer_t &buffer, GLintptr where);
  void buffer_texcoord0(rbuffer_t &buffer, GLintptr where);
  void buffer_texcoord1(rbuffer_t &buffer, GLintptr where);
  void buffer_bone_indices(rbuffer_t &buffer, GLintptr where);
  void buffer_bone_weights(rbuffer_t &buffer, GLintptr where);
  void buffer_indices(rbuffer_t &buffer, GLintptr where);

  // Draw with whatever buffers are currently bound using the given offsets for
  // vertex array data and index array data respectively.
  void draw_buffered_indices(GLsizeiptr vertices_off, GLsizeiptr indices_off) const;

  inline void add_vertex(const vertex_in_t &vertex)
  {
    add_vertex(vertex.position, vertex.normal, vertex.color,
               vertex.texcoord0, vertex.texcoord1,
               vertex.bone_weights, vertex.bone_indices);
  }

  // Adds a vertex or vertices to the mesh.
  void add_vertex(const vec4f_t &position, const vec3f_t &normal,
                  const vertexcolor_t &color,
                  const vec2f_t &uv0, const vec2f_t &uv1,
                  const vec3f_t &bone_weights = vec3f_t(),
                  const boneindices_t &bone_indices = boneindices_t());
  void add_vertices(size_t const num_vertices,
                    const vec4f_t *pos_p, const vec3f_t *norm_p,
                    const vec3f_t *binormal, const vertexcolor_t *color,
                    const vec2f_t *uv0, const vec2f_t *uv1,
                    const vec3f_t *bone_weights,
                    const boneindices_t *bone_indices);

  // Adds a triangle or triangles to the mesh.
  void add_triangle(const triangle_t &tri);
  void add_triangles(size_t const num_triangles, const triangle_t *tri_p);

  // Add a drawing stage to the mesh.
  // Modes correspond to GL primitive types (e.g., GL_TRIANGLES). Offsets and
  // lengths are those in the index buffer.
  void add_stage(GLenum mode, int index_offset, int index_length);
  void add_stages(size_t const num_stages, const GLenum *modes,
                  const int *offsets, const int *lengths);

  void compute_tangents();
  void compute_bitangents();

private:
  struct stage_t
  {
    GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.
    int offset;
    int length;
  };

  struct texcoord_t
  {
    vec2f_t uv[2];
  };

  // drawing stages
  std::vector<stage_t> stages_;
  // vertices
  std::vector<vec4f_t> positions_;
  std::vector<vec3f_t> normals_;
  std::vector<vec3f_t> tangents_;
  std::vector<vec3f_t> bitangents_;
  std::vector<vertexcolor_t> colors_;
  std::vector<vec2f_t> texcoord0_;
  std::vector<vec2f_t> texcoord1_;
  std::vector<vec3f_t> weights_;
  std::vector<boneindices_t> indices_;
  // triangle indices
  std::vector<triangle_t> faces_;
};


} // namespace snow

#endif /* end __SNOW__MESH_HH__ include guard */
