#ifndef __SNOW__CONSOLE_PANE_HH__
#define __SNOW__CONSOLE_PANE_HH__

#include "../config.hh"
#include "system.hh"
#include <deque>


namespace snow {


struct cvar_set_t;
struct cvar_t;
struct rdraw_2d_t;
struct rfont_t;
struct rmaterial_t;


struct console_pane_t : public system_t
{
  virtual bool event(const event_t &event);
  virtual void frame(double step, double timeslice);
  virtual void draw(double timeslice);

  void set_background(rmaterial_t *bg_mat);
  rmaterial_t *background() const;

  void set_font(rfont_t *font);
  rfont_t *font() const;

  void set_drawer(rdraw_2d_t *drawer);
  rdraw_2d_t *drawer() const;

  void set_cvar_set(cvar_set_t *cvars);
  cvar_set_t *cvar_set() const;

  void write_log(const string &message);

private:
  unsigned            top_      = 0;
  unsigned            log_max_  = 100;
  float               font_scale_ = 1.0f; // set by set_font
  bool                open_     = false;
  rmaterial_t *       bg_mat_   = nullptr;
  rdraw_2d_t *        drawer_   = nullptr;
  rfont_t *           font_     = nullptr;
  cvar_set_t *        cvars_    = nullptr;
  cvar_t *            wnd_mouseMode = nullptr;
  string              buffer_;
  std::deque<string>  log_;
};


console_pane_t &default_console();


} // namespace snow

#endif /* end __SNOW__CONSOLE_PANE_HH__ include guard */
