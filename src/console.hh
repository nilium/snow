#ifndef __SNOW__CONSOLE_HH__
#define __SNOW__CONSOLE_HH__

#include "config.hh"
#include "data/database.hh"
#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>


namespace snow {


enum cvar_flags_t : unsigned
{
  // Note: all cvar flags that restrict write access are ignored when the cvar
  // is forcibly set to a value.
  // Cvar is read-only at runtime.
  CVAR_READ_ONLY        = 0x1 << 0,
  // Cvar is intended only for program initialization and should not be changed
  // afterward
  CVAR_INIT_ONLY        = 0x1 << 1,
  // Cvar is saved when write_cvars is called.
  CVAR_SAVED            = 0x1 << 2,
  // Cvar is for cheating/debugging only and should cannot be changed if cheats
  // are disabled.
  CVAR_CHEAT            = 0x1 << 3,
  // Cvar changes should be sent to the server and ergo all clients.
  // this implies CVAR_SERVER even if unset.
  CVAR_CLIENT           = 0x1 << 4,
  // Cvar changes should be sent to the server (may result in the server
  // rejecting the change and sending a cvar-set command back [or a revoke]).
  CVAR_SERVER           = 0x1 << 5,
  // cvar is user created (usually goes with CVAR_DEALLOC).
  CVAR_USER             = 0x1 << 6,
  // Changes to the cvar are delayed until cvar_set_t::update_cvars is called
  // if the cvar is bound to a cvar_set_t. Otherwise, has no effect.
  CVAR_DELAYED          = 0x1 << 7,
  // Specifies the cvar is invisible to the user (e.g., via a console). This is
  // never a default flag.
  CVAR_INVISIBLE        = 0x1 << 8,
  // Cvar has been modified (do not set this yourself -- it's not needed)
  CVAR_MODIFIED         = 0x1 << 16,
  CVAR_HAS_CACHE        = CVAR_DELAYED | CVAR_MODIFIED,

  // cvar types
  CVAR_INT              = 0x1 << 21,
  CVAR_FLOAT            = 0x1 << 22,
  CVAR_STRING           = 0x1 << 23,
  CVAR_TYPE_MASK        = CVAR_INT | CVAR_FLOAT | CVAR_STRING,

  // cached cvar types
  CVAR_CACHED_INT       = 0x1 << 24,
  CVAR_CACHED_FLOAT     = 0x1 << 25,
  CVAR_CACHED_STRING    = 0x1 << 26,
  CVAR_CACHED_MASK      = CVAR_CACHED_INT | CVAR_CACHED_FLOAT | CVAR_CACHED_STRING,
  CVAR_CACHE_STRIP_MASK = ~(CVAR_MODIFIED | CVAR_TYPE_MASK | CVAR_CACHED_MASK),
  CVAR_TYPE_SHIFT       = 3,

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
  cvar_t(const string &name, int value, unsigned flags = CVAR_FLAGS_DEFAULT);
  cvar_t(const string &name, float value, unsigned flags = CVAR_FLAGS_DEFAULT);
  cvar_t(const string &name, const string &value, unsigned flags = CVAR_FLAGS_DEFAULT);

  cvar_t(const cvar_t &) = default;

  cvar_t(cvar_t &&) = delete;
  cvar_t &operator = (const cvar_t &) = delete;
  cvar_t &operator = (cvar_t &&) = delete;

  uint32_t name_hash() const;
  const string &name() const;
  unsigned flags() const;
  bool has_flags(unsigned flags) const;

  int type() const;

  int geti() const;
  float getf() const;
  const string &gets() const;

  // Always checks cvar permissions before setting
  void seti(int value);
  void setf(float value);
  void sets(const string &value);

  // If forced, will ignore cvar permissions temporarily. If allowing the user
  // to arbitrarily set flags, do not set 'force' to true for their actions.
  void seti_force(int value, bool force);
  void setf_force(float value, bool force);
  void sets_force(const string &value, bool force);

  // Revokes cached changes to the cvar (if delayed). If using an iterator,
  // the iterator for this cvar will be invalidated. It would be better to
  // keep your own list of cvars you'll revoke changes to and iterate over it
  // to revoke changes so you don't accidentally end up with an invalid iter.
  void revoke_changes();

  // Updates the current value to the cached value -- typically only called by
  // cvar_set_t::update_cvars
  void update();

private:
  friend struct cvar_set_t;

  bool can_modify() const;

  struct cvar_set_t *owner_;
  uint32_t        hash_;
  unsigned        flags_;     // associated flags
  int             int_value_;   // atoi(value_.c_str()) (atoX skips exceptions)
  float           float_value_; // atof(value_.c_str())
  string          name_;   // cvar name
  string          value_;  // to_string(int or float)
  string          cache_;  // cached value (not retreived prior to update)
  std::list<cvar_t *>::const_iterator update_iter_;
};



struct ccmd_t
{
  using arg_t = std::pair<string::const_iterator, string::const_iterator>;
  using args_t = std::deque<arg_t>;
  using ccmd_fn_t = std::function<void(cvar_set_t &cvars, const args_t &)>;

  ccmd_t(const string &name, const ccmd_fn_t &fn);
  ccmd_t(const string &name, ccmd_fn_t &&fn);

  uint32_t name_hash() const;
  const string &name() const;

  // This is a no-op if either:
  // 1) the call_ function is a nullptr (this will crash the ctor though)
  // 2) the ccmd_t is not registered ot a cvar_set_t.
  void call(const args_t &);
  void call(const string &args_str);


  static size_t ccmd_arg_iters(const string &str, args_t &out, size_t max = SIZE_MAX);

private:
  friend struct cvar_set_t;

  cvar_set_t *  owner_;
  string        name_;
  uint32_t      hash_;
  ccmd_fn_t     call_;
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

  cvar_set_t();

  // Inserts or updates values for any cvars in the cvar set into the database's
  // console_variables table.
  void write_cvars(database_t &db);
  // Reads values for any cvars in the cvar set from the database's
  // console_variables table. If the cvar does not exist in the cvar set, it
  // will be ignored.
  void read_cvars(database_t &db);

  // Tries to find a cvar with the given name, returns it if it exists. Returns
  // nullptr otherwise.
  cvar_t *get_cvar(const string &name) const;
  // Tries to find a cvar with the given hash, returns it if it exists.
  // Otherwise, returns nullptr.
  cvar_t *get_cvar(uint32_t hash) const;
  // Tries to find a cvar with the given name, returns it if it exists. If it
  // doesn't exist, a new cvar will be created with the provided default_value
  // and default_flags and registered in the cvar set.
  cvar_t *get_cvar(const string &name,
    const string &default_value,
    int default_flags = CVAR_FLAGS_DEFAULT);
  cvar_t *get_cvar(const string &name,
    int default_value,
    int default_flags = CVAR_FLAGS_DEFAULT);
  cvar_t *get_cvar(const string &name,
    float default_value,
    int default_flags = CVAR_FLAGS_DEFAULT);

  // Allocates a cvar using an internal vector of cvars and returns it.
  // This does not register the cvar in the cvar_set_t, though the pointer to
  // the returned cvar will be invalid either after the cvar set's destruction
  // or after clear() is called.
  cvar_t *make_cvar(const string &name, int value, unsigned flags = CVAR_FLAGS_DEFAULT);
  cvar_t *make_cvar(const string &name, float value, unsigned flags = CVAR_FLAGS_DEFAULT);
  cvar_t *make_cvar(const string &name, const string &value, unsigned flags = CVAR_FLAGS_DEFAULT);
  // Adds an existing cvar to the cvar set. This cvar should not be shared with
  // other cvar sets. This does not copy the cvar, it keeps the address in its
  // internal map.
  bool register_cvar(cvar_t *cvar);
  bool unregister_cvar(cvar_t *cvar);

  void update_cvars();

  bool register_ccmd(ccmd_t *ccmd);
  bool unregister_ccmd(ccmd_t *ccmd);
  // Gets a console command with the given name. Returns it if found, nullptr
  // otherwise.

  // Will do one of three things:
  // Either a) it will call a ccmd with the first name in the command string or
  // b) if the string is a single name, it will get the value of the named cvar
  // or c) if the string has multiple values and names a cvar, it will set the
  // cvar to the combination of all those values minus preceeding/trailing
  // whitespace.
  void execute(const string &command, bool force = false);

  ccmd_t *get_ccmd(const string &name) const;
  // Hash version of above.
  ccmd_t *get_ccmd(uint32_t hash) const;
  // Calls the given console command, if it exists, with the given argument string.
  // This is essentially a convenience function for getting the ccmd by name and
  // calling it. Returns true if the ccmd was called (regardless of whether the
  // ccmd was successful or not).
  bool call_ccmd(const string &name, const ccmd_t::args_t &args);

  // Clears both cvars and ccmds from the set
  void clear();

  // Use these before calling update_cvars to iterate over all changed cvars
  ptr_list_t::const_iterator modified_cbegin() const;
  ptr_list_t::const_iterator modified_cend() const;

private:
  friend struct cvar_t;
  struct console_item_t {
    enum : unsigned {
      KIND_CVAR,
      KIND_CCMD
    } kind;
    union {
      cvar_t *cvar;
      ccmd_t *cmd;
    };
  };

  using item_map_t = std::unordered_map<uint32_t, console_item_t>;

  item_map_t cvars_ { };
  ptr_list_t update_cvars_ { };
  // FIXME: Use std::list so cvar_t/ccmd_t pointers aren't invalidated later
  // NOTE: determine a fixed size for the vector, reserve
  // enough, and then quietly fail on overflow.
  std::vector<cvar_t> temp_cvars_     { };
};


} // namespace snow

#endif /* end __SNOW__CONSOLE_HH__ include guard */
