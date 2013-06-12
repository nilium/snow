/*
  sgl.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__SGL_HH__
#define __SNOW__SGL_HH__

#include "../config.hh"

#if S_PLATFORM_MAC
// #include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#define GLFW_INCLUDE_GLCOREARB
#include <glfw/glfw3.h>

#endif /* end __SNOW__SGL_HH__ include guard */
