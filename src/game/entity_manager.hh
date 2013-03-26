#ifndef __SNOW__ENTITY_MANAGER_HH__
#define __SNOW__ENTITY_MANAGER_HH__

#include <list>
#include "entity.hh"
#include <snow/types/object_pool.hh>
#include <snow/memory/ref_counter.hh>

namespace snow {


struct S_EXPORT entity_manager_t
{
  using list_t = std::list<int>;

  entity_manager_t() = default;
  ~entity_manager_t() = default;

  entity_t &  make_entity();
  entity_t &  get_entity(int index);

  list_t      active_entities() const;

private:
  friend void entity_t::finalizer(entity_t *);
  friend entity_t &entity_t::retain();
  friend void entity_t::release();

  entity_t::pool_t  entities_;
  ref_counter_t     counter_;

  void        destroy_entity(int index);
  void        retain_entity(const entity_t *entity);
  void        release_entity(entity_t *entity);
};


} // namespace snow

#endif /* end __SNOW__ENTITY_MANAGER_HH__ include guard */
