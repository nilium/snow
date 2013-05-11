#ifndef __SNOW__COMPONENT_HH__
#define __SNOW__COMPONENT_HH__

#include "../../config.hh"
#include "component_id.hh"
#include "component_handle.hh"
#include <snow/types/object_pool.hh>
#include <cstdint>


namespace snow {


/* Default maximum storage reserved for any component type */
enum : size_t { MAX_COMPONENT_STORAGE = 8192 };


struct game_object_t;


struct component_base_t
{
  component_base_t(); // nop ctor
  component_base_t(game_object_t *obj);
  virtual ~component_base_t();

  game_object_t *game_object = nullptr;
};


/*==============================================================================

  Base class for all components. Subclasses may not be inherited from as this
  would break the object pool. It is, however, possible to create multiple
  components that inherit from both component_t and another base data class,
  provided the data class does not inherit from component_t at any point.

  Components may also share the same ID to indicate that they fill the same
  component slot in a game object -- so, a COLLIDER_COMPONENT ID might be
  shared among capsule, AABB, sphere, etc. collider types.

  Subclasses must provide the following:
    - a no-op ctor() that simply calls the no-op superclass ctor()
    - a ctor(game_object_t *) that calls the superclass ctor(game_object_t *)
    - a virtual ~dtor()
    - a static constexpr const char *COMPONENT_NAME member pointing to a string
      naming the component (often the classname sans _t suffix)

==============================================================================*/
template <typename T, unsigned ID, size_t RESERVED = MAX_COMPONENT_STORAGE>
struct component_t : component_base_t
{
  static_assert(ID < MAX_COMPONENT_IDS,
    "Component ID must be within the range of valid component IDs");

  component_t(); // nop ctor
  component_t(game_object_t *obj);
  virtual ~component_t() = 0;

  template <typename Q> Q *get_component();
  template <typename Q> const Q *get_component() const;
  template <typename Q> Q *get_child_component();
  template <typename Q> const Q *get_child_component() const;

  // If the component was not allocated through the pool, the result of this
  // function is undefined and may crash the application.
  const component_handle_t &handle() const;


  static constexpr const unsigned MAX_COMPONENTS  = RESERVED;
  static constexpr const unsigned COMPONENT_ID = ID;

  static const char *name() { return T::COMPONENT_NAME; }
  static T *data_for_index(uint32_t index);

  static void *operator new (size_t);
  static void operator delete (void *);

  // Applies a given member function or regular function to all instances of
  // the component.
  template <typename Q, typename... MARGS, typename... ARGS>
  static void apply_method(Q (T::*function)(MARGS...), ARGS &&... args);

  template <typename Q, typename... MARGS, typename... ARGS>
  static void const_apply_method(Q (T::*function)(MARGS...) const, ARGS &&... args);

  template <typename Q, typename... ARGS>
  static void apply_fn(Q function, ARGS &&... args);

protected:

  struct component_store_t
  {
    using data_t = typename std::aligned_storage<sizeof(T)>::type;
    component_handle_t    handle;
    data_t                component;
  };

  component_store_t *store_ptr();
  const component_store_t *store_ptr() const;

  using component_pool_t = object_pool_t<component_store_t, uint32_t, false>;

  static component_pool_t component_pool_;


#ifndef NDEBUG
  // Counter to log the number of a type of component used in total at any one
  // time during execution. Only to be used in debug mode -- useful if I want
  // to go back and adjust the reserved storage for specific components later.
  struct max_component_log_t {
    static size_t count;
    static size_t max_count;

    static void log_count()
    {
      std::cout << "Total components of type <" << T::COMPONENT_NAME << "> used: " << max_count << std::endl;
    }

    max_component_log_t()
    {
      std::atexit(log_count);
    }
  };

  static max_component_log_t max_component_log;
#endif

}; // struct component_t


} // namespace snow


#include "../gameobject.hh"


namespace snow {


template <typename T, unsigned ID, size_t RESERVED>
typename component_t<T, ID, RESERVED>::component_pool_t component_t<T, ID, RESERVED>::component_pool_((RESERVED));



#ifndef NDEBUG

// Max instance counter

template <typename T, unsigned ID, size_t RESERVED>
typename component_t<T, ID, RESERVED>::max_component_log_t component_t<T, ID, RESERVED>::max_component_log;

template <typename T, unsigned ID, size_t RESERVED>
size_t component_t<T, ID, RESERVED>::max_component_log_t::count = 0;

template <typename T, unsigned ID, size_t RESERVED>
size_t component_t<T, ID, RESERVED>::max_component_log_t::max_count = 0;

#endif



template <typename T, unsigned ID, size_t RESERVED>
component_t<T, ID, RESERVED>::component_t() :
  component_base_t()
{
  /* nop */
}



template <typename T, unsigned ID, size_t RESERVED>
component_t<T, ID, RESERVED>::component_t(game_object_t *obj) :
  component_base_t(obj)
{
  /* nop */
}



template <typename T, unsigned ID, size_t RESERVED>
component_t<T, ID, RESERVED>::~component_t()
{
  /* nop */
}



template <typename T, unsigned ID, size_t RESERVED>
const component_handle_t &component_t<T, ID, RESERVED>::handle() const
{
  return store_ptr()->handle;
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q, typename... ARGS>
void component_t<T, ID, RESERVED>::apply_fn(Q function, ARGS &&... args)
{
  for (component_store_t &store : component_pool_) {
    function(*(T *)&store.component, std::forward<ARGS>(args)...);
  }
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q, typename... MARGS, typename... ARGS>
void component_t<T, ID, RESERVED>::apply_method(Q (T::*function)(MARGS...), ARGS &&... args)
{
  for (component_store_t &store : component_pool_) {
    (((T *)&store.component)->*function)(std::forward<ARGS>(args)...);
  }
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q, typename... MARGS, typename... ARGS>
void component_t<T, ID, RESERVED>::const_apply_method(Q (T::*function)(MARGS...) const, ARGS &&... args)
{
  for (const component_store_t &store : component_pool_) {
    (((const T *)&store.component)->*function)(std::forward<ARGS>(args)...);
  }
}



template <typename T, unsigned ID, size_t RESERVED>
T *component_t<T, ID, RESERVED>::data_for_index(uint32_t index)
{
  return (T *)&component_pool_[index].component;
}



template <typename T, unsigned ID, size_t RESERVED>
void *component_t<T, ID, RESERVED>::operator new (size_t size)
{
  assert(size <= sizeof(T));
  const uint32_t index = component_pool_.make_storage();
  assert(index < RESERVED); // Do not exceed reserved storage amount
  component_store_t &store = component_pool_[index];
  store.handle = component_handle_t::allocate(index);
  component_handle_t::put(store.handle, (component_base_t *)&store.component);
#ifndef NDEBUG
  ++max_component_log.count;
  if (max_component_log.count > max_component_log.max_count) {
    max_component_log.max_count = max_component_log.count;
  }
#endif
  return (void *)&store.component;
}



template <typename T, unsigned ID, size_t RESERVED>
void component_t<T, ID, RESERVED>::operator delete (void *ptr)
{
  component_store_t *store;
  store = (component_store_t *)(((char *)ptr) - offsetof(component_store_t, component));
  component_handle_t::erase(store->handle);
  component_pool_.destroy(store->handle.local_index);
#ifndef NDEBUG
  --max_component_log.count;
#endif
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q>
Q *component_t<T, ID, RESERVED>::get_component()
{
  if (game_object) {
    return game_object->get_component<Q>();
  }
  return nullptr;
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q>
const Q *component_t<T, ID, RESERVED>::get_component() const
{
  if (game_object) {
    return game_object->get_component<Q>();
  }
  return nullptr;
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q>
Q *component_t<T, ID, RESERVED>::get_child_component()
{
  if (game_object) {
    return game_object->get_child_component<Q>();
  }
}



template <typename T, unsigned ID, size_t RESERVED>
template <typename Q>
const Q *component_t<T, ID, RESERVED>::get_child_component() const
{
  if (game_object) {
    return game_object->get_child_component<Q>();
  }
}



template <typename T, unsigned ID, size_t RESERVED>
auto component_t<T, ID, RESERVED>::store_ptr() -> component_store_t *
{
  return (component_store_t *)(((char *)this) - offsetof(component_store_t, component));
}



template <typename T, unsigned ID, size_t RESERVED>
auto component_t<T, ID, RESERVED>::store_ptr() const -> const component_store_t *
{
  return (const component_store_t *)(((const char *)this) - offsetof(component_store_t, component));
}


} // namespace snow


#endif /* end __SNOW__COMPONENT_HH__ include guard */
