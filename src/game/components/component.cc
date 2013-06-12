/*
  component.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "component.hh"


namespace snow {


component_base_t::component_base_t()
{
  /* nop */
}



component_base_t::component_base_t(game_object_t *obj) :
  game_object(obj)
{
  /* nop */
}



component_base_t::~component_base_t()
{
  /* nop */
}


} // namespace snow
