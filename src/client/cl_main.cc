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

#include <snow-common.hh>
#include <snow/math/math3d.hh>

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
  UNIFORM_PROJECTION
};

enum shader_attr_t : GLuint
{
  ATTR_POSITION,
  ATTR_NORMAL,
  ATTR_COLOR
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

std::array<vec4f_t, 4> point_data {{
  { -1.0f,  1.0f, -1.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f }
}};

std::array<vec3f_t, 4> normal_data {{
  { 0.0f, 0.0f, -1.0f },
  { 0.0f, 0.0f, -1.0f },
  { 0.0f, 0.0f, -1.0f },
  { 0.0f, 0.0f, -1.0f }
}};

std::array<vec4f_t, 4> color_data {{
  { 1, 0, 0, 1 },
  { 0, 1, 0, 1 },
  { 0, 0, 1, 1 },
  { 1, 0, 1, 1 }
}};

std::array<vec3_t<uint32_t>, 2> face_data {{
  { 0, 1, 2 },
  { 0, 3, 2 }
}};

const string vertex_shader_source {
  "#version 150\n"
  "in vec4 position;\n"
  "in vec3 normal;\n"
  "in vec4 color;\n"
  "smooth out vec4 color_varying;\n"
  "uniform mat4 projection;\n"
  "uniform mat4 modelview;\n"
  "void main()\n"
  "{\n"
  "    gl_Position = projection * modelview * position;\n"
  "//    vec3 normalColor = (normal + vec3(1.0)) * 0.5;\n"
  "    color_varying = color;\n"
  "}\n"
};

const string fragment_shader_source {
  "#version 150\n"
  "smooth in vec4 color_varying;\n"
  "out vec4 out_color;\n"
  "void main() {\n"
  "    out_color = color_varying;\n"
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

// Run as single thread
void client_t::run_frameloop()
{
  using namespace snow::renderer;
  auto window = window_;
  gl_state_t &gl = glstate_;
  vertex_array_t vao(gl);
  rbuffer_t vertex_buffer(gl, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 512);
  rbuffer_t index_buffer(gl, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 128);

  pointi_t mpos = {0, 0};
  glfwGetCursorPos(window_, &mpos.x, &mpos.y);

  glfwMakeContextCurrent(window);

  rshader_t vert_shader(glstate_, GL_VERTEX_SHADER);
  rshader_t frag_shader(glstate_, GL_FRAGMENT_SHADER);
  rprogram_t program(glstate_);

  // Bind uniforms to arbitrary names
  program.bind_uniform(UNIFORM_MODELVIEW, "modelview");
  program.bind_uniform(UNIFORM_PROJECTION, "projection");
  // Bind attributes to arbitrary names
  program.bind_attrib(ATTR_POSITION, "position");
  program.bind_attrib(ATTR_NORMAL, "normal");
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

  dispatch_sync(g_gl_queue, [&vertex_buffer, &gl, &program, &index_buffer, &vao, window] {
    glfwMakeContextCurrent(window);

    // Acquire GL state
    gl.acquire();

    // Log system info
    std::clog << "GL Version: " << gl.version_string() << std::endl;
    std::clog << "GLSL Version: " << gl.glsl_version_string() << std::endl;
    std::clog << "Vendor: " << gl.vendor() << std::endl;
    std::clog << "Renderer: " << gl.renderer() << std::endl;

    // Write out available extensions:
    std::clog << "-------------------- GL EXTENSIONS -------------------" << std::endl;
    for (auto ext : gl.extensions())
      std::clog << ext << ' ' << std::endl;
    std::clog << "------------------ END GL EXTENSIONS -----------------" << std::endl;

    const GLsizeiptr pos_size = sizeof(vec4f_t) * point_data.size();
    const GLsizeiptr norm_size = sizeof(vec3f_t) * normal_data.size();
    const GLsizeiptr color_size = sizeof(vec4f_t) * color_data.size();

    // Build buffers
    {
      // Build vertex buffer
      vertex_buffer.bind();

      glBufferSubData(GL_ARRAY_BUFFER, 0, pos_size, point_data.data());
      assert_gl("Buffering point data");

      glBufferSubData(GL_ARRAY_BUFFER, pos_size, norm_size, normal_data.data());
      assert_gl("Buffering normal data");

      glBufferSubData(GL_ARRAY_BUFFER, pos_size + norm_size, color_size, color_data.data());
      assert_gl("Buffering color data");

      // Build index buffer
      index_buffer.bind();
      const GLsizeiptr index_size = face_data.size() * sizeof(decltype(face_data)::value_type);
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_size, face_data.data());
      assert_gl("Buffering index data");
    }

    vao.set_initializer([&vertex_buffer, &index_buffer,
                         pos_size, norm_size, color_size]
                        (gl_state_t &gl) -> bool {
      vertex_buffer.bind();
      index_buffer.bind();

      gl.set_attrib_array_enabled(ATTR_POSITION, true);
      gl.set_attrib_array_enabled(ATTR_NORMAL, true);
      gl.set_attrib_array_enabled(ATTR_COLOR, true);

      glVertexAttribPointer(ATTR_POSITION, 4, GL_FLOAT, GL_FALSE, stride_of(point_data),
                            (const GLvoid *)0);
      assert_gl("Binding position attribute data");

      glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, stride_of(normal_data),
                            (const GLvoid *)pos_size);
      assert_gl("Binding normal attribute data");

      glVertexAttribPointer(ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, stride_of(color_data),
                            (const GLvoid *)(pos_size + norm_size));
      assert_gl("Binding color attribute data");

      return true;
    });
  });

  dispatch_async(g_gl_queue, [window] {
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    int wm_width, wm_height;
    glfwGetWindowSize(window, &wm_width, &wm_height);
    glClearColor(0, 0, 0, 1.0);
    glViewport(0, 0, wm_width, wm_height);

    glfwMakeContextCurrent(NULL);
  });

  running_.store(true);
  last_frame_ = glfwGetTime() - (FRAME_SEQ_TIME * 0.5);

  while (running_.load()) {

    // Poll events from the main thread. Can do this asynchronously since the
    // handler will prevent simultaneous read/write operations on its event queue.
    // However, we probably want all possible events as needed, so better to call
    // it synchronously.
    dispatch_async(g_main_queue, [] { glfwPollEvents(); });

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

    dispatch_async(g_gl_queue, [&gl, &program, &vao, window] {
      glfwMakeContextCurrent(window);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      assert_gl("Clearing buffers");

      program.use();

      glUniformMatrix4fv(program.uniform_location(UNIFORM_MODELVIEW), 1, GL_FALSE,
                         mat4f_t::identity);
      assert_gl("Setting modelview");
      glUniformMatrix4fv(program.uniform_location(UNIFORM_PROJECTION), 1, GL_FALSE,
                         mat4f_t::orthographic(-2, 2, 2, -2, -10, 10));
      assert_gl("Setting projection");

      // Draw square
      vao.bind();
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      assert_gl("Drawing faces");
    });

    // Must be sync to flush the GL queue
    dispatch_sync(g_gl_queue, [window] {
      glfwSwapBuffers(window);
    });

  } // while (running)

  vao.unload();
  program.unload();
  index_buffer.unload();
  vertex_buffer.unload();

  glfwMakeContextCurrent(window);

  // Go back to the main thread and kill the program cleanly.
  dispatch_async(g_main_queue, [this] { terminate(); });
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
