/*
  player_mover.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__PLAYER_MOVER_HH__
#define __SNOW__PLAYER_MOVER_HH__

#include "component.hh"


namespace snow {


struct player_mover_t : component_t<player_mover_t, PLAYER_COMPONENT>
{

  DECL_COMPONENT_CTOR_DTOR(player_mover_t);

  void move(const vec2f_t &velocity);

};


} // namespace snow

#endif /* end __SNOW__PLAYER_MOVER_HH__ include guard */
