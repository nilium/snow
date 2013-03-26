#ifndef __SNOW_CVAR_HH__
#define __SNOW_CVAR_HH__

#include <iostream>
#include <memory>
#include "snow-config.hh"

namespace snow {

enum cvar_kind_t : int
{
  CVAR_KIND_INT    = 0,
  CVAR_KIND_DOUBLE = 1,
  CVAR_KIND_STRING = 2
};

enum cvar_flags_t : int
{
  CVAR_PERSISTED = 0x1 << 0,
  CVAR_READONLY = 0x1 << 1,
  CVAR_SYSTEM = 0x1 << 2,
};

class S_EXPORT cvar_base_t
{
  int flags_;
  string name_;

public:
  cvar_base_t(string name, int flags);
  virtual ~cvar_base_t() = default;

  inline auto flags() const -> int
  {
    return flags_;
  }

  inline auto name()  const -> const string&
  {
    return name_;
  }

  // In the case that converting a string cvar to an int or double fails, the
  // <type>_value function will return 0.
  virtual auto int_value()    const -> int       = 0;
  virtual auto double_value() const -> double    = 0;
  virtual auto string_value() const -> string    = 0;

  virtual void store_int(int value)              = 0;
  virtual void store_double(double value)        = 0;
  virtual void store_string(const string &value) = 0;

  virtual auto kind() const -> cvar_kind_t       = 0;
};

using cvar_t = std::shared_ptr<cvar_base_t>;

S_EXPORT cvar_t make_cvar(string name, int value, int flags);
S_EXPORT cvar_t make_cvar(string name, double value, int flags);
S_EXPORT cvar_t make_cvar(string name, const string &value, int flags);
S_EXPORT cvar_t &convert_cvar(cvar_t &&cvar, cvar_kind_t to_kind);

S_EXPORT std::ostream &operator << (std::ostream &out, const cvar_base_t *in);

} // namespace snow

#endif /* end __SNOW_CVAR_HH__ include guard */
