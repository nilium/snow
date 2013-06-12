/*
  component_handle.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__COMPONENT_HANDLE_HH__
#define __SNOW__COMPONENT_HANDLE_HH__

#include "../../config.hh"
#include <cstdint>


namespace snow {


struct component_base_t;
struct game_object_t;


struct component_handle_t final
{
  uint32_t local_index;   // Index within the component's own pool
  uint32_t global_index;  // Global index used to identify the component among all others

  // Short-hand for get(*this)
  const component_base_t *get() const;
  component_base_t *get();

  // Gets a new handle with a new global index and the given local index.
  static component_handle_t allocate(uint32_t local);
  // Maps the given component handle to the component address.
  static void put(const component_handle_t &handle, component_base_t *component);
  // Gets the component associated with a given handle. Returns null if no
  // component is found. If the handle isn't associated with a handle, however,
  // it will fail an assert.
  static component_base_t *get(const component_handle_t &handle);
  // Unmaps any component with the given handle from the global component map.
  static void erase(const component_handle_t &handle);


  bool operator == (const component_handle_t &other) const;
  bool operator != (const component_handle_t &other) const;
  bool operator <= (const component_handle_t &other) const;
  bool operator >= (const component_handle_t &other) const;
  bool operator <  (const component_handle_t &other) const;
  bool operator >  (const component_handle_t &other) const;
};


} // namespace snow

#endif /* end __SNOW__COMPONENT_HANDLE_HH__ include guard */
