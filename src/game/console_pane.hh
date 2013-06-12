/*
  console_pane.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__CONSOLE_PANE_HH__
#define __SNOW__CONSOLE_PANE_HH__

#include "../config.hh"
#include "system.hh"
#include <deque>
#include "../renderer/buffer.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/vertex_array.hh"


namespace snow {


struct cvar_set_t;
struct cvar_t;
struct rfont_t;
struct rmaterial_t;


struct console_pane_t : public system_t
{
  virtual bool event(const event_t &event);
  virtual void frame(double step, double timeslice);
  virtual void draw(double timeslice);

  void set_cvar_set(cvar_set_t *cvars);
  cvar_set_t *cvar_set() const;

  void write_log(const string &message);

private:
  rdraw_2d_t          drawer_;
  string              buffer_;
  rbuffer_t           vbuffer_      { rbuffer_t(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 16384) };
  rbuffer_t           ibuffer_      { rbuffer_t(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 16384) };
  std::deque<string>  log_;
  rvertex_array_t     vao_;
  rmaterial_t *       bg_mat_       { nullptr };
  rfont_t *           font_         { nullptr };
  cvar_set_t *        cvars_        { nullptr };
  cvar_t *            wnd_mouseMode { nullptr };
  unsigned            top_          { 0 };
  unsigned            log_max_      { 100 };
  float               font_scale_   { 1.0f }; // set by set_font
  bool                open_         { false };
};


console_pane_t &default_console();


} // namespace snow

#endif /* end __SNOW__CONSOLE_PANE_HH__ include guard */
