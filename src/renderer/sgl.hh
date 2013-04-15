#ifndef __SNOW__SGL_HH__
#define __SNOW__SGL_HH__

#include "../config.hh"

#if S_PLATFORM_MAC
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "sgl_extensions.hh"
#include "sgl_enums.hh"

#define GLFW_INCLUDE_GLCOREARB
#include <gl/glfw3.h>

#endif /* end __SNOW__SGL_HH__ include guard */
