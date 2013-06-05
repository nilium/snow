#include "component_handle.hh"
#include <map>
#include <unordered_map>
#include <cassert>


namespace snow {


struct hash_handle_t {
  uint_fast64_t operator () (const component_handle_t &handle) const
  {
    const uint_fast64_t left = static_cast<uint_fast64_t>(handle.local_index);
    const uint_fast64_t right = static_cast<uint_fast64_t>(handle.global_index);
    return ((left & 0xFFFF) | ((right & 0xFFFF) << 16) |
      ((left & 0xFFFF0000) << 32) | ((right & 0xFFFF0000) << 16));
  }
};



using component_map_t = typename std::conditional<true, //sizeof(size_t) >= 8,
  std::unordered_map<component_handle_t, component_base_t *, hash_handle_t>,
  std::map<component_handle_t, component_base_t *> >::type;
// using component_map_t = std::unordered_map<component_handle_t, component_base_t *, hash_handle_t>;


static component_map_t g_components;



component_handle_t component_handle_t::allocate(uint32_t local)
{
  static uint32_t global_count = 0xDEADBEEFUL;
  const size_t start_index = global_count;
  component_handle_t result = {
    local, ++global_count
  };
  while (g_components.find(result) != g_components.end() && global_count != start_index) {
    result.global_index = ++global_count;
  }
  if (global_count == start_index) {
    std::clog << "No free global indices for a component, somehow." << std::endl;
    assert(global_count == start_index);
  }
  return result;
}



void component_handle_t::put(const component_handle_t &handle, component_base_t *component)
{
#ifndef NDEBUG
  const auto result =
#endif
  g_components.insert({ handle, component });
#ifndef NDEBUG
  assert(result.second);
#endif
}



component_base_t *component_handle_t::get(const component_handle_t &handle)
{
  auto iter = g_components.find(handle);
  if (iter != g_components.end()) {
    return iter->second;
  }
  return nullptr;
}



void component_handle_t::erase(const component_handle_t &handle)
{
  auto iter = g_components.find(handle);
  if (iter != g_components.end()) {
    g_components.erase(iter);
  }
}



const component_base_t *component_handle_t::get() const
{
  return get(*this);
}



component_base_t *component_handle_t::get()
{
  return get(*this);
}



bool component_handle_t::operator == (const component_handle_t &other) const
{
  return local_index == other.local_index && global_index == other.global_index;
}



bool component_handle_t::operator != (const component_handle_t &other) const
{
  return local_index != other.local_index || global_index != other.global_index;
}



bool component_handle_t::operator <= (const component_handle_t &other) const
{
  return local_index <= other.local_index && global_index <= other.global_index;
}



bool component_handle_t::operator >= (const component_handle_t &other) const
{
  return local_index >= other.local_index && global_index >= other.global_index;
}



bool component_handle_t::operator <  (const component_handle_t &other) const
{
  return local_index < other.local_index && global_index <= other.global_index;
}



bool component_handle_t::operator >  (const component_handle_t &other) const
{
  return local_index > other.local_index && global_index >= other.global_index;
}


} // namespace snow
