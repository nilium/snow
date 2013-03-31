#ifndef __SNOW__DATABASE_STATEMENT_HH__
#define __SNOW__DATABASE_STATEMENT_HH__

#include "database_local.hh"
#include "database_result.hh"
#include <map>


namespace snow {


struct dbstatement_t
{
  using free_fn_t = void (*)(void *);


  dbstatement_t(dbstatement_t &&);
  ~dbstatement_t();

  dbstatement_t() = delete;
  dbstatement_t(const dbstatement_t &) = delete;
  dbstatement_t &operator =  (dbstatement_t &&) = delete;
  dbstatement_t &operator =  (const dbstatement_t &) = delete;

  inline bool is_finalized() const { return stmt_ == NULL; }

  bool finalize();

  int execute();
  int execute(const dbresult_t::result_fn_t &fn);
  inline int execute(std::nullptr_t np) { return execute(); }

  // Index bindings
  int bind_null(int index);
  int bind_int(int index, int);
  int bind_int64(int index, int64_t);
  int bind_double(int index, double);
  int bind_text(int index, const char *str, int byte_size, free_fn_t freefn);
  int bind_text16(int index, const void *str16, int byte_size, free_fn_t freefn);
  int bind_blob(int index, const void *blob, int byte_size, free_fn_t freefn);

  // Named bindings
  inline int bind_null(const string &name)
  {
    return bind_null(binding_index(name));
  }
  inline int bind_int(const string &name, int val)
  {
    return bind_int(binding_index(name), val);
  }
  inline int bind_int64(const string &name, int64_t val)
  {
    return bind_int64(binding_index(name), val);
  }
  inline int bind_double(const string &name, double dbl)
  {
    return bind_double(binding_index(name), dbl);
  }
  inline int bind_text(const string &name, const char *str, int byte_size, free_fn_t freefn)
  {
    return bind_text(binding_index(name), str, byte_size, freefn);
  }
  inline int bind_text16(const string &name, const void *str16, int byte_size, free_fn_t freefn)
  {
    return bind_text16(binding_index(name), str16, byte_size, freefn);
  }
  inline int bind_blob(const string &name, const void *blob, int byte_size, free_fn_t freefn)
  {
    return bind_blob(binding_index(name), blob, byte_size, freefn);
  }

  inline int bind_float(int index, float flt)
  {
    return bind_double(index, (double)flt);
  }

  inline int bind_text_copy(int index, const string &str)
  {
    return bind_text(index, str.c_str(), str.size(), (free_fn_t)SQLITE_TRANSIENT);
  }

  inline int bind_text_static(int index, const string &str)
  {
    return bind_text(index, str.c_str(), str.size(), (free_fn_t)SQLITE_STATIC);
  }

  inline int bind_float(const string &name, float flt)
  {
    return bind_double(name, (double)flt);
  }

  inline int bind_text(const string &name, const string &str)
  {
    return bind_text(name, str.c_str(), str.size(), (free_fn_t)SQLITE_TRANSIENT);
  }


  int binding_index(const string &name);
  int clear_bindings();


private:
  friend struct database_t;
  friend struct dbresult_t;

  dbstatement_t(database_t &db, const string &sql);
  void finalize_nothrow();

  database_t &db_;
  sqlite3_stmt *stmt_;
  std::map<string, int> bind_vars_;
};


} // namespace snow

#endif /* end __SNOW__DATABASE_STATEMENT_HH__ include guard */
