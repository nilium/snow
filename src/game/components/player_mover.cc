#include "player_mover.hh"
#include "transform.hh"


namespace snow {


DEFINE_COMPONENT_CTOR_DTOR(player_mover_t);



void player_mover_t::move(const vec2f_t &velocity)
{
  get_component<transform_t>()->translate(velocity);
}


} // namespace snow
