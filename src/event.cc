/*
  event.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "event.hh"

namespace snow {


namespace {


GLFWwindow *g_main_window = nullptr;


} // namespace <anon>


GLFWwindow *main_window()
{
  return g_main_window;
}



void set_main_window(GLFWwindow *window)
{
  g_main_window = window;
}


} // namespace snow
