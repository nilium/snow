/*
  console_pane.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "console_pane.hh"
#include "../renderer/material.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/font.hh"
#include "../console.hh"
#include "../event_queue.hh"
#include "resources.hh"


#define VERTEX_OFFSET   (0)
#define INDEX_OFFSET    (0)
#define CONSOLE_HEIGHT (300)
#define CONSOLE_SPEED (30)


namespace snow {


bool console_pane_t::event(const event_t &event)
{
  bool propagate = true;
  switch (event.kind) {
  case KEY_EVENT:
    switch (event.key.button) {
    case GLFW_KEY_BACKSPACE:
      if (event.key.action == GLFW_RELEASE) {
        break;
      }

      if (!buffer_.empty()) {
        auto end = buffer_.cend();
        auto iter = utf8::before(end);
        if (iter == buffer_.cbegin()) {
          buffer_.clear();
        } else {
          buffer_.erase(iter, end);
        }
      }

      propagate = false;
      break;

    case GLFW_KEY_GRAVE_ACCENT:
      if (event.key.action == GLFW_PRESS && event.key.mods == GLFW_MOD_SHIFT) {
        open_ = !open_;
        #if HIDE_CURSOR_ON_CONSOLE_CLOSE
        if (wnd_mouseMode) {
          wnd_mouseMode->seti(open_);
        }
        #endif
        propagate = false;
      }
      break;

    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
      if (!open_) {
        break;
      }

      propagate = false;

      if (!event.key.action) {
        break;
      } else if (cvars_) {
        cvars_->execute(buffer_);
      }

      buffer_.clear();
      break;
    default: propagate = !open_; break;
    }
    break;

  case CHAR_EVENT:
    if (!open_) {
      break;
    }

    if (event.character >= ' ' && event.character < '~') {
      // FIXME: handle non-ASCII characters
      utf8::put_code(std::back_inserter(buffer_), event.character);
      propagate = false;
    }
    break;
    default: break;
  }
  return propagate;
}



void console_pane_t::frame(double step, double timeslice)
{
  if (open_ && top_ != CONSOLE_HEIGHT) {
    if (top_ < CONSOLE_HEIGHT) {
      top_ += CONSOLE_SPEED;
    }

    if (top_ > CONSOLE_HEIGHT) {
      top_ = CONSOLE_HEIGHT;
    }
  } else if (!open_ && top_) {
    top_ -= CONSOLE_SPEED;

    if (top_ == 1) {
      top_ = 0;
    }
  }
}



void console_pane_t::draw(double timeslice)
{
  resources_t &res = resources_t::default_resources();

  if (!bg_mat_) {
    bg_mat_ = res.load_material("console/background");
  }

  if (!font_) {
    font_ = res.load_font("console");
    if (font_) {
      font_scale_ = 20.0f / font_->line_height();
    }
  }

  drawer_.clear();

  const float alpha = float(top_) / float(CONSOLE_HEIGHT);
  const vec4f_t tint = { 1.0, 1.0, 1.0, alpha };
  const vec2f_t screen_size = drawer_.offset_to_screen({ 1, 1 });
  vec2f_t size = screen_size;
  vec2f_t pos = { 0, size.y - top_ };

  if (bg_mat_ != nullptr && top_) {
    size.y = CONSOLE_HEIGHT;
    drawer_.draw_rect_raw(pos, size, tint, bg_mat_);
  }

  if (font_ != nullptr && top_ != 0) {
    const float line_height = std::ceil(font_->line_height() * font_scale_);
    vec2f_t buffer_pos = { 4, pos.y + std::ceil(font_->descent() * font_scale_) + 4};
    font_->draw_text(drawer_, buffer_pos, buffer_, tint, true, font_scale_);
    buffer_pos.y += line_height + 10;
    for (const string &log_message : log_) {
      font_->draw_text(drawer_, buffer_pos, log_message, tint, true, font_scale_);
      buffer_pos.y += line_height;
      if (buffer_pos.y > screen_size.y) {
        break;
      }
    }
  }

  drawer_.buffer_vertices(vbuffer_, VERTEX_OFFSET);
  drawer_.buffer_indices(ibuffer_, INDEX_OFFSET);

  if (!vao_.generated()) {
    vao_ = drawer_.build_vertex_array(ATTRIB_POSITION, ATTRIB_TEXCOORD0, ATTRIB_COLOR, vbuffer_, VERTEX_OFFSET, ibuffer_);
  }

  drawer_.draw_with_vertex_array(vao_, INDEX_OFFSET);
}



void console_pane_t::set_cvar_set(cvar_set_t *cvars)
{
  cvars_ = cvars;
  #if HIDE_CURSOR_ON_CONSOLE_CLOSE
  if (cvars_) {
    wnd_mouseMode = cvars_->get_cvar("wnd_mouseMode", true, CVAR_DELAYED | CVAR_INVISIBLE);
  } else {
    wnd_mouseMode = nullptr;
  }
  #endif
}



cvar_set_t *console_pane_t::cvar_set() const
{
  return cvars_;
}



void console_pane_t::write_log(const string &message)
{
  if (log_.size() == log_max_) {
    log_.pop_back();
  }
  log_.push_front(message);
}



namespace {


console_pane_t g_default_console_pane;


} // namespace <anon>


console_pane_t &default_console()
{
  return g_default_console_pane;
}


} // namespace snow
