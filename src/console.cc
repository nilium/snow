#include "console.hh"
#include <cstdlib>
#include <snow/data/hash.hh>


namespace snow {


namespace {


const string create_cvar_table_string {
  "CREATE IF NOT EXISTS console_variables "
  "(name AS TEXT UNIQUE OR REPLACE, value AS TEXT)"
};


const string cvar_single_query_string {
  "SELECT value FROM console_variables WHERE name = :name LIMIT 1"
};


const string cvar_update_string {
  "INSERT OR REPLACE INTO console_variables (:name, :value)"
};


} // namespace <anon>

/*******************************************************************************
*                                    cvar_t                                    *
*******************************************************************************/

/*==============================================================================
  cvar_t(const string &name, int value, int flags) :

    Constructs a cvar with an initial integer value.
==============================================================================*/
cvar_t::cvar_t(const string &name, int value, int flags) :
  owner_(nullptr), flags_(flags & ~CVAR_DELAYED), name_(name), cache_()
{
  seti_force(value, true);
  // if delayed, no longer delayed, also strip modified flag if someone was
  // dumb enough to pass that in.
  flags_ = flags & ~CVAR_MODIFIED;
}



/*==============================================================================
  cvar_t(const string &name, float value, int flags) :

    Constructs a cvar with an initial float value.
==============================================================================*/
cvar_t::cvar_t(const string &name, float value, int flags) :
  owner_(nullptr), flags_(flags&~CVAR_DELAYED), name_(name), cache_()
{
  setf_force(value, true);
  flags_ = flags & ~CVAR_MODIFIED;
}



/*==============================================================================
  cvar_t(const string &name, const string &value, int flags) :

    Constructs a cvar with an initial string value.
==============================================================================*/
cvar_t::cvar_t(const string &name, const string &value, int flags) :
  owner_(nullptr), flags_(flags&~CVAR_DELAYED), name_(name), cache_()
{
  sets_force(value, true);
  flags_ = flags & ~CVAR_MODIFIED;
}



/*==============================================================================
  name() const

    Gets the name of the cvar.
==============================================================================*/
const string &cvar_t::name() const
{
  return name_;
}



/*==============================================================================
  flags() const

    Gets any flags associated with the cvar.
==============================================================================*/
int cvar_t::flags() const
{
  return flags_;
}



/*==============================================================================
  has_flags(int flags) const

    Tests if the cvar has a flag or a specific combination of flags.
==============================================================================*/
bool cvar_t::has_flags(int flags) const
{
  return (flags_ & flags) == flags;
}



/*==============================================================================
  type() const

    Returns the type of the cvar.
==============================================================================*/
int cvar_t::type() const
{
  return flags_ & CVAR_TYPE_MASK;
}



/*==============================================================================
  geti() const

    Gets the float value of the cvar. If a string that doesn't represent any
    int value, the result will be zero.
==============================================================================*/
int cvar_t::geti() const
{
  return int_value_;
}



/*==============================================================================
  getf() const

    Gets the float value of the cvar. If a string that doesn't represent any
    float value, the result will be zero.
==============================================================================*/
float cvar_t::getf() const
{
  return float_value_;
}



/*==============================================================================
  gets() const

    Gets the string value of the cvar. If the cvar is of a numeric type, it will
    return a string version of that value.
==============================================================================*/
const string &cvar_t::gets() const
{
  return value_;
}



/*==============================================================================
  seti(int value)

    Sets the cvar to an int value. Skips restriction flags.
==============================================================================*/
void cvar_t::seti(int value)
{
  seti_force(value, true);
}



/*==============================================================================
  setf(float value)

    Sets the cvar to a float value. Skips restriction flags.
==============================================================================*/
void cvar_t::setf(float value)
{
  setf_force(value, true);
}



/*==============================================================================
  sets(const string &value)

    Sets the cvar to a string value. Skips restriction flags.
==============================================================================*/
void cvar_t::sets(const string &value)
{
  sets_force(value, true);
}



/*==============================================================================
  seti_force(int value, bool force)

    Description
==============================================================================*/
void cvar_t::seti_force(int value, bool force)
{
#ifndef NDEBUG
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



/*==============================================================================
  setf_force(float value, bool force)

    Description
==============================================================================*/
void cvar_t::setf_force(float value, bool force)
{
#ifndef NDEBUG
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



/*==============================================================================
  sets_force(const string &value, bool force)

    Description
==============================================================================*/
void cvar_t::sets_force(const string &value, bool force)
{
#ifndef NDEBUG
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



/*==============================================================================
  revoke_changes()

    Description
==============================================================================*/
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



/*==============================================================================
  can_modify() const

    Description
==============================================================================*/
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



/*==============================================================================
  update()

    Description
==============================================================================*/
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
*                                  cvar_set_t                                  *
*******************************************************************************/



void cvar_set_t::write_cvars(database_t &db)
{
  db.execute(create_cvar_table_string);
  db.execute("BEGIN TRANSACTION"); {
    auto update_query = db.prepare(cvar_update_string);
    for (const auto &kp : cvars_) {
      cvar_t *cvar = kp.second;
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
      cvar_t *cvar = kp.second;
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



cvar_t *cvar_set_t::get_cvar(const string &name) const
{
  auto namehash = hash32(name);
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end()) {
    return iter->second;
  }
  return nullptr;
}



cvar_t *cvar_set_t::get_cvar(const string &name, const string &default_value,
                             int default_flags)
{
  cvar_t *temp = get_cvar(name);
  if (!temp) {
    temp = make_cvar(name, default_value, default_flags);
    register_cvar(temp);
  }
  return temp;
}



cvar_t *cvar_set_t::make_cvar(const string &name, int value, int flags)
{
  int index = temp_cvars_.size();
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.at(index);
}



cvar_t *cvar_set_t::make_cvar(const string &name, float value, int flags)
{
  int index = temp_cvars_.size();
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.at(index);
}



cvar_t *cvar_set_t::make_cvar(const string &name, const string &value, int flags)
{
  int index = temp_cvars_.size();
  temp_cvars_.emplace_back(name, value, flags);
  return &temp_cvars_.at(index);
}



bool cvar_set_t::register_cvar(cvar_t *cvar)
{
  auto namehash = hash32(cvar->name());
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end()) {
    s_log_error("CVar %s is already registered to a different cvar",
      cvar->name().c_str());
    return false;
  }
  cvars_.insert({ namehash, cvar });
  cvar->owner_ = this;
  return true;
}



bool cvar_set_t::unregister_cvar(cvar_t *cvar)
{
  auto namehash = hash32(cvar->name());
  auto iter = cvars_.find(namehash);
  if (iter != cvars_.end()) {
    iter->second->owner_ = nullptr;
    cvars_.erase(iter);
    return true;
  } else {
    s_log_error("CVar %s is not registered", cvar->name().c_str());
    return false;
  }
}



void cvar_set_t::update_cvars()
{
  if (!update_cvars_.empty()) {
    for (cvar_t *cv : update_cvars_) {
      cv->update();
    }
    update_cvars_.clear();
  }
}



void cvar_set_t::clear()
{
  // force any cvars that haven't been updated to be.. updated.
  update_cvars();

  cvars_.clear();
  temp_cvars_.clear();
}



auto cvar_set_t::modified_cbegin() const -> ptr_list_t::const_iterator
{
  return update_cvars_.cbegin();
}



auto cvar_set_t::modified_cend() const -> ptr_list_t::const_iterator
{
  return update_cvars_.cend();
}


} // namespace snow
