#include "cvar.hh"

namespace snow {

cvar_base_t::cvar_base_t(string name, int flags) :
  flags_(flags), name_(name)
{}

template <typename T>
struct S_EXPORT cvar_typed_t : public cvar_base_t
{
  typedef T value_type;
  static_assert(std::is_same<value_type, int>::value |
                std::is_same<value_type, double>::value,
                "cvar_typed_t<T> must be of type string, int, or double");

protected:
  value_type value_;

public:
  cvar_typed_t(string name, value_type value, int flags) :
    cvar_base_t(name, flags), value_(value)
  {}

  virtual ~cvar_typed_t() = default;

  virtual auto int_value() const -> int
  {
    if (std::is_same<value_type, int>::value)
      return value_;
    else
      return static_cast<int>(value_);
  }

  virtual auto double_value() const -> double
  {
    if (std::is_same<value_type, double>::value)
      return value_;
    else
      return static_cast<double>(value_);
  }

  virtual auto string_value() const -> string
  {
    return std::to_string(value_);
  }

  virtual void store_int(int value)
  {
    value_ = value;
  }

  virtual void store_double(double value)
  {
    value_ = value;
  }

  virtual void store_string(const string &value)
  {
    try {
      if (std::is_same<value_type, double>::value)
        value_ = stod(value);
      else
        value_ = stoi(value);
    } catch (std::exception ex) {
      value_ = 0;
    }
  }

  virtual auto kind() const -> cvar_kind_t
  {
    return (cvar_kind_t)std::is_same<value_type, double>::value;
  }

};

template <>
struct S_EXPORT cvar_typed_t<string> : public cvar_base_t
{
  typedef string value_type;

protected:
  value_type value_;

public:
  cvar_typed_t(string name, value_type value, int flags) :
    cvar_base_t(name, flags), value_(value)
  {}

  virtual ~cvar_typed_t() = default;

  virtual auto int_value() const -> int
  {
    try {
      return std::stoi(value_);
    } catch(std::exception ex) {
      return 0;
    }
  }

  virtual auto double_value() const -> double
  {
    try {
      return std::stod(value_);
    } catch(std::exception ex) {
      return 0;
    }
  }

  virtual auto string_value() const -> string
  {
    return value_;
  }

  virtual void store_int(int value)
  {
    value_ = std::to_string(value);
  }

  virtual void store_double(double value)
  {
    value_ = std::to_string(value);
  }

  virtual void store_string(const string &value)
  {
    value_ = value;
  }

  virtual auto kind() const -> cvar_kind_t
  {
    return CVAR_KIND_STRING;
  }
};

cvar_t make_cvar_int(string name, int value, int flags)
{
  return std::make_shared<cvar_typed_t<int> >(name, value, flags);
}

cvar_t make_cvar_double(string name, double value, int flags)
{
  return std::make_shared<cvar_typed_t<double> >(name, value, flags);
}

cvar_t make_cvar_string(string name, const string &value, int flags)
{
  return std::make_shared<cvar_typed_t<string> >(name, value, flags);
}

cvar_t &convert_cvar(cvar_t &&cvar, cvar_kind_t to_kind)
{
  if (cvar->kind() == to_kind)
    return cvar;

  switch (to_kind) {
  case CVAR_KIND_STRING:
    cvar = std::make_shared<cvar_typed_t<string> >(cvar->name(), cvar->string_value(), cvar->flags());
    break;
  case CVAR_KIND_INT:
    cvar = std::make_shared<cvar_typed_t<int> >(cvar->name(), cvar->int_value(), cvar->flags());
    break;
  case CVAR_KIND_DOUBLE:
    cvar = std::make_shared<cvar_typed_t<double> >(cvar->name(), cvar->double_value(), cvar->flags());
    break;
  }
  return cvar;
}

std::ostream &operator << (std::ostream &out, const cvar_base_t *in)
{
  out << "{ name: " << in->name() << ", value: ";
  switch (in->kind()) {
  case CVAR_KIND_INT:    out << in->int_value();    break;
  case CVAR_KIND_DOUBLE: out << in->double_value(); break;
  case CVAR_KIND_STRING: out << in->string_value(); break;
  }
  return (out << " }");
}

} // namespace snow
