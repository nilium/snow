#include "entity.hh"
#include "entity_manager.hh"

namespace snow {


void entity_t::finalizer(entity_t *entity)
{
  entity->manager_->destroy_entity(entity->index());
}



entity_t::entity_t(entity_manager_t *manager, int index)
: manager_(manager), index_(index), parent_(NO_ENTITY)
{
}



entity_t::~entity_t()
{
  auto iter = children_.begin();
  auto term = children_.end();
  while (iter != term) {
    int child_index = *iter;
    entity_t &e = manager_->get_entity(child_index);
    e.remove_from_parent();
    ++iter;
  }
}



/*******************************************************************************
*                            transform_t overrides                             *
*******************************************************************************/

void entity_t::set_translation(const vec3_t &t)
{
  invalidate_cache();
  transform_t::set_translation(t);
}



void entity_t::set_rotation(const mat3_t &r)
{
  invalidate_cache();
  transform_t::set_rotation(r);
}



void entity_t::set_scale(const vec3_t &s)
{
  invalidate_cache();
  transform_t::set_scale(s);
}



auto entity_t::to_matrix() const -> mat4_t
{
  if (tform_valid_) {
    tform_cache_ = transform_t::to_matrix();
    tform_valid_ = true;
  }
  return tform_cache_;
}



/*******************************************************************************
*                              transform caching                               *
*******************************************************************************/

auto entity_t::to_world() const -> mat4_t
{
  if (world_valid_) {
    world_cache_ = to_matrix();
    int pred = parent_;
    while (pred != NO_ENTITY) {
      entity_t &entity = manager_->get_entity(pred);
      world_cache_ = entity.to_matrix() * world_cache_;
      pred = entity.parent();
    }
    world_valid_ = true;
  }
  return world_cache_;
}



void entity_t::invalidate_cache() const
{
  if (!(tform_valid_ || world_valid_))
    return;

  tform_valid_ = false;
  world_valid_ = false;
  if (!children_.empty()) {
    for (auto child_index : children_) {
      manager_->get_entity(child_index).invalidate_cache();
    }
  }
}



/*******************************************************************************
*                               entity hierarchy                               *
*******************************************************************************/

auto entity_t::parent() const -> int
{
  return parent_;
}



auto entity_t::children() const -> list_t
{
  return children_;
}



void entity_t::add_child(entity_t *entity)
{
  if (entity->manager_ != manager_) {
    throw std::invalid_argument("Entity belongs to a different entity manager");
  } else if (entity->parent()) {
    throw std::runtime_error("Entity already has a parent");
  }

  entity->child_link_ = children_.insert(children_.end(), entity->index());
  entity->parent_ = index();
  entity->retain();
}



void entity_t::remove_from_parent()
{
  if (!parent())
    throw std::runtime_error("Entity does not have a parent");

  manager_->get_entity(parent_).children_.erase(child_link_);
  parent_ = NO_ENTITY;
  release();
}



/*******************************************************************************
*                        Manager convenience functions                         *
*******************************************************************************/

entity_t &entity_t::retain()
{
  manager_->retain_entity(this);
  return *this;
}



void entity_t::release()
{
  manager_->release_entity(this);
}


} // namespace snow
