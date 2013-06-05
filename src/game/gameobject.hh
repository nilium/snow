#ifndef __SNOW__GAMEOBJECT_HH__
#define __SNOW__GAMEOBJECT_HH__

#include "../config.hh"
#include "../ext/memory_pool.hh"
#include "components/component_handle.hh"
#include "components/component_id.hh"
#include <cassert>
#include <list>


namespace snow {


/*!
  \brief Game object type. Used to store a transformation component as well as a number
  of other components.

  A game object is always guaranteed to have a transform component so as to
  avoid breaking the transformation hierarchy.
*/
struct game_object_t final
{
  //! \brief List container for game object pointers.
  using list_t = std::list<game_object_t *>;
  // static void *operator new (size_t);
  // static void operator delete (void *);

  game_object_t();
  ~game_object_t();

  // Templated get/add/remove component member functions
  template <typename T>
  T* get_component();

  template <typename T>
  const T* get_component() const;

  template <typename T>
  T* get_child_component();

  template <typename T>
  const T* get_child_component() const;

  template <typename T>
  void add_component();

  template <typename T>
  void remove_component();

  template <typename T>
  bool has_component() const;

  game_object_t *parent();
  const game_object_t *parent() const;
  void add_child(game_object_t *);
  void remove_from_parent();
  const list_t &children() const;

private:
  void remove_component(unsigned component_id);

  mutable std::bitset<MAX_COMPONENT_IDS> components_;
  std::array<component_handle_t, MAX_COMPONENT_IDS> component_indices_;

  game_object_t *parent_ = nullptr;
  list_t::const_iterator parent_iter_;
  list_t children_;
};


/*!
  \brief Gets the component of type T associated with the game object if
  available.

  \returns A component of type T on success or nullptr on failure.
*/
template <typename T>
T *game_object_t::get_component()
{
  if (components_[T::COMPONENT_ID]) {
    T* R = T::data_for_index(component_indices_[T::COMPONENT_ID].local_index);
    assert(R);
    if (!R) {
      components_[T::COMPONENT_ID] = false;
    }
    return R;
  }
  return nullptr;
}



/*!
  See get_component()
*/
template <typename T>
const T *game_object_t::get_component() const
{
  if (components_[T::COMPONENT_ID]) {
    const T* R = T::data_for_index(component_indices_[T::COMPONENT_ID].local_index);
    assert(R);
    if (!R) {
      components_[T::COMPONENT_ID] = false;
    }
    return R;
  }
  return nullptr;
}



/*!
  \brief Gets the first component of type T available in the game object's
  children.
  \returns A pointer to a component in a child it succeeds, otherwise nullptr.
*/
template <typename T>
T *game_object_t::get_child_component()
{
  T *child_comp = nullptr;
  for (game_object_t *child : children_) {
    child_comp = child->get_component<T>();
    if (!child_comp) {
      child_comp = child->get_child_component<T>();
      if (child_comp) {
        break;
      }
    } else {
      break;
    }
  }
  return child_comp;
}



/*!
  \see get_child_component()
*/
template <typename T>
const T *game_object_t::get_child_component() const
{
  const T *child_comp = nullptr;
  for (const game_object_t *child : children_) {
    child_comp = child->get_component<T>();
    if (!child_comp) {
      child_comp = child->get_child_component<T>();
      if (child_comp) {
        break;
      }
    } else {
      break;
    }
  }
  return child_comp;
}



/*!
  \brief Allocates a component of type T and associates it with this game
  object.

  If the game object already has a component with the same component_id_t as T,
  then behavior is undefined.
*/
template <typename T>
void game_object_t::add_component()
{
  assert(!components_[T::COMPONENT_ID]);
  component_indices_[T::COMPONENT_ID] = (new T(this))->handle();
  components_[T::COMPONENT_ID] = true;
}



/*!
  \brief Removes a component of type T from the game object.

  If the object has an associated component of the same type ID but of a
  different type, the behavior is undefined.
*/
template <typename T>
void game_object_t::remove_component()
{
  // Transform may not be removed from objects so as not to break transform
  // hierarchy needed for rendering
  assert(T::COMPONENT_ID != TRANSFORM_COMPONENT);
  assert(components_[T::COMPONENT_ID]);
  delete T::data_for_index(component_indices_[T::COMPONENT_ID].local_index);
  components_[T::COMPONENT_ID] = false;
}



/*!
  \brief Returns whether the component has an associated component of type T.

  If it has an associated component of some other type but the same component
  ID, it will also return true. However, further behavior in this case is
  undefined.
*/
template <typename T>
bool game_object_t::has_component() const
{
  if (T::COMPONENT_ID == TRANSFORM_COMPONENT) {
    return true;
  } else {
    return components_[T::COMPONENT_ID];
  }
}


} // namespace snow


#endif /* end __SNOW__GAMEOBJECT_HH__ include guard */
