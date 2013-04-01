#include "event_queue.hh"
#include <atomic>
#include <map>

namespace snow {


namespace {


/*******************************************************************************
*                          Event callback prototypes                           *
*******************************************************************************/

void ecb_key_event(GLFWwindow *window, int key, int action);
void ecb_mouse_event(GLFWwindow *window, int button, int action);
void ecb_char_event(GLFWwindow *window, int character);
void ecb_mouse_pos_event(GLFWwindow *window, int x, int y);
void ecb_mouse_scroll_event(GLFWwindow *window, double x, double y);
void ecb_mouse_enter_event(GLFWwindow *window, int entered);
int  ecb_window_close_event(GLFWwindow *window);
void ecb_window_move_event(GLFWwindow *window, int x, int y);
void ecb_window_size_event(GLFWwindow *window, int width, int height);
void ecb_window_focus_event(GLFWwindow *window, int focused);
void ecb_window_iconify_event(GLFWwindow *window, int iconified);




/*******************************************************************************
*                                Event mappings                                *
*******************************************************************************/

using event_name_map_t = std::map<event_kind_t, string>;
const event_name_map_t g_event_names {
  std::make_pair(NULL_EVENT,           STT("NULL_EVENT")),
  std::make_pair(KEY_EVENT,            STT("KEY_EVENT")),
  std::make_pair(CHAR_EVENT,           STT("CHAR_EVENT")),
  std::make_pair(MOUSE_EVENT,          STT("MOUSE_EVENT")),
  std::make_pair(MOUSE_MOVE_EVENT,     STT("MOUSE_MOVE_EVENT")),
  std::make_pair(MOUSE_SCROLL_EVENT,   STT("MOUSE_SCROLL_EVENT")),
  std::make_pair(MOUSE_ENTER_EVENT,    STT("MOUSE_ENTER_EVENT")),
  std::make_pair(WINDOW_CLOSE_EVENT,   STT("WINDOW_CLOSE_EVENT")),
  std::make_pair(WINDOW_FOCUS_EVENT,   STT("WINDOW_FOCUS_EVENT")),
  std::make_pair(WINDOW_ICONIFY_EVENT, STT("WINDOW_ICONIFY_EVENT")),
  std::make_pair(WINDOW_SIZE_EVENT,    STT("WINDOW_SIZE_EVENT")),
  std::make_pair(WINDOW_MOVE_EVENT,    STT("WINDOW_MOVE_EVENT")),
  std::make_pair(OPAQUE_EVENT,         STT("OPAQUE_EVENT")),
};



/*******************************************************************************
*                        Event callback implementations                        *
*******************************************************************************/

void ecb_key_event(GLFWwindow *window, int key, int action)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, KEY_EVENT, glfwGetTime()};
    event.key = {key, action};
    queue->emit_event(event);
  }
}



void ecb_mouse_event(GLFWwindow *window, int button, int action)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_EVENT, glfwGetTime()};
    event.mouse = {button, action};
    queue->emit_event(event);
  }
}



void ecb_char_event(GLFWwindow *window, int character)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, CHAR_EVENT, glfwGetTime()};
    event.character = character;
    queue->emit_event(event);
  }
}



void ecb_mouse_pos_event(GLFWwindow *window, int x, int y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_MOVE_EVENT, glfwGetTime()};
    event.mouse_pos = {x, y};
    queue->emit_event(event);
  }
}



void ecb_mouse_scroll_event(GLFWwindow *window, double x, double y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_SCROLL_EVENT, glfwGetTime()};
    event.scroll = {x, y};
    queue->emit_event(event);
  }
}



void ecb_mouse_enter_event(GLFWwindow *window, int entered)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_ENTER_EVENT, glfwGetTime()};
    event.entered = (entered == GL_TRUE);
    queue->emit_event(event);
  }
}


int ecb_window_close_event(GLFWwindow *window)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_CLOSE_EVENT, glfwGetTime()};
    queue->emit_event(event);
  }
  return GL_FALSE;
}



void ecb_window_move_event(GLFWwindow *window, int x, int y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_MOVE_EVENT, glfwGetTime()};
    event.window_pos = {x, y};
    queue->emit_event(event);
  }
}



void ecb_window_size_event(GLFWwindow *window, int width, int height)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_SIZE_EVENT, glfwGetTime()};
    event.window_size = {width, height};
    queue->emit_event(event);
  }
}



void ecb_window_focus_event(GLFWwindow *window, int focused)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_FOCUS_EVENT, glfwGetTime()};
    event.focused = (focused == GL_TRUE);
    queue->emit_event(event);
  }
}



void ecb_window_iconify_event(GLFWwindow *window, int iconified)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_ICONIFY_EVENT, glfwGetTime()};
    event.iconified = (iconified == GL_TRUE);
    queue->emit_event(event);
  }
}


} // namespace <anon>


/*******************************************************************************
*                           Event string conversion                            *
*******************************************************************************/

const string &event_kind_string(event_kind_t kind)
{
  event_name_map_t::const_iterator iter = g_event_names.find(kind);
  if (iter == g_event_names.cend())
    throw std::out_of_range("Invalid event kind");
  return iter->second;
}



/*******************************************************************************
*                         event_queue_t implementation                         *
*******************************************************************************/

event_queue_t::event_queue_t() :
  queue_(dispatch_queue_create("net.spifftastic.snow.event_queue", DISPATCH_QUEUE_CONCURRENT))
{
}



event_queue_t::~event_queue_t()
{
  dispatch_release(queue_);
}



bool event_queue_t::wait_event(event_t &out, dispatch_time_t timeout)
{
  bool set = false;
  auto assign_out = [&]() {
      if ((set = !events_.empty())) {
        out = events_.front();
        events_.pop_front();
      }
    };

  if (timeout != DISPATCH_TIME_FOREVER) {
    while (!set && dispatch_time(DISPATCH_TIME_NOW, 0) < timeout)
      dispatch_barrier_sync(queue_, assign_out);
  } else {
    while (!set)
      dispatch_barrier_sync(queue_, assign_out);
  }
  return set;
}



bool event_queue_t::peek_event(event_t &out) const
{
  bool set = false;
  dispatch_sync(queue_, [&]() {
    if (!events_.empty()) {
      out = events_.front();
      set = true;
    }
  });
  return set;
}



bool event_queue_t::poll_event(event_t &out)
{
  bool set = false;
  dispatch_barrier_sync(queue_, [&]() {
    if (!events_.empty()) {
      out = events_.front();
      events_.pop_front();
      set = true;
    }
  });
  return set;
}



bool event_queue_t::poll_event_before(event_t &out, double time)
{
  bool set = false;
  dispatch_barrier_sync(queue_, [&]() {
    if (!events_.empty()) {
      const event_t &front = events_.front();
      if (front.time < time) {
        out = events_.front();
        events_.pop_front();
        set = true;
      }
    }
  });
  return set;
}



void event_queue_t::emit_event(const event_t &event)
{
  const event_t copy = event;
  dispatch_barrier_async(queue_, [&, copy]() {
    events_.push_back(copy);
  });
}



auto event_queue_t::event_queue() const -> event_list_t
{
  event_list_t copy;
  dispatch_barrier_sync(queue_, [&]() { copy = events_; });
  return copy;
}



void event_queue_t::set_window_callbacks(GLFWwindow *window, int events_mask)
{
  #define flag_check(FLAGS, FLAG) (((FLAGS)&(FLAG))==(FLAG))

  glfwSetWindowUserPointer(window, (events_mask ? (void *)this : NULL));

  glfwSetKeyCallback(window,
    (flag_check(events_mask, KEY_EVENT)
     ? ecb_key_event
     : NULL));

  glfwSetCharCallback(window,
    (flag_check(events_mask, CHAR_EVENT)
     ? ecb_char_event
     : NULL));

  glfwSetMouseButtonCallback(window,
    (flag_check(events_mask, MOUSE_EVENT)
     ? ecb_mouse_event
     : NULL));

  glfwSetCursorPosCallback(window,
    (flag_check(events_mask, MOUSE_MOVE_EVENT)
     ? ecb_mouse_pos_event
     : NULL));

  glfwSetScrollCallback(window,
    (flag_check(events_mask, MOUSE_SCROLL_EVENT)
     ? ecb_mouse_scroll_event
     : NULL));

  glfwSetCursorEnterCallback(window,
    (flag_check(events_mask, MOUSE_ENTER_EVENT)
     ? ecb_mouse_enter_event
     : NULL));

  glfwSetWindowCloseCallback(window,
    (flag_check(events_mask, WINDOW_CLOSE_EVENT)
     ? ecb_window_close_event
     : NULL));

  glfwSetWindowPosCallback(window,
    (flag_check(events_mask, WINDOW_MOVE_EVENT)
     ? ecb_window_move_event
     : NULL));

  glfwSetWindowSizeCallback(window,
    (flag_check(events_mask, WINDOW_SIZE_EVENT)
     ? ecb_window_size_event
     : NULL));

  glfwSetWindowFocusCallback(window,
    (flag_check(events_mask, WINDOW_FOCUS_EVENT)
     ? ecb_window_focus_event
     : NULL));

  glfwSetWindowIconifyCallback(window,
    (flag_check(events_mask, WINDOW_ICONIFY_EVENT)
     ? ecb_window_iconify_event
     : NULL));

  #undef flag_check
}


} // namespace snow
