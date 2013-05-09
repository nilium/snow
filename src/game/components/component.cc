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
