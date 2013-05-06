#ifndef __SNOW__COMPONENT_HH__
#define __SNOW__COMPONENT_HH__

#include "../../config.hh"
#include "component_id.hh"
#include <snow/types/object_pool.hh>
#include <cstdint>


namespace snow {


struct game_object_t;


template <typename T, unsigned ID>
struct component_t
{
  static_assert(ID < MAX_COMPONENT_IDS,
    "Component ID must be within the range of valid component IDs");

  static constexpr const unsigned COMPONENT_ID = ID;

  static T *data_for_index(uint16_t index);
  template <typename... ARGS>
  static uint16_t allocate(ARGS &&...);
  static void destroy(uint16_t index);


  component_t() = default;
  component_t(game_object_t *obj);


  game_object_t *game_object = nullptr;

protected:
  ~component_t() = default;

  static object_pool_t<T, uint16_t, false> component_pool_;
};



template <typename T, unsigned ID>
object_pool_t<T, uint16_t, false> component_t<T, ID>::component_pool_;



template <typename T, unsigned ID>
component_t<T, ID>::component_t(game_object_t *obj) :
  game_object(obj)
{
  /* nop */
}



template <typename T, unsigned ID>
T *component_t<T, ID>::data_for_index(uint16_t index)
{
  return &component_pool_[index];
}



template <typename T, unsigned ID>
template <typename... ARGS>
uint16_t component_t<T, ID>::allocate(ARGS &&... args)
{
  return component_pool_.allocate(std::forward<ARGS>(args)...);
}



template <typename T, unsigned ID>
void component_t<T, ID>::destroy(uint16_t index)
{
  return component_pool_.destroy(index);
}


} // namespace snow


#endif /* end __SNOW__COMPONENT_HH__ include guard */
