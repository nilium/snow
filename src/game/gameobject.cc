#include "gameobject.hh"


namespace snow {


game_object_t::~game_object_t()
{
  for (unsigned id = 0; id < MAX_COMPONENT_IDS; ++id) {
    if (components_[id]) {
      delete component_handle_t::get(component_indices_[id]);
    }
  }
}



game_object_t *game_object_t::parent()
{
  return parent_;
}



const game_object_t *game_object_t::parent() const
{
  return parent_;
}



void game_object_t::add_child(game_object_t *child)
{
  assert(child != nullptr);
  assert(child->parent_ == nullptr);

  child->parent_ = this;
  child->parent_iter_ = children_.insert(children_.cend(), child);
}



void game_object_t::remove_from_parent()
{
  assert(parent_);

  parent_->children_.erase(parent_iter_);
  parent_ = nullptr;
}



auto game_object_t::children() const -> const list_t &
{
  return children_;
}


} // namespace snow