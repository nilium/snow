#ifndef __SNOW__CONSOLE_HH__
#define __SNOW__CONSOLE_HH__

#include <snow/config.hh>
#include "data/database.hh"
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>


namespace snow {


enum cvar_flags_t : int
{
  // cvar is read-only at runtime.
  CVAR_READ_ONLY        = 0x1 << 0,
  // cvar is intended only for program initialization and should not be changed
  // afterward
  CVAR_INIT_ONLY        = 0x1 << 1,
  // cvar is saved when write_cvars is called.
  CVAR_SAVED            = 0x1 << 2,
  // cvar is for cheating/debugging only and should cannot be changed if cheats
  // are disabled.
  CVAR_CHEAT            = 0x1 << 3,
  // cvar changes should be sent to the server and ergo all clients.
  // this implies CVAR_SERVER even if unset.
  CVAR_CLIENT           = 0x1 << 4,
  // cvar changes should be sent to the server (may result in the server
  // rejecting the change and sending a cvar-set command back)
  CVAR_SERVER           = 0x1 << 5,
  // cvar is user created (usually goes with CVAR_DEALLOC)
  CVAR_USER             = 0x1 << 7,
  // changes to the cvar are delayed until the next frame -- recommended for
  // most purposes since it'll keep internal db transactions down.
  CVAR_DELAYED          = 0x1 << 8,
  // cvar has been modified (do not set this yourself -- it's not needed)
  CVAR_MODIFIED         = 0x1 << 24,

  // Some common combinations
  CVAR_FLAGS_DEFAULT    = CVAR_DELAYED,
  CVAR_FLAGS_GAME       = CVAR_SAVED | CVAR_DELAYED,
  CVAR_FLAGS_RENDERER   = CVAR_FLAGS_GAME,
  CVAR_FLAGS_SERVER     = CVAR_SAVED | CVAR_DELAYED | CVAR_SERVER,
  CVAR_FLAGS_CHEATS     = CVAR_CHEAT | CVAR_DELAYED | CVAR_SERVER,
};


#define CHEATS_CVAR_NAME "g_cheats"


struct cvar_t
{
  cvar_t(const string &name, int value, int flags = CVAR_FLAGS_DEFAULT);
  cvar_t(const string &name, float value, int flags = CVAR_FLAGS_DEFAULT);
  cvar_t(const string &name, const string &value, int flags = CVAR_FLAGS_DEFAULT);

  cvar_t(const cvar_t &) = default;

  cvar_t(cvar_t &&) = delete;
  cvar_t &operator = (const cvar_t &) = delete;
  cvar_t &operator = (cvar_t &&) = delete;

  const string &name() const;
  int flags() const;
  bool has_flags(int flags) const;

  int getl() const;
  float getf() const;
  const string &gets() const;

  // Always checks cvar permissions before setting
  void setl(int value);
  void setf(float value);
  void sets(const string &value);

  // If forced, will ignore cvar permissions temporarily. Avoid using these
  // in game code.
  void setl_force(int value, bool force);
  void setf_force(float value, bool force);
  void sets_force(const string &value, bool force);

  // Revokes cached changes to the cvar (if delayed). If using an iterator,
  // the iterator for this cvar will be invalidated.
  void revoke_changes();

private:
  friend struct cvar_set_t;

  bool can_modify() const;
  // Updates the current value to the cached value -- only called by
  // cvar_set_t::update_cvars
  void update();

  struct cvar_set_t *owner_;
  int     flags_;     // associated flags
  int     int_value_;   // atoi(value_.c_str()) (atoX skips exceptions)
  float   float_value_; // atof(value_.c_str())
  string  name_;   // cvar name
  string  value_;  // to_string(int or float)
  string  cache_;  // cached value (not the value retreived prior to update)
  std::list<cvar_t *>::const_iterator update_iter_;
};



/*==============================================================================

  cvar_set_t - a simple container for cvars. Doesn't depend on any globals aside
  from constant strings, so the set is for the most part safe to use from
  multiple threads as long as only one thread accesses it at a time. So, two
  cvar_set_ts could exist and each could be accessed by a different thread, but
  each set's thread could not access the other thread's set.

==============================================================================*/
struct cvar_set_t
{
  using ptr_list_t = std::list<cvar_t *>;

  // Inserts or updates values for any cvars in the cvar set into the database's
  // console_variables table.
  void write_cvars(database_t &db);
  // Reads values for any cvars in the cvar set from the database's
  // console_variables table. If the cvar does not exist in the cvar set, it
  // will be ignored.
  void read_cvars(database_t &db);

  cvar_t *get_cvar(const string &name) const;

  // Allocates a cvar using an internal vector of cvars and registers it.
  cvar_t *make_cvar(const string &name, int value, int flags = CVAR_FLAGS_DEFAULT);
  cvar_t *make_cvar(const string &name, float value, int flags = CVAR_FLAGS_DEFAULT);
  cvar_t *make_cvar(const string &name, const string &value, int flags = CVAR_FLAGS_DEFAULT);
  // Adds an existing cvar to the cvar set. This cvar should not be shared with
  // other cvar sets. This does not copy the cvar, it keeps the address in its
  // internal map.
  bool register_cvar(cvar_t *cvar);
  bool unregister_cvar(cvar_t *cvar);

  void update_cvars();
  void clear();

  // Use these before calling update_cvars to iterate over all changed cvars
  ptr_list_t::const_iterator modified_cbegin() const;
  ptr_list_t::const_iterator modified_cend() const;

private:
  friend struct cvar_t;

  std::map<string, cvar_t *> cvars_   { };
  ptr_list_t update_cvars_ { };
  std::vector<cvar_t> temp_cvars_     { };
};


} // namespace snow

#endif /* end __SNOW__CONSOLE_HH__ include guard */
