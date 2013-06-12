/*
  player_mover.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "player_mover.hh"
#include "transform.hh"


namespace snow {


DEFINE_COMPONENT_CTOR_DTOR(player_mover_t);



void player_mover_t::move(const vec2f_t &velocity)
{
  get_component<transform_t>()->translate(velocity);
}


} // namespace snow
