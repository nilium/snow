#ifndef __SNOW__ENTITY_HH__
#define __SNOW__ENTITY_HH__

#include <snow/config.hh>
#include <snow/types/object_pool.hh>
#include "transform.hh"
#include <vector>

namespace snow {


struct entity_manager_t;


constexpr int NO_ENTITY = -1;


struct S_EXPORT entity_t : public transform_t
{
  using pool_t    = object_pool_t<entity_t, int>;
  using list_t    = std::vector<int>;

  entity_t() = delete;

  virtual void set_translation(const vec3_t &t);
  virtual void set_rotation(const mat3_t &r);
  virtual void set_scale(const vec3_t &s);
  virtual auto to_matrix() const -> mat4_t;
  virtual auto to_world() const -> mat4_t;

  // hierarchy
  auto parent() const -> int;
  auto children() const -> list_t;
  void add_child(entity_t *entity);
  void remove_from_parent();

  auto retain() -> entity_t &;
  void release();

  inline auto manager() -> entity_manager_t * { return manager_; }
  inline auto index() -> int { return index_; }

  inline bool active() const          { return active_; }
  inline void set_active(bool active) { active_ = active; }

private:
  friend pool_t;                          // to allow obj pool construction
  friend pool_t::objects_t;               // same reason as pool_t
  friend class std::allocator<entity_t>;  // same reason as pool_t
  friend entity_manager_t;    // to set index_

  entity_t(entity_manager_t *manager, int index);
  entity_t(const entity_t &other) = default;
  virtual ~entity_t();

  // const because it only modifies cache data, so it's not altering the entity
  // in any meaningful way (if we removed the cache and calculated the local and
  // world transforms at each request, the end result would still be the same)
  void S_HIDDEN invalidate_cache() const;

  // Friend of entity_manager_t
  static void finalizer(entity_t *entity);

  // Manager that created the entity.
  entity_manager_t *manager_;
  // The entity's index in the manager
  int index_;

  // tform cache
  mutable bool tform_valid_ = false;   // whether tform_cache_ is valid
  mutable bool world_valid_ = false;   // whether world_cache_ is valid
  mutable mat4_t tform_cache_;        // cached local transformation matrix
  mutable mat4_t world_cache_;        // cached world transformation matrix

  // hierarchy
  int parent_ = NO_ENTITY;
  list_t children_;
  list_t::iterator child_link_; // only valid while parent_ != null

  // Whether the entity should be considered active
  bool active_;
};


} // namespace snow

#endif /* end __SNOW__ENTITY_HH__ include guard */
