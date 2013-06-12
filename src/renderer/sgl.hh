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
