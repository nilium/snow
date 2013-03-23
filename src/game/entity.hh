#ifndef __SNOW__ENTITY_HH__
#define __SNOW__ENTITY_HH__

#include <snow/config.hh>
#include "transform.hh"
#include <list>

namespace snow {
namespace game{
struct entity_t;

/*******************************************************************************
         foo bar baz quux som amount of stupid is contained within thee
*******************************************************************************/

typedef std::list<entity_t *> entitylist_t;

struct S_EXPORT entity_t : public transform_t
{
  entity_t() = default;
  virtual ~entity_t();

  virtual void set_translation(const vec3_t &t);
  virtual void set_rotation(const mat3_t &r);
  virtual void set_scale(const vec3_t &s);
  virtual auto to_matrix() const -> mat4_t;
  virtual auto to_world() const -> mat4_t;

  // Override for specialized subclasses
  virtual void frame(double time);

  // hierarchy
  virtual auto parent() const -> entity_t *;
  virtual auto children() const -> entitylist_t;
  virtual void add_child(entity_t *entity);
  virtual void remove_from_parent();

private:
  // const because it only modifies cache data, so it's not altering the entity
  // in any meaningful way (if we removed the cache and calculated the local and
  // world transforms at each request, the end result would still be the same)
  void S_HIDDEN invalidate_cache() const;

  // frame loop stuff
  double hertz_;

  // tform cache
  mutable bool tform_valid_ = false;   // whether tform_cache_ is valid
  mutable bool world_valid_ = false;   // whether world_cache_ is valid
  mutable mat4_t tform_cache_;        // cached local transformation matrix
  mutable mat4_t world_cache_;        // cached world transformation matrix

  // hierarchy
  entity_t *parent_;
  entitylist_t children_;
  entitylist_t::iterator child_link_; // only valid while parent_ != null
};

} // namespace snow
} // namespace game

#endif /* end __SNOW__ENTITY_HH__ include guard */
