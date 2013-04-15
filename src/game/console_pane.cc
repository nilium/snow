#include "console_pane.hh"
#include "../renderer/material.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/font.hh"
#include "../console.hh"
#include "../ext/utf8.h"
#include "../event_queue.hh"


namespace u8 = utf8::unchecked;


namespace snow {


bool console_pane_t::event(const event_t &event)
{
  bool propagate = true;
  switch (event.kind) {
  case KEY_EVENT:
    if (event.key.button == GLFW_KEY_WORLD_1 && event.key.action == 0) {
      open_ = !open_;
      propagate = false;
    }
    break;
  case CHAR_EVENT:
    if (event.character == '`') {
      propagate = false;
    } else if (open_) {
      if (event.character == '\b') {
        if (!buffer_.empty()) {
          auto end = buffer_.cend();
          auto iter = buffer_.cbegin();
          auto codes = u8::distance(iter, end);
          if (codes > 1) {
            u8::advance(iter, codes - 1);
            buffer_.erase(iter, end);
          } else {
            buffer_.clear();
          }
        }

        propagate = false;
      } else if (event.character >= ' ') {
        u8::append(event.character, std::back_inserter(buffer_));
        propagate = false;
      }
    }
    break;
    default: break;
  }
  return propagate;
}



void console_pane_t::frame(double step, double timeslice)
{
  if (open_ && top_ != 100) {
    if (top_ < 100) {
      top_ += 10;
    }

    if (top_ > 100) {
      top_ = 100;
    }
  } else if (!open_ && top_) {
    top_ -= 10;

    if (top_ == 1) {
      top_ = 0;
    }
  }
}



void console_pane_t::draw(double timeslice)
{
  if (drawer_ == nullptr) {
    return;
  }

  const uint8_t alpha = uint8_t((top_ * 255) / 100);
  const vec4_t<uint8_t> tint = { 255, 255, 255, alpha };
  vec2f_t size = drawer_->offset_to_screen({ 1, 1 });
  vec2f_t pos = { 0, size.y - top_ };

  if (bg_mat_ != nullptr && top_) {
    size.y = 100;
    drawer_->draw_rect_raw(pos, size, tint, bg_mat_);
  }

  if (font_ != nullptr && top_ != 0) {
    const vec2f_t buffer_pos = { 4, pos.y + font_->descent() * 0.5f + 4 };
    font_->draw_text(*drawer_, buffer_pos, buffer_, tint, 0.5f);
  }

}


void console_pane_t::set_background(rmaterial_t *bg_mat)
{
  bg_mat_ = bg_mat;
}



rmaterial_t *console_pane_t::background() const
{
  return bg_mat_;
}



void console_pane_t::set_font(rfont_t *font)
{
  font_ = font;
}



rfont_t *console_pane_t::font() const
{
  return font_;
}



void console_pane_t::set_drawer(rdraw_2d_t *drawer)
{
  drawer_ = drawer;
}



rdraw_2d_t *console_pane_t::drawer() const
{
  return drawer_;
}



void console_pane_t::set_cvar_set(cvar_set_t *cvars)
{
  cvars_ = cvars;
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



console_pane_t &default_console()
{
  static console_pane_t pane;
  return pane;
}


} // namespace snow
