#include "console.hh"
#include <cstdlib>


namespace snow {


namespace {


const string create_cvar_table_string {
  "CREATE IF NOT EXISTS console_variables (name AS TEXT UNIQUE OR REPLACE, value AS TEXT)"
};


const string cvar_single_query_string {
  "SELECT value FROM console_variables WHERE name = :name LIMIT 1"
};


const string cvar_update_string {
  "INSERT OR REPLACE INTO CONSOLE_VARIABLES (:name, :value)"
};



const string cvar_default_value { };


} // namespace <anon>

/*******************************************************************************
*                                    cvar_t                                    *
*******************************************************************************/

cvar_t::cvar_t(const string &name, int value, int flags) :
  owner_(nullptr), flags_(flags&~CVAR_DELAYED), name_(name), cache_()
{
  setl_force(value, true);
  // if delayed, no longer delayed, also strip modified flag if someone was
  // dumb enough to pass that in.
  flags_ = flags&~CVAR_MODIFIED;
}



cvar_t::cvar_t(const string &name, float value, int flags) :
  owner_(nullptr), flags_(flags&~CVAR_DELAYED), name_(name), cache_()
{
  setf_force(value, true);
  flags_ = flags&~CVAR_MODIFIED;
}



cvar_t::cvar_t(const string &name, const string &value, int flags) :
  owner_(nullptr), flags_(flags&~CVAR_DELAYED), name_(name), cache_()
{
  sets_force(value, true);
  flags_ = flags&~CVAR_MODIFIED;
}



const string &cvar_t::name() const
{
  return name_;
}



int cvar_t::flags() const
{
  return flags_;
}



bool cvar_t::has_flags(int flags) const
{
  return (flags_ & flags) == flags;
}



int cvar_t::getl() const
{
  return int_value_;
}



float cvar_t::getf() const
{
  return float_value_;
}



const string &cvar_t::gets() const
{
  return value_;
}



void cvar_t::setl(int value)
{
  setl_force(value, true);
}



void cvar_t::setf(float value)
{
  setf_force(value, true);
}



void cvar_t::sets(const string &value)
{
  sets_force(value, true);
}



void cvar_t::setl_force(int value, bool force)
{
  if (!(force || can_modify())) {
    return;
  } else if (has_flags(CVAR_DELAYED)) {
    cache_ = std::to_string(value);
  } else {
    int_value_ = value;
    float_value_ = (float)value;
    value_ = std::to_string(value);
  }
}



void cvar_t::setf_force(float value, bool force)
{
  if (!(force || can_modify())) {
    return;
  } else if (has_flags(CVAR_DELAYED)) {
    cache_ = std::to_string(value);
  } else {
    int_value_ = (int)value;
    float_value_ = value;
    value_ = std::to_string(value);
  }
}



void cvar_t::sets_force(const string &value, bool force)
{
  if (!(force || can_modify())) {
    return;
  } else if (has_flags(CVAR_DELAYED)) {
    cache_ = value;
  } else {
    int_value_ = std::atoi(value.c_str());
    float_value_ = std::atof(value.c_str());
    value_ = value;
  }
}



void cvar_t::revoke_changes()
{
  if (has_flags(CVAR_DELAYED|CVAR_MODIFIED)) {
    flags_ &= ~CVAR_MODIFIED;
    cache_.clear();
    owner_->update_cvars_.erase(update_iter_);
  }
}



bool cvar_t::can_modify() const
{
  const char *namez = name_.c_str();

  if (flags_ & CVAR_READ_ONLY) {
    s_log_note("CVar %s is read-only", namez);
    return false;
  }

  if (flags_ & CVAR_INIT_ONLY) {
    s_log_note("CVar %s may only be set at program launch", namez);
    return false;
  }

  if (flags_ & CVAR_CHEAT) {
    if (owner_) {
      cvar_t *cheats = owner_->get_cvar(CHEATS_CVAR_NAME);
      if (cheats && !cheats->getl() >= 1) {
        s_log_note("CVar %s may not be set if " CHEATS_CVAR_NAME
          " isn't set to >= 1", namez);
        return false;
      }
    }
  }

  return true;
}



void cvar_t::update()
{
  if (has_flags(CVAR_MODIFIED | CVAR_DELAYED)) {
    int_value_ = std::atoi(cache_.c_str());
    float_value_ = std::atof(cache_.c_str());
    value_ = std::move(cache_);
  }
}



/*******************************************************************************
*                                  cvar_set_t                                  *
*******************************************************************************/



void cvar_set_t::write_cvars(database_t &db)
{
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
  auto iter = cvars_.find(name);
  if (iter != cvars_.end()) {
    return iter->second;
  }
  return nullptr;
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
  auto iter = cvars_.find(cvar->name());
  if (iter != cvars_.end()) {
    s_log_error("CVar %s is already registered to a different cvar",
      cvar->name().c_str());
    return false;
  }
  cvars_.insert({ cvar->name(), cvar });
  return true;
}



bool cvar_set_t::unregister_cvar(cvar_t *cvar)
{
  auto iter = cvars_.find(cvar->name());
  if (iter != cvars_.end()) {
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
