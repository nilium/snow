/*
  console.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "console.hh"
#include <cstdlib>
#include <snow-ext/hash.hh>


namespace snow {


namespace {


/*!
  The default storage for cvar caches and string values. Should hold any
  32-bit integer.
*/
#define CVAR_RESERVE_STORAGE (11)


/*!
  SQL statement to create the console_variables table if it doesn't already
  exist in a DB.
*/
const string create_cvar_table_string {
  "CREATE IF NOT EXISTS console_variables "
  "(name AS TEXT UNIQUE OR REPLACE, value AS TEXT)"
};


/*!
  Grabs a console variable's value from a DB by its name.
*/
const string cvar_single_query_string {
  "SELECT value FROM console_variables WHERE name = :name LIMIT 1"
};


/*!
  Writes a console variable's value to a DB, either inserting it if it's new or
  replacing the existing value of the variable.
*/
const string cvar_update_string {
  "INSERT OR REPLACE INTO console_variables (:name, :value)"
};


using arg_t = ccmd_t::arg_t;
using args_t = ccmd_t::args_t;


} // namespace <anon>

/*******************************************************************************
*                                    cvar_t                                    *
*******************************************************************************/

/*!
  \brief Constructs a cvar with an initial integer value.
*/
cvar_t::cvar_t(const string &name, int value, unsigned flags) :
  owner_(nullptr),
  hash_(murmur3::hash32(name)),
  flags_(flags & ~CVAR_DELAYED),
  name_(name),
  cache_()
{
  cache_.reserve(CVAR_RESERVE_STORAGE);
  value_.reserve(CVAR_RESERVE_STORAGE);
  seti_force(value, true);
  // if delayed, no longer delayed, also strip modified flag if someone was
  // dumb enough to pass that in.
  flags_ = flags & ~CVAR_MODIFIED;
}



/*!
  \brief Constructs a cvar with an initial float value.
*/
cvar_t::cvar_t(const string &name, float value, unsigned flags) :
  owner_(nullptr),
  hash_(murmur3::hash32(name)),
  flags_(flags&~CVAR_DELAYED),
  name_(name),
  cache_()
{
  cache_.reserve(CVAR_RESERVE_STORAGE);
  value_.reserve(CVAR_RESERVE_STORAGE);
  setf_force(value, true);
  flags_ = flags & ~CVAR_MODIFIED;
}



/*!
  \brief Constructs a cvar with an initial string value.
*/
cvar_t::cvar_t(const string &name, const string &value, unsigned flags) :
  owner_(nullptr),
  hash_(murmur3::hash32(name)),
  flags_(flags&~CVAR_DELAYED),
  name_(name),
  cache_()
{
  cache_.reserve(CVAR_RESERVE_STORAGE);
  value_.reserve(CVAR_RESERVE_STORAGE);
  sets_force(value, true);
  flags_ = flags & ~CVAR_MODIFIED;
}



/*!
  \brief Gets the name of the cvar.
*/
const string &cvar_t::name() const
{
  return name_;
}



/*!
  \brief Gets the name hash of the cvar.
*/
uint32_t cvar_t::name_hash() const
{
  return hash_;
}



/*!
  \brief Gets any flags associated with the cvar.
*/
unsigned cvar_t::flags() const
{
  return flags_;
}



/*!
  \brief Tests if the cvar has a flag or a specific combination of flags.
*/
bool cvar_t::has_flags(unsigned flags) const
{
  return (flags_ & flags) == flags;
}



/*!
  \brief Returns the type of the cvar.
  \see CVAR_INT
  \see CVAR_STRING
  \see CVAR_FLOAT
*/
int cvar_t::type() const
{
  return flags_ & CVAR_TYPE_MASK;
}



/*!
  \brief Gets the float value of the cvar.

  If the cvar holds a string that doesn't represent any int value, the result
  will be zero. The cvar's type will not change.
*/
int cvar_t::geti() const
{
  return int_value_;
}



/*!
  \brief Gets the float value of the cvar.

  If the cvar holds a string that doesn't represent any float value, the
  result will be zero. The cvar's type will not change.
*/
float cvar_t::getf() const
{
  return float_value_;
}



/*!
  \brief Gets the string value of the cvar.

  If the cvar is of a numeric type, it will return a string version of that
  value. The cvar's type will not change.
*/
const string &cvar_t::gets() const
{
  return value_;
}



/*!
  \brief Sets the cvar to an int value. Skips restriction flags.
  \see seti_force()
*/
void cvar_t::seti(int value)
{
  seti_force(value, true);
}



/*!
  \brief Sets the cvar to a float value. Skips restriction flags.
  \see setf_force()
*/
void cvar_t::setf(float value)
{
  setf_force(value, true);
}



/*!
  \brief Sets the cvar to a string value. Skips restriction flags.
  \see sets_force()
*/
void cvar_t::sets(const string &value)
{
  sets_force(value, true);
}



/*!
  \brief Sets the cvar value to an int, optionally skipping flag checks.

  The cvar's type will be changed to CVAR_INT if it was not already an integer.

  If `force` is true, the cvar's flags are not checked to see if the cvar is
  modifiable. It will simply set the value of the cvar. \note The CVAR_DELAYED
  flag is respected by this function.
*/
void cvar_t::seti_force(int value, bool force)
{
#if !NDEBUG
  if (has_flags(CVAR_HAS_CACHE)) {
    s_log_warning("Overwriting cached value for cvar %s", name_.c_str());
  }
#endif

  if (!(force || can_modify())) {
    return;
  } else if (owner_ && has_flags(CVAR_DELAYED)) {
    flags_ = (flags_ & ~CVAR_CACHED_MASK) | CVAR_CACHED_INT;
    cache_ = std::to_string(value);
  } else {
    flags_ = (flags_ & ~CVAR_TYPE_MASK) | CVAR_INT;
    int_value_ = value;
    float_value_ = (float)value;
    value_ = std::to_string(value);
  }

  if (owner_) {
    flags_ |= CVAR_MODIFIED;
    auto at_end = owner_->update_cvars_.cend();
    update_iter_ = owner_->update_cvars_.insert(at_end, this);
  }
}



/*!
  \brief Sets the cvar value to a float, optionally skipping flag checks.

  The cvar's type will be changed to CVAR_FLOAT if it was not already a float.

  If `force` is true, the cvar's flags are not checked to see if the cvar is
  modifiable. It will simply set the value of the cvar. \note The CVAR_DELAYED
  flag is respected by this function.
*/
void cvar_t::setf_force(float value, bool force)
{
#if !NDEBUG
  if (has_flags(CVAR_HAS_CACHE)) {
    s_log_warning("Overwriting cached value for cvar %s", name_.c_str());
  }
#endif

  if (!(force || can_modify())) {
    return;
  } else if (owner_ && has_flags(CVAR_DELAYED)) {
    flags_ = (flags_ & ~CVAR_CACHED_MASK) | CVAR_CACHED_FLOAT;
    cache_ = std::to_string(value);
  } else {
    flags_ = (flags_ & ~CVAR_TYPE_MASK) | CVAR_FLOAT;
    int_value_ = (int)value;
    float_value_ = value;
    value_ = std::to_string(value);
  }

  if (owner_) {
    flags_ |= CVAR_MODIFIED;
    auto at_end = owner_->update_cvars_.cend();
    update_iter_ = owner_->update_cvars_.insert(at_end, this);
  }
}



/*!
  \brief Sets the cvar value to a string, optionally skipping flag checks.

  The cvar's type will be changed to CVAR_STRING if it was not already a string.

  If `force` is true, the cvar's flags are not checked to see if the cvar is
  modifiable. It will simply set the value of the cvar. \note The CVAR_DELAYED
  flag is respected by this function.
*/
void cvar_t::sets_force(const string &value, bool force)
{
#if !NDEBUG
  if (has_flags(CVAR_HAS_CACHE)) {
    s_log_warning("Overwriting cached value for cvar %s", name_.c_str());
  }
#endif

  if (!(force || can_modify())) {
    return;
  } else if (owner_ && has_flags(CVAR_DELAYED)) {
    flags_ = (flags_ & ~CVAR_CACHED_MASK) | CVAR_CACHED_STRING;
    cache_ = value;
  } else {
    flags_ = (flags_ & ~CVAR_TYPE_MASK) | CVAR_STRING;
    int_value_ = std::atoi(value.c_str());
    float_value_ = std::atof(value.c_str());
    value_ = value;
  }

  if (owner_) {
    flags_ |= CVAR_MODIFIED;
    auto at_end = owner_->update_cvars_.cend();
    update_iter_ = owner_->update_cvars_.insert(at_end, this);
  }
}



/*!
  \brief Description
*/
void cvar_t::revoke_changes()
{
  if (has_flags(CVAR_HAS_CACHE)) {
    flags_ ^= CVAR_MODIFIED;
    cache_.clear();
    if (owner_) {
      owner_->update_cvars_.erase(update_iter_);
    }
  }
}



/*!
  \brief Description
*/
bool cvar_t::can_modify() const
{
  const char *namez = name_.c_str();

  if (flags_ & CVAR_READ_ONLY) {
    s_log_warning("CVar %s is read-only", namez);
    return false;
  }

  if (flags_ & CVAR_INIT_ONLY) {
    s_log_warning("CVar %s may only be set at program launch", namez);
    return false;
  }

  if (flags_ & CVAR_CHEAT) {
    if (owner_) {
      cvar_t *cheats = owner_->get_cvar(CHEATS_CVAR_NAME);
      if (cheats && !cheats->geti() >= 1) {
        s_log_warning("CVar %s may not be set if " CHEATS_CVAR_NAME
          " isn't set to >= 1", namez);
        return false;
      }
    }
  }

  return true;
}



/*!
  \brief Description
*/
void cvar_t::update()
{
  if (has_flags(CVAR_HAS_CACHE)) {
    int_value_ = std::atoi(cache_.c_str());
    float_value_ = std::atof(cache_.c_str());
    value_ = std::move(cache_);
    // remove modified flag and shift cached type into place
    flags_ = (flags_ & CVAR_CACHE_STRIP_MASK) |
      ((flags_ & CVAR_CACHED_MASK) << CVAR_TYPE_SHIFT);
  } else if (has_flags(CVAR_MODIFIED)) {
    flags_ ^= CVAR_MODIFIED;
  }
}



/*******************************************************************************
*                                    ccmd_t                                    *
*******************************************************************************/

ccmd_t::ccmd_t(const string &name, const ccmd_fn_t &fn) :
  name_(name),
  hash_(murmur3::hash32(name)),
  call_(fn)
{
  if (!fn) {
    s_throw(std::invalid_argument, "ccmd function cannot be nullptr");
  }
}



ccmd_t::ccmd_t(const string &name, ccmd_fn_t &&fn) :
  name_(name),
  hash_(murmur3::hash32(name)),
  call_(std::forward<ccmd_fn_t &&>(fn))
{
  if (!fn) {
    s_throw(std::invalid_argument, "ccmd function cannot be nullptr");
  }
}



uint32_t ccmd_t::name_hash() const
{
  return hash_;
}



const string &ccmd_t::name() const
{
  return name_;
}



void ccmd_t::call(const args_t &args)
{
  if (owner_) {
    call_(*owner_, args);
  }
}



void ccmd_t::call(const string &args_str)
{
  if (owner_) {
    args_t args;
    ccmd_arg_iters(args_str, args);
    call_(*owner_, args);
  }
}



size_t ccmd_t::ccmd_arg_iters(const string &str, args_t &out, size_t max)
{
  size_t count = 0;
  string::const_iterator end = str.cend();
  utf8::iterator<string::const_iterator> first { str.cbegin(), end };
  utf8::iterator<string::const_iterator> second { first, end };

  while (first != end) {
    uint32_t delim = 0;
    for (; first != end && (delim = *first) == ' '; ++first) {
      // nop
    }

    if (first == end || count == max) {
      break;
    }

    switch (delim) {
    case '"':
    case '\'': {
      second = ++first;
      ++second;
      bool escape = false;
      for (; second != end && *second != delim && !escape; ++second) {
        if (*second == '\\') {
          escape = true;
        } else {
          escape = false;
        }
      }

      out.push_back({ first, second });
      ++count;

      if (second != end) {
        ++second;
      }
    } break;

    default: {
      second = first;
      ++second;

      for (; second != end && *second != ' '; ++second) {
        // nop
      }
      out.push_back({ first, second });
      ++count;
    } break;
    } // switch

    first = second;
  } // while

  // For cases when count is reached before
  if (first != end) {
    out.push_back({ first, second });
    ++count;
  }

  return count;
}



/*******************************************************************************
*                                  cvar_set_t                                  *
*******************************************************************************/

cvar_set_t::cvar_set_t()
{
  #define CVAR_CAPACITY (2048)
  #define CCMD_CAPACITY (512)
  temp_cvars_.reserve(CVAR_CAPACITY);
}



void cvar_set_t::write_cvars(database_t &db)
{
  db.execute(create_cvar_table_string);
  db.execute("BEGIN TRANSACTION"); {
    auto update_query = db.prepare(cvar_update_string);
    for (const auto &kp : cvars_) {
      if (kp.second.kind != console_item_t::KIND_CVAR) {
        continue;
      }

      cvar_t *cvar = kp.second.cvar;
      if (cvar->has_flags(CVAR_SAVED)) {
        update_query.bind_text_static(":name", cvar->name());
        update_query.bind_text_static(":value", cvar->gets());
      }
    }
  }
  db.execute("END TRANSACTION");
}



void cvar_set_t::read_cvars(database_t &db)
{
  db.execute("BEGIN TRANSACTION"); {
    auto get_query = db.prepare(cvar_single_query_string);
    for (const auto &kp : cvars_) {
      if (kp.second.kind != console_item_t::KIND_CVAR) {
        continue;
      }

      cvar_t *cvar = kp.second.cvar;
      get_query.bind_text_static(":name", cvar->name());
      for (auto &r : get_query) {
        cvar->sets(r.column_text("value"));
        s_log_note("Loaded CVar %s -> %s",
          cvar->name().c_str(), cvar->gets().c_str());
      }
    }
  }
  db.execute("END TRANSACTION");
}



/*! Tries to find a cvar with the given name, returns it if it exists. Returns
nullptr otherwise. */
cvar_t *cvar_set_t::get_cvar(const string &name) const
{
  return get_cvar(murmur3::hash32(name));
}



/*! Tries to find a cvar with the given hash, returns it if it exists.
Otherwise, returns nullptr. */
cvar_t *cvar_set_t::get_cvar(uint32_t hash) const
{
  auto iter = cvars_.find(hash);
  if (iter != cvars_.end() && iter->second.kind == console_item_t::KIND_CVAR) {
    return iter->second.cvar;
  }
  return nullptr;
}



/*!
  \brief Tries to find a cvar with the given name, returns it if it exists.

  If it doesn't exist, a new cvar will be created with the provided
  default_value and default_flags and registered in the cvar set.

  \see cvar_t::cvar_t()
*/
cvar_t *cvar_set_t::get_cvar(const string &name, const string &default_value,
                             int default_flags)
{
  cvar_t *temp = get_cvar(name);
  if (!temp) {
    temp = make_cvar(name, default_value, default_flags);
    if (temp) {
      register_cvar(temp);
    }
  }
  return temp;
}



/*! \see get_cvar(const string &name, const string &default_value, int default_flags) */
cvar_t *cvar_set_t::get_cvar(const string &name, int default_value,
                             int default_flags)
{
  cvar_t *temp = get_cvar(name);
  if (!temp) {
    temp = make_cvar(name, default_value, default_flags);
    if (temp) {
      register_cvar(temp);
    }
  }
  return temp;
}



/*! \see get_cvar(const string &name, const string &default_value, int default_flags) */
cvar_t *cvar_set_t::get_cvar(const string &name, float default_value,
                             int default_flags)
{
  cvar_t *temp = get_cvar(name);
  if (!temp) {
    temp = make_cvar(name, default_value, default_flags);
    if (temp) {
      register_cvar(temp);
    }
  }
  return temp;
}



/*!
    \brief Allocates a cvar using an internal vector of cvars and returns it.

    This does not register the cvar in the cvar_set_t, though the pointer to
    the returned cvar will be invalid either after the cvar set's destruction
    or after clear() is called.

    \see cvar_t::cvar_t()
  */
cvar_t *cvar_set_t::make_cvar(const string &name, int value, unsigned flags)
{
  if (temp_cvars_.size() + 1 > temp_cvars_.capacity()) {
    s_log_error("Unable to allocate new cvar '%s': cvar storage is full",
      name.c_str());
    return nullptr;
  }
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.back();
}



/*! \see make_cvar(const string &, int, unsigned) */
cvar_t *cvar_set_t::make_cvar(const string &name, float value, unsigned flags)
{
  if (temp_cvars_.size() + 1 > temp_cvars_.capacity()) {
    s_log_error("Unable to allocate new cvar '%s': cvar storage is full",
      name.c_str());
    return nullptr;
  }
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.back();
}



/*! \see make_cvar(const string &, int, unsigned) */
cvar_t *cvar_set_t::make_cvar(const string &name, const string &value, unsigned flags)
{
  if (temp_cvars_.size() + 1 > temp_cvars_.capacity()) {
    s_log_error("Unable to allocate new cvar '%s': cvar storage is full",
      name.c_str());
    return nullptr;
  }
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.back();
}



/*!
  \brief Adds an existing cvar to the cvar set.

  This cvar should not be shared with other cvar sets. This does not copy the
  cvar, it keeps the address in its internal map.
*/
bool cvar_set_t::register_cvar(cvar_t *cvar)
{
  auto namehash = cvar->hash_;
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end()) {
    s_log_error("CVar %s is already registered to a different cvar",
      cvar->name().c_str());
    return false;
  }
  console_item_t item = {
    console_item_t::KIND_CVAR,
    { .cvar = cvar }
  };
  cvars_.insert({ namehash, item });
  cvar->owner_ = this;
  return true;
}



/*! \brief Removes an existing cvar from the cvar set. */
bool cvar_set_t::unregister_cvar(cvar_t *cvar)
{
  auto namehash = cvar->hash_;
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end() && iter->second.kind == console_item_t::KIND_CVAR) {
    iter->second.cvar->owner_ = nullptr;
    cvars_.erase(iter);
    return true;
  } else {
    s_log_error("CVar %s is not registered", cvar->name().c_str());
    return false;
  }
}



/*! \brief For all cvars with the CVAR_DELAYED flag, updates their values. */
void cvar_set_t::update_cvars()
{
  if (!update_cvars_.empty()) {
    for (cvar_t *cv : update_cvars_) {
      cv->update();
    }
    update_cvars_.clear();
  }
}



/*! \brief Adds a console command to the cvar set. Only useful for execute(). */
bool cvar_set_t::register_ccmd(ccmd_t *ccmd)
{
  auto namehash = ccmd->hash_;
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end()) {
    s_log_error("CCmd %s is already registered to a different cvar or ccmd",
      ccmd->name().c_str());
    return false;
  }
  console_item_t item = {
    console_item_t::KIND_CCMD,
    { .cmd = ccmd }
  };
  cvars_.insert({ namehash, item });
  ccmd->owner_ = this;
  return true;
}



/*! \brief Removes a console command from the cvar set. */
bool cvar_set_t::unregister_ccmd(ccmd_t *ccmd)
{
  auto namehash = ccmd->hash_;
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end() && iter->second.kind == console_item_t::KIND_CCMD) {
    iter->second.cmd->owner_ = nullptr;
    cvars_.erase(iter);
    return true;
  } else {
    s_log_error("%s is not a registered ccmd", ccmd->name().c_str());
    return false;
  }
}



/*! \brief Executes a space-separated command against the cvar set.

This function will do one of three things with a valid command:

1. If the first name in the string is a ccmd's name, it will call the ccmd
with any further words in the string as arguments, passing them as a list.
There may be no arguments.
2. If the string contains only a single name, no arguments, and the name is
that of a cvar, it will log the value of the cvar.
3. If the string is a name, has arguments, and the name is that of a cvar, it
will attempt to assign the first argument in the string (excluding the name)
to the cvar, keeping `force` checks in mind.

\param force Whether to force assignment of cvars in the case of a cvar
assignment. See cvar_t::seti_force for more information.
*/
void cvar_set_t::execute(const string &command, bool force)
{
  if (command.empty()) {
    return;
  }

  args_t args;
  size_t len = ccmd_t::ccmd_arg_iters(command, args);
  if (len == 0) {
    return;
  }
  arg_t namearg = args.front();
  args.pop_front();
  string name(namearg.first, namearg.second);
  --len;

  item_map_t::const_iterator iter = cvars_.find(murmur3::hash32(name));
  if (iter == cvars_.cend()) {
    cvar_is_invisible:
    s_log_error("No ccmd or cvar named %s", name.c_str());
    return;
  }

  const console_item_t &item = iter->second;

  switch (item.kind) {
  case console_item_t::KIND_CVAR:
    if (item.cvar->has_flags(CVAR_INVISIBLE)) {
      goto cvar_is_invisible;
    }

    if (len == 0) {
      s_log_note("%s = %s", name.c_str(), item.cvar->gets().c_str());
    } else {
      string value(args.front().first, args.back().second);
      s_log_note("%s = %s", name.c_str(), value.c_str());
      item.cvar->sets_force(value, force);
    }
    break;

  case console_item_t::KIND_CCMD:
    item.cmd->call(args);
    break;

  default:
    s_log_error("cvar or ccmd %s has invalid type", name.c_str());
    break;
  }
}



/*! Gets a console command with the given name. Returns it if found, nullptr otherwise. */
ccmd_t *cvar_set_t::get_ccmd(const string &name) const
{
  return get_ccmd(murmur3::hash32(name));
}



/*! Same as get_cmmd(const string &name), but takes a hash instead of the string name. */
ccmd_t *cvar_set_t::get_ccmd(uint32_t hash) const
{
  auto iter = cvars_.find(hash);
  if (iter != cvars_.end() && iter->second.kind == console_item_t::KIND_CCMD) {
    return iter->second.cmd;
  }
  return nullptr;
}



/*!
  \brief Calls the given console command, if it exists, with the given
  argument string.

  This is essentially a convenience function for getting the ccmd by name and
  calling it. Returns true if the ccmd was called (regardless of whether the
  ccmd was successful or not).
*/
bool cvar_set_t::call_ccmd(const string &name, const args_t &args)
{
  ccmd_t *cmd = get_ccmd(name);
  if (cmd) {
    cmd->call(args);
    return true;
  } else {
    return false;
  }
}



/*! Clears both cvars and ccmds from the set */
void cvar_set_t::clear()
{
  // force any cvars that haven't been updated to be.. updated.
  update_cvars();

  cvars_.clear();
  temp_cvars_.clear();
}



/*! Returns a const iterator to the beginning of the list of modified cvars */
auto cvar_set_t::modified_cbegin() const -> ptr_list_t::const_iterator
{
  return update_cvars_.cbegin();
}



/*! Returns a const iterator to after the end of the list of modified cvars */
auto cvar_set_t::modified_cend() const -> ptr_list_t::const_iterator
{
  return update_cvars_.cend();
}


} // namespace snow
