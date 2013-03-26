#include "entity_manager.hh"

namespace snow {


entity_t &entity_manager_t::make_entity()
{
  int index = -1;

  index = entities_.reserve(this, -1);
  entity_t &entity = entities_[index];
  entity.index_ = index;
  return entity;
}



entity_t &entity_manager_t::get_entity(int index)
{
  return entities_[index];
}



auto entity_manager_t::active_entities() const -> list_t
{
  list_t actives;
  entities_.each_object_const([&actives](const entity_t &entity, const int &index) {
    actives.push_back(index);
  });
  return actives;
}



void entity_manager_t::destroy_entity(int index)
{
  entities_.collect(index);
}



void entity_manager_t::retain_entity(const entity_t *entity)
{
  counter_.retain(entity);
}



void entity_manager_t::release_entity(entity_t *entity)
{
  counter_.release(entity, entity_t::finalizer);
}


} // namespace snow
