#ifndef __SNOW__PLAYER_HH__
#define __SNOW__PLAYER_HH__

#include "../../config.hh"
#include "../system.hh"
#include <snow/math/vec2.hh>
#include "../../renderer/buffer.hh"
#include "../../renderer/draw_2d.hh"
#include "../../renderer/vertex_array.hh"


namespace snow {


struct game_object_t;
struct player_mover_t;


struct player_t : system_t
{

  player_t();
  virtual ~player_t();

  virtual bool event(const event_t &event);
  virtual void frame(double step, double timeslice);
  virtual void draw(double timeslice);

  void set_player(game_object_t *player);

private:
  vec2f_t mouse_pos_ = { 0, 0 };
  vec2f_t window_size_ = { 800, 600 };
  vec2_t<int> move_direction_ = { 0, 0 };
  game_object_t *player_ = nullptr;
  rmaterial_t *player_mat_ = nullptr;
  rdraw_2d_t drawer_;
  rbuffer_t vbuffer_;
  rbuffer_t ibuffer_;
  bool init_vao_ = false;
  rvertex_array_t vao_;
};


} // namespace snow

#endif /* end __SNOW__PLAYER_HH__ include guard */
