#ifndef __SNOW__GL_ENUMS_HH__
#define __SNOW__GL_ENUMS_HH__

#include "sgl.hh"

namespace snow {


enum sgl_buffer_target_t : unsigned
{
  SGL_ARRAY_BUFFER,
  SGL_ELEMENT_ARRAY_BUFFER,
  SGL_PIXEL_PACK_BUFFER,
  SGL_PIXEL_UNPACK_BUFFER,
  SGL_TEXTURE_BUFFER_, /* alias for SGL_TEXTURE_BUFFER */
  SGL_TRANSFORM_FEEDBACK_BUFFER,
  SGL_UNIFORM_BUFFER,
  SGL_COPY_READ_BUFFER,
  SGL_COPY_WRITE_BUFFER,
  SGL_DRAW_INDIRECT_BUFFER,
  SGL_ATOMIC_COUNTER_BUFFER,
  SGL_DISPATCH_INDIRECT_BUFFER,
  SGL_SHADER_STORAGE_BUFFER,
  SGL_BUFFER_TARGET_COUNT,
  SGL_BUFFER_INVALID_TARGET
};



enum sgl_texture_target_t : unsigned
{
  SGL_TEXTURE_1D,
  SGL_TEXTURE_2D,
  SGL_TEXTURE_3D,
  SGL_TEXTURE_CUBE_MAP,
  SGL_TEXTURE_BUFFER,
  SGL_TEXTURE_RECTANGLE,
  SGL_TEXTURE_1D_ARRAY,
  SGL_TEXTURE_2D_ARRAY,
  SGL_TEXTURE_2D_MULTISAMPLE,
  SGL_TEXTURE_2D_MULTISAMPLE_ARRAY,
  SGL_TEXTURE_CUBE_MAP_ARRAY,
  SGL_TEXTURE_TARGET_COUNT,
  SGL_TEXTURE_INVALID_TARGET
};




/*==============================================================================
  sgl_texture_target

    Converts the given sgl_texture_target_t to its corresponding GL texture
    target enum.
==============================================================================*/
inline GLenum sgl_texture_target_to_gl(const unsigned target)
{
  switch (target) {
  case SGL_TEXTURE_1D: return GL_TEXTURE_1D;
  case SGL_TEXTURE_2D: return GL_TEXTURE_2D;
  case SGL_TEXTURE_3D: return GL_TEXTURE_3D;
  case SGL_TEXTURE_CUBE_MAP: return GL_TEXTURE_CUBE_MAP;
#if GL_VERSION_3_1
  case SGL_TEXTURE_BUFFER: return GL_TEXTURE_BUFFER;
  case SGL_TEXTURE_RECTANGLE: return GL_TEXTURE_RECTANGLE;
#endif
#if GL_VERSION_3_0
  case SGL_TEXTURE_1D_ARRAY: return GL_TEXTURE_1D_ARRAY;
  case SGL_TEXTURE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;
#endif
#if GL_VERSION_3_2 || GL_ARB_texture_multisample
  case SGL_TEXTURE_2D_MULTISAMPLE: return GL_TEXTURE_2D_MULTISAMPLE;
  case SGL_TEXTURE_2D_MULTISAMPLE_ARRAY: return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#endif
#if GL_VERSION_4_0 || GL_ARB_texture_cube_map_array
  case SGL_TEXTURE_CUBE_MAP_ARRAY: return GL_TEXTURE_CUBE_MAP_ARRAY;
#endif
  default: return 0;
  }
}



/*==============================================================================
  sgl_texture_target_binding

    Converts the given sgl_texture_target_t to its corresponding GL texture
    target binding enum (for use with glGet).
==============================================================================*/
inline GLenum sgl_texture_target_to_gl_binding(const unsigned target)
{
  switch (target) {
  case SGL_TEXTURE_1D: return GL_TEXTURE_BINDING_1D;
  case SGL_TEXTURE_2D: return GL_TEXTURE_BINDING_2D;
  case SGL_TEXTURE_3D: return GL_TEXTURE_BINDING_3D;
  case SGL_TEXTURE_CUBE_MAP: return GL_TEXTURE_BINDING_CUBE_MAP;
#if GL_VERSION_3_1
  case SGL_TEXTURE_RECTANGLE: return GL_TEXTURE_BINDING_RECTANGLE;
#endif
#if GL_VERSION_3_0
  case SGL_TEXTURE_1D_ARRAY: return GL_TEXTURE_BINDING_1D_ARRAY;
  case SGL_TEXTURE_2D_ARRAY: return GL_TEXTURE_BINDING_2D_ARRAY;
#endif
#if GL_VERSION_3_2 || GL_ARB_texture_multisample
  case SGL_TEXTURE_2D_MULTISAMPLE: return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
  case SGL_TEXTURE_2D_MULTISAMPLE_ARRAY: return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
#endif
#if GL_VERSION_4_0 || GL_ARB_texture_cube_map_array
  case SGL_TEXTURE_CUBE_MAP_ARRAY: return GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
#endif
  default: return 0;
  }
}



/*==============================================================================
  sgl_buffer_target

    Converts the given GL buffer target to an sgl_buffer_target_t.
==============================================================================*/
inline unsigned sgl_buffer_target_from_gl(const GLenum target)
{
  switch (target) {
  case GL_ARRAY_BUFFER: return SGL_ARRAY_BUFFER;
  case GL_ELEMENT_ARRAY_BUFFER: return SGL_ELEMENT_ARRAY_BUFFER;
  case GL_PIXEL_PACK_BUFFER: return SGL_PIXEL_PACK_BUFFER;
  case GL_PIXEL_UNPACK_BUFFER: return SGL_PIXEL_UNPACK_BUFFER;
#if GL_VERSION_3_1
  case GL_TEXTURE_BUFFER: return SGL_TEXTURE_BUFFER;
#endif
#if GL_VERSION_3_0
  case GL_TRANSFORM_FEEDBACK_BUFFER: return SGL_TRANSFORM_FEEDBACK_BUFFER;
#endif
#if GL_VERSION_3_1 || GL_ARB_uniform_buffer_object
  case GL_UNIFORM_BUFFER: return SGL_UNIFORM_BUFFER;
#endif
#if GL_VERSION_3_1 || GL_ARB_copy_buffer
  case GL_COPY_READ_BUFFER: return SGL_COPY_READ_BUFFER;
  case GL_COPY_WRITE_BUFFER: return SGL_COPY_WRITE_BUFFER;
#endif
#if GL_VERSION_4_0 || GL_ARB_draw_indirect
  case GL_DRAW_INDIRECT_BUFFER: return SGL_DRAW_INDIRECT_BUFFER;
#endif
#if GL_VERSION_4_2 || GL_ARB_shader_atomic_counters
  case GL_ATOMIC_COUNTER_BUFFER: return SGL_ATOMIC_COUNTER_BUFFER;
#endif
#if GL_VERSION_4_3 || GL_ARB_compute_shader
  case GL_DISPATCH_INDIRECT_BUFFER: return SGL_DISPATCH_INDIRECT_BUFFER;
#endif
#if GL_VERSION_4_3 || GL_ARB_shader_storage_buffer_object
  case GL_SHADER_STORAGE_BUFFER: return SGL_SHADER_STORAGE_BUFFER;
#endif
  default: return SGL_BUFFER_INVALID_TARGET;
  }
}



/*==============================================================================
  sgl_buffer_target_binding

    Converts the given GL buffer target binding constant to an
    sgl_buffer_target_t.
==============================================================================*/
inline unsigned sgl_buffer_target_from_gl_binding(const GLenum target)
{
  switch (target) {
  case GL_ARRAY_BUFFER_BINDING: return SGL_ARRAY_BUFFER;
  case GL_ELEMENT_ARRAY_BUFFER_BINDING: return SGL_ELEMENT_ARRAY_BUFFER;
  case GL_PIXEL_PACK_BUFFER_BINDING: return SGL_PIXEL_PACK_BUFFER;
  case GL_PIXEL_UNPACK_BUFFER_BINDING: return SGL_PIXEL_UNPACK_BUFFER;
#if GL_VERSION_3_1
  case GL_TEXTURE_BINDING_BUFFER: return SGL_TEXTURE_BUFFER;
#endif
#if GL_VERSION_3_0
  case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING: return SGL_TRANSFORM_FEEDBACK_BUFFER;
#endif
#if GL_VERSION_3_1 || GL_ARB_uniform_buffer_object
  case GL_UNIFORM_BUFFER_BINDING: return SGL_UNIFORM_BUFFER;
#endif
#if GL_VERSION_4_0 || GL_ARB_draw_indirect
  case GL_DRAW_INDIRECT_BUFFER_BINDING: return SGL_DRAW_INDIRECT_BUFFER;
#endif
#if GL_VERSION_4_2 || GL_ARB_shader_atomic_counters
  case GL_ATOMIC_COUNTER_BUFFER_BINDING: return SGL_ATOMIC_COUNTER_BUFFER;
#endif
#if GL_VERSION_4_3 || GL_ARB_compute_shader
  case GL_DISPATCH_INDIRECT_BUFFER_BINDING: return SGL_DISPATCH_INDIRECT_BUFFER;
#endif
#if GL_VERSION_4_3 || GL_ARB_shader_storage_buffer_object
  case GL_SHADER_STORAGE_BUFFER_BINDING: return SGL_SHADER_STORAGE_BUFFER;
#endif
  default: return SGL_BUFFER_INVALID_TARGET;
  }
}



/*==============================================================================
  sgl_texture_target

    Converts the given GL texture target to an sgl_texture_target_t.
==============================================================================*/
inline unsigned sgl_texture_target_from_gl(const GLenum target)
{
  switch (target) {
  case GL_TEXTURE_1D: return SGL_TEXTURE_1D;
  case GL_TEXTURE_2D: return SGL_TEXTURE_2D;
  case GL_TEXTURE_3D: return SGL_TEXTURE_3D;
  case GL_TEXTURE_CUBE_MAP: return SGL_TEXTURE_CUBE_MAP;
#if GL_VERSION_3_1
  case GL_TEXTURE_BUFFER: return SGL_TEXTURE_BUFFER;
  case GL_TEXTURE_RECTANGLE: return SGL_TEXTURE_RECTANGLE;
#endif
#if GL_VERSION_3_0
  case GL_TEXTURE_1D_ARRAY: return SGL_TEXTURE_1D_ARRAY;
  case GL_TEXTURE_2D_ARRAY: return SGL_TEXTURE_2D_ARRAY;
#endif
#if GL_VERSION_3_2 || GL_ARB_texture_multisample
  case GL_TEXTURE_2D_MULTISAMPLE: return SGL_TEXTURE_2D_MULTISAMPLE;
  case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return SGL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#endif
#if GL_VERSION_4_0 || GL_ARB_texture_cube_map_array
  case GL_TEXTURE_CUBE_MAP_ARRAY: return SGL_TEXTURE_CUBE_MAP_ARRAY;
#endif
  default: return SGL_TEXTURE_INVALID_TARGET;
  }
}



/*==============================================================================
  sgl_texture_target_binding

    Converts the given GL texture target binding constant to an
    sgl_texture_target_t.
==============================================================================*/
inline unsigned sgl_texture_target_from_gl_binding(const GLenum target)
{
  switch (target) {
  case GL_TEXTURE_BINDING_1D: return SGL_TEXTURE_1D;
  case GL_TEXTURE_BINDING_2D: return SGL_TEXTURE_2D;
  case GL_TEXTURE_BINDING_3D: return SGL_TEXTURE_3D;
  case GL_TEXTURE_BINDING_CUBE_MAP: return SGL_TEXTURE_CUBE_MAP;
#if GL_VERSION_3_1
  case GL_TEXTURE_BINDING_RECTANGLE: return SGL_TEXTURE_RECTANGLE;
#endif
#if GL_VERSION_3_0
  case GL_TEXTURE_BINDING_1D_ARRAY: return SGL_TEXTURE_1D_ARRAY;
  case GL_TEXTURE_BINDING_2D_ARRAY: return SGL_TEXTURE_2D_ARRAY;
#endif
#if GL_VERSION_3_2 || GL_ARB_texture_multisample
  case GL_TEXTURE_BINDING_2D_MULTISAMPLE: return SGL_TEXTURE_2D_MULTISAMPLE;
  case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY: return SGL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#endif
#if GL_VERSION_4_0 || GL_ARB_texture_cube_map_array
  case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY: return SGL_TEXTURE_CUBE_MAP_ARRAY;
#endif
  default: return SGL_TEXTURE_INVALID_TARGET;
  }
}


} // namespace snow

#endif /* end __SNOW__GL_ENUMS_HH__ include guard */
