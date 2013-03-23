#include "entity.hh"

namespace snow {
namespace game {

entity_t::~entity_t()
{
  if (parent_)
    remove_from_parent();

  while (!children_.empty())
    delete children_.back();
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
*                                  frame loop                                  *
*******************************************************************************/

void entity_t::frame(double time)
{
  // default: nop
}


/*******************************************************************************
*                              transform caching                               *
*******************************************************************************/

auto entity_t::to_world() const -> mat4_t
{
  if (world_valid_) {
    world_cache_ = to_matrix();
    entity_t *pred = parent_;
    while (pred) {
      world_cache_ = pred->to_matrix() * world_cache_;
      pred = pred->parent_;
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
  if (!children_.empty())
    for (auto subentity : children_)
      subentity->invalidate_cache();
}


/*******************************************************************************
*                               entity hierarchy                               *
*******************************************************************************/

auto entity_t::parent() const -> entity_t *
{
  return parent_;
}

auto entity_t::children() const -> entitylist_t
{
  return children_;
}

void entity_t::add_child(entity_t *entity)
{
  if (entity->parent())
    throw std::runtime_error("Entity already has a parent");
  entity->child_link_ = children_.insert(children_.end(), entity);
  entity->parent_ = this;
}

void entity_t::remove_from_parent()
{
  if (!parent())
    throw std::runtime_error("Entity does not have a parent");
  parent_->children_.erase(child_link_);
  parent_ = nullptr;
}

} // namespace game
} // namespace snow
