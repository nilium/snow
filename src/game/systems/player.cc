#include "player.hh"
#include "../components/player_mover.hh"
#include "../components/transform.hh"
#include "../../renderer/constants.hh"
#include "../../renderer/material.hh"
#include "../../event.hh"
#include "../resources.hh"
#include <snow/math/math.hh>

namespace snow {


player_t::player_t() :
  drawer_(),
  vbuffer_(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 128),
  ibuffer_(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 36)
{
}



player_t::~player_t()
{
  if (player_mat_) {
    resources_t::default_resources().release_material(player_mat_);
  }
}



bool player_t::event(const event_t &event)
{
  if (!player_) {
    return true;
  }

  switch (event.kind) {
  case MOUSE_MOVE_EVENT: {
    // Cursor facing
    int x = 0;
    int y = 0;
    glfwGetWindowSize(event.window, &x, &y);
    mouse_pos_.x = event.mouse_pos.x;
    mouse_pos_.y = static_cast<float>(y) - event.mouse_pos.y;
  } return true; // MOUSE_MOVE_EVENT

  case WINDOW_SIZE_EVENT: {
    window_size_ = event.window_size;
  } return false; // WINDOW_SIZE_EVENT

  default: return true;
  }
}



void player_t::frame(double step, double timeslice)
{
  if (!player_) {
    // If no player is set, just forcibly grab the last object that has a
    // player_mover component.
    player_mover_t::apply_fn([&] (player_mover_t &mover) { player_ = mover.game_object; });
    if (!player_) {
      s_log_note("No player object found");
      return;
    }
  }

  GLFWwindow *window = main_window();
  move_direction_ = vec2f_t::zero;
  if (glfwGetKey(window, GLFW_KEY_W)) move_direction_.y += 1;
  if (glfwGetKey(window, GLFW_KEY_S)) move_direction_.y -= 1;
  if (glfwGetKey(window, GLFW_KEY_A)) move_direction_.x -= 1;
  if (glfwGetKey(window, GLFW_KEY_D)) move_direction_.x += 1;

  vec2f_t delta = {
    static_cast<float>(move_direction_.x),
    static_cast<float>(move_direction_.y)
  };
  delta.normalize().scale(4.0f);

  player_->get_component<player_mover_t>()->move(delta);
}



void player_t::draw(double timeslice)
{
  if (!player_) {
    return;
  }

  if (!player_mat_) {
    player_mat_ = resources_t::default_resources().load_material("actors/player");
    assert(player_mat_);
    if (!player_mat_) {
      return;
    }
  }

  rmaterial_t::set_modelview(mat4f_t::identity);
  // truncate Z
  const vec2f_t pos = player_->get_component<transform_t>()->translation();

  drawer_.clear();
  drawer_.set_rotation(atan2(pos.y - mouse_pos_.y, mouse_pos_.x - pos.x) * S_RAD2DEG + 90);
  drawer_.set_handle({0.5, 0.5});
  drawer_.set_origin(pos);
  drawer_.draw_rect(vec2f_t::zero, {32, 32}, {1.0, 1.0, 1.0, 1.0}, player_mat_);
  drawer_.buffer_vertices(vbuffer_, 0);
  drawer_.buffer_indices(ibuffer_, 0);

  if (!init_vao_) {
    vao_ = drawer_.build_vertex_array(ATTRIB_POSITION, ATTRIB_TEXCOORD0, ATTRIB_COLOR, vbuffer_, 0, ibuffer_);
    init_vao_ = true;
  }

  drawer_.draw_with_vertex_array(vao_, 0);
}


} // namespace snow
