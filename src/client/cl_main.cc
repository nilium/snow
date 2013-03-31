#include <iostream>
#include <memory>
#include <stdexcept>

#include "cl_main.hh"
#include "../event_queue.hh"
#include "../system.hh"
#include "../renderer/buffer.hh"
#include "../renderer/gl_error.hh"
#include "../renderer/program.hh"
#include "../renderer/shader.hh"
#include "../renderer/vertex_array.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/material_basic.hh"

#include <snow-common.hh>
#include <snow/math/math3d.hh>

// Font test
#include "../renderer/font.hh"
#include "../renderer/texture.hh"
#include "../ext/stb_image.h"
#include "../data/database.hh"


namespace snow {


#define GL_QUEUE_NAME    SNOW_ORG".gl_queue"
#define FRAME_QUEUE_NAME SNOW_ORG".frame_queue"



static client_t         g_client;
static std::once_flag   g_init_flag;
static dispatch_queue_t g_gl_queue = NULL;
static dispatch_queue_t g_main_queue = NULL;



const double FRAME_HERTZ = 60;
const double FRAME_SEQ_TIME = 1000.0 / FRAME_HERTZ;



enum shader_uniform_t : int
{
  UNIFORM_MODELVIEW,
  UNIFORM_PROJECTION,
  UNIFORM_DIFFUSE
};



enum shader_attr_t : GLuint
{
  ATTR_POSITION=0,
  ATTR_TEXCOORD=4,
  ATTR_NORMAL=8,
  ATTR_COLOR=12
};



template <typename T>
GLsizei stride_of(const T &arr)
{
  if (arr.size() < 2)
    return 0;
  else {
    GLsizei elem_diff = (GLsizei)((const char *)&arr[1] - (const char *)&arr[0]);
    elem_diff -= sizeof(arr[0]);
    return elem_diff;
  }
}



const string vertex_shader_source {
  "#version 150\n"
  "in vec4 position;\n"
  "in vec2 texcoord;\n"
  "in vec4 color;\n"
  "smooth out vec4 color_varying;\n"
  "smooth out vec2 tc0;\n"
  "uniform mat4 projection;\n"
  "uniform mat4 modelview;\n"
  "void main()\n"
  "{\n"
  "    gl_Position = projection * modelview * position;\n"
  "    color_varying = color;\n"
  "    tc0 = texcoord;\n"
  "}\n"
};



const string fragment_shader_source {
  "#version 150\n"
  "uniform sampler2D diffuse; // 0\n"
  "smooth in vec4 color_varying;\n"
  "smooth in vec2 tc0;\n"
  "out vec4 out_color;\n"
  "void main() {\n"
  // "    out_color = color_varying * 0.5;\n"
  "    out_color = texture(diffuse, tc0);// * color_varying;\n"
  "}\n"
};



static void client_error_callback(int error, const char *msg)
{
  std::clog << "GLFW Error [" << error << "] " << msg << std::endl;
}



static void client_cleanup()
{
  if (g_gl_queue)
    dispatch_release(g_gl_queue);

  glfwTerminate();
}



static rprogram_t load_program(gl_state_t &gl)
{
  rshader_t vert_shader(gl, GL_VERTEX_SHADER);
  rshader_t frag_shader(gl, GL_FRAGMENT_SHADER);
  rprogram_t program(gl);

  // Bind uniforms to arbitrary names
  program.bind_uniform(UNIFORM_MODELVIEW, "modelview");
  program.bind_uniform(UNIFORM_PROJECTION, "projection");
  program.bind_uniform(UNIFORM_DIFFUSE, "diffuse");
  // Bind attributes to arbitrary names
  program.bind_attrib(ATTR_POSITION, "position");
  program.bind_attrib(ATTR_TEXCOORD, "texcoord");
  program.bind_attrib(ATTR_COLOR, "color");
  // Bind fragment data to color attachment
  program.bind_frag_out(0, "out_color");

  // Attach fragment/vertex shaders
  program.attach_shader(vert_shader);
  program.attach_shader(frag_shader);

  // Compile vertex shader
  vert_shader.load_source(vertex_shader_source);
  if (!vert_shader.compile()) {
    std::clog << vert_shader.error_string() << std::endl;
    throw std::runtime_error("Failed to compile vertex shader");
  }

  // Compile fragment shader
  frag_shader.load_source(fragment_shader_source);
  if (!frag_shader.compile()) {
    std::clog << frag_shader.error_string() << std::endl;
    throw std::runtime_error("Failed to compile fragment shader");
  }

  // Link program
  if (!program.link()) {
    std::clog << program.error_string() << std::endl;
    throw std::runtime_error("Failed to link program");
  }

  // Mark shaders for unloading
  vert_shader.unload();
  frag_shader.unload();

  return std::move(program);
}



static rtexture_t load_texture(gl_state_t &gl, const string &path)
{
  rtexture_t texture(gl, GL_TEXTURE_2D);

  int width = 0, height = 0, components = 0;
  stbi_uc *img_data = NULL;
  dispatch_sync(g_main_queue, [&path, &img_data, &width, &height, &components] {
    int w, h, c;
    w = h = c = 0;
    img_data = stbi_load(path.c_str(), &w, &h, &c, 4);
    width = w;
    height = h;
    components = c;
  });

  texture.bind();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  assert_gl("Setting min filter");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  assert_gl("Setting mag filter");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  assert_gl("Setting U wrap mode");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  assert_gl("Setting V wrap mode");

  texture.tex_image_2d(0, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
  glGenerateMipmap(GL_TEXTURE_2D);

  gl.bind_texture(GL_TEXTURE_2D, 0);

  dispatch_sync(g_main_queue, [&] {
    // stbi_image_free(img_data);
  });

  return texture;
}



dispatch_queue_t cl_get_gl_queue()
{
  return g_gl_queue;
}



client_t &client_t::get_client(int client_num)
{
  if (client_num != DEFAULT_CLIENT_NUM)
    throw std::out_of_range("Invalid client number provided to client_t::get_client");

  return g_client;
}



client_t::client_t() :
frame_queue_(dispatch_queue_create(FRAME_QUEUE_NAME, DISPATCH_QUEUE_CONCURRENT)),
running_(false)
{
}



client_t::~client_t()
{
  dispose();
}



void client_t::dispose()
{
  if (is_connected())
    disconnect();

  if (frame_queue_) {
    dispatch_release(frame_queue_);
    frame_queue_ = nullptr;
  }

  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
}



// must be run on main queue
void client_t::terminate()
{
  dispose();
  client_cleanup();
  sys_quit();
}



void client_t::quit()
{
  running_.store(false);
}



void client_t::run_frameloop()
{
  frameloop();

  // Go back to the main thread and kill the program cleanly.
  dispatch_async(g_main_queue, [this] { terminate(); });
}



static void poll_events(void *d)
{
  (void)d;
  glfwPollEvents();
}


// Run as single thread
void client_t::frameloop()
{
  auto window = window_;
  glfwMakeContextCurrent(window);
  gl_state_t &gl = gl_state();
  rbuffer_t vertex_buffer(gl, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 1024);
  rbuffer_t index_buffer(gl, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 1024);

  pointi_t mpos = {0, 0};
  glfwGetCursorPos(window_, &mpos.x, &mpos.y);

  // Acquire GL state
  gl.acquire();

  rdraw_2d draw(gl);
  rmaterial_basic_t basic_mat0(gl);
  rmaterial_basic_t basic_mat1(gl);
  rprogram_t program = load_program(gl);

  basic_mat0.set_program(&program, UNIFORM_PROJECTION, UNIFORM_MODELVIEW, UNIFORM_DIFFUSE);
  basic_mat1.set_program(&program, UNIFORM_PROJECTION, UNIFORM_MODELVIEW, UNIFORM_DIFFUSE);

  {
    // Set viewport and such
    glfwSwapInterval(0);
    int wm_width, wm_height;
    glfwGetWindowSize(window, &wm_width, &wm_height);
    glClearColor(0.02, 0.02, 0.03, 1.0);
    // glViewport(0, 0, wm_width, wm_height);
    draw.set_screen_size({(uint16_t)wm_width, (uint16_t)wm_height});
  }

  glEnable(GL_BLEND);
  gl.set_blend_func(GL_SRC_ALPHA, GL_ONE);

  running_.store(true);
  last_frame_ = glfwGetTime() - (FRAME_SEQ_TIME * 0.5);

  database_t db("fonts.db", true, SQLITE_OPEN_READONLY);
  rfont_t font(db, "HelveticaNeue");
  db.close();

  rtexture_t tex0 = load_texture(gl, "HVN.png");
  rtexture_t tex1 = load_texture(gl, "Times1.png");
  basic_mat0.set_texture(&tex0);
  basic_mat1.set_texture(&tex1);

  font.set_font_page(0, &basic_mat0);
  font.set_font_page(1, &basic_mat1);
  rvertex_array_t vao_2d(gl);

  while (running_.load()) {

    // Poll events from the main thread. Can do this asynchronously since the
    // handler will prevent simultaneous read/write operations on its event queue.
    // However, we probably want all possible events as needed, so better to call
    // it synchronously.
    dispatch_sync_f(g_main_queue, NULL,  poll_events);

    // Handle events as needed in frame logic thread.
    {
      event_t event;

      while (event_queue_.poll_event(event)) {
        switch (event.kind) {
        case KEY_EVENT:
          if (event.key.button == GLFW_KEY_ESCAPE && event.key.action == GLFW_RELEASE) {
            case WINDOW_CLOSE_EVENT:
            quit();
          }
          break;

        case MOUSE_MOVE_EVENT:
          mpos = event.mouse_pos;
          break;

        default:
          break;
        }
      }
    }

    double cur_time = glfwGetTime();
    glfwMakeContextCurrent(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    assert_gl("Clearing buffers");

    // Draw text
    draw.set_handle(vec2f_t::zero);
    draw.set_rotation(0);
    font.draw_text(draw, {400, 300}, "Foo bar\nGilgamesh", 0.3);

    // Draw rects
    draw.set_handle({ 64, 64 });
    draw.set_rotation(cur_time * 8);
    draw.draw_rect({ 84, 84 }, { 128, 128 }, { 255, 0, 0, 255 }, &basic_mat1);
    draw.set_rotation(cur_time * 9);
    draw.draw_rect({ 800 - 84, 84 }, { 128, 128 }, { 0, 255, 0, 255 }, &basic_mat0);
    draw.set_rotation(cur_time * 10);
    draw.draw_rect({ 84, 600 - 84 }, { 128, 128 }, { 0, 0, 255, 255 }, &basic_mat0);
    draw.set_rotation(cur_time * 11);
    draw.draw_rect({ 800 - 84, 600 - 84 }, { 128, 128 }, { 255, 255, 255, 255 }, &basic_mat0);
    draw.build_vertex_array(vao_2d, ATTR_POSITION, ATTR_TEXCOORD, ATTR_COLOR,
      vertex_buffer, 0, index_buffer, 0);
    draw.draw_with_vertex_array(vao_2d, index_buffer, 0);
    draw.clear();

    glfwSwapBuffers(window);
  } // while (running)

  program.unload();
  index_buffer.unload();
  vertex_buffer.unload();

  glfwMakeContextCurrent(NULL);
}



// must be run on main queue
void client_t::initialize(int argc, const char *argv[])
{
  std::call_once(g_init_flag, [] {
    if (!glfwInit())
      throw std::runtime_error("Failed to initialize GLFW");

    glfwSetErrorCallback(client_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_HIDPI_IF_AVAILABLE, GL_TRUE);

    g_main_queue = dispatch_get_main_queue();
    g_gl_queue = dispatch_queue_create(GL_QUEUE_NAME, DISPATCH_QUEUE_SERIAL);

    std::clog << "---------------- STATIC INIT FINISHED ----------------" << std::endl;
  });

  std::clog << "Initializing window" << std::endl;
  window_ = glfwCreateWindow(800, 600, "Snow", NULL, NULL);
  if (!window_) {
    std::clog << "Window failed to initialize" << std::endl;
    throw std::runtime_error("Failed to create GLFW window");
  } else {
    std::clog << "Window initialized" << std::endl;
  }

  event_queue_.set_window_callbacks(window_, ALL_EVENT_KINDS);
  glfwSetInputMode(window_, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN);

  // Launch frameloop thread
  async_thread(&client_t::run_frameloop, this);
  std::clog << "------------------- INIT FINISHED --------------------" << std::endl;
}



bool client_t::connect(ENetAddress address)
{
  return false;
}



bool client_t::is_connected() const
{
  return false;
}



void client_t::disconnect()
{
}


} // namespace snow
