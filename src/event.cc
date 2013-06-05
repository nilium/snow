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
