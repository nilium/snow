/*
  gameobject.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "gameobject.hh"
#include "components/component.hh"
#include "components/transform.hh"


namespace snow {


/*!
  \brief Constructor
*/
game_object_t::game_object_t()
{
  add_component<transform_t>();
}



/*!
  \brief Destructor
*/
game_object_t::~game_object_t()
{
  for (unsigned id = 0; id < MAX_COMPONENT_IDS; ++id) {
    if (components_[id]) {
      delete component_handle_t::get(component_indices_[id]);
    }
  }
}



/*!
  \brief Gets the parent game object, if one is present.
  \returns A pointer to a game object or nullptr.
*/
game_object_t *game_object_t::parent()
{
  return parent_;
}



/*!
  \see parent()
*/
const game_object_t *game_object_t::parent() const
{
  return parent_;
}



/*!
  \brief Adds a child to the game object's children.
  \param in child The child to add.
*/
void game_object_t::add_child(game_object_t *child)
{
  assert(child != nullptr);
  assert(child->parent_ == nullptr);

  child->parent_ = this;
  child->parent_iter_ = children_.insert(children_.cend(), child);
}



/*!
  \brief Removes this object from its parent's children.
*/
void game_object_t::remove_from_parent()
{
  assert(parent_);

  parent_->children_.erase(parent_iter_);
  parent_ = nullptr;
}



/*!
  \brief Returns a const reference to the object's list of children.

  Can be used for iterating over children or recursing down through the scene
  graph. At no point should a reference or pointer to this be stored.
*/
auto game_object_t::children() const -> const list_t &
{
  return children_;
}


} // namespace snow
