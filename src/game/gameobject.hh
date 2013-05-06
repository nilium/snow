#ifndef __SNOW__GAMEOBJECT_HH__
#define __SNOW__GAMEOBJECT_HH__

#include "../config.hh"
#include "../ext/memory_pool.hh"
#include "components/component_id.hh"
#include "components/transform.hh"
#include <cassert>
#include <list>


namespace snow {


struct game_object_t final
{
  using list_t = std::list<game_object_t *>;
  // static void *operator new (size_t);
  // static void operator delete (void *);

  ~game_object_t();

  // Templated get/add/remove component member functions
  template <typename T>
  T* get_component();
  template <typename T>
  const T* get_component() const;

  template <typename T>
  void add_component();

  template <typename T>
  void remove_component();

  game_object_t *parent();
  const game_object_t *parent() const;
  void add_child(game_object_t *);
  void remove_from_parent();
  const list_t &children() const;

private:
  void remove_component(unsigned component_id);

  static mempool_t g_memory_;

  std::bitset<MAX_COMPONENT_IDS> components_;
  std::array<uint16_t, MAX_COMPONENT_IDS> component_indices_;

  game_object_t *parent_ = nullptr;
  list_t::const_iterator parent_iter_;
  list_t children_;
};


template <typename T>
T *game_object_t::get_component()
{
  if (components_[T::COMPONENT_ID]) {
    return T::data_for_index(component_indices_[T::COMPONENT_ID]);
  }
  return nullptr;
}



template <typename T>
const T *game_object_t::get_component() const
{
  if (components_[T::COMPONENT_ID]) {
    return T::data_for_index(component_indices_[T::COMPONENT_ID]);
  }
  return nullptr;
}



template <typename T>
void game_object_t::add_component()
{
  assert(!components_[T::COMPONENT_ID]);
  component_indices_[T::COMPONENT_ID] = T::allocate(this);
}



template <typename T>
void game_object_t::remove_component()
{
  assert(components_[T::COMPONENT_ID]);
  T::destroy(component_indices_[T::COMPONENT_ID]);\
  components_[T::COMPONENT_ID] = false;
}


} // namespace snow


#endif /* end __SNOW__GAMEOBJECT_HH__ include guard */
