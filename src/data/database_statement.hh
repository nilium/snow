/*
  database_statement.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__DATABASE_STATEMENT_HH__
#define __SNOW__DATABASE_STATEMENT_HH__

#include "database_local.hh"
#include "database_result.hh"
#include "database_iterator.hh"
#include <iterator>
#include <map>


namespace snow {


struct dbiterator_t;


struct dbstatement_t
{
  using free_fn_t = void (*)(void *);
  using iterator = dbiterator_t;

  dbstatement_t(dbstatement_t &&);
  ~dbstatement_t();

  dbstatement_t(const dbstatement_t &) = delete;
  dbstatement_t &operator =  (dbstatement_t &&) = delete;
  dbstatement_t &operator =  (const dbstatement_t &) = delete;

  inline bool is_finalized() const { return stmt_ == NULL; }

  bool finalize();

  int execute();
  int execute(const dbresult_t::result_fn_t &fn);
  inline int execute(std::nullptr_t np) { return execute(); }

  iterator begin();
  iterator end();

  int num_columns() const;
  const string &column_name(int index) const;
  int column_index(const string &name) const;

  // Untyped bindings (indexed)
  int bind(int index, int v);
  int bind(int index, int64_t v);
  int bind(int index, float v);
  int bind(int index, double v);
  int bind(int index, const string &v);
  int bind(int index, std::nullptr_t v);

  // Unntyped bindings (named)
  int bind(const string &name, int v);
  int bind(const string &name, int64_t v);
  int bind(const string &name, float v);
  int bind(const string &name, double v);
  int bind(const string &name, const string &v);
  int bind(const string &name, std::nullptr_t v);

  // Index bindings
  int bind_null(int index);
  int bind_int(int index, int);
  int bind_int64(int index, int64_t);
  int bind_uint(int index, int);
  int bind_uint64(int index, int64_t);
  int bind_double(int index, double);
  int bind_text(int index, const char *str, int byte_size, free_fn_t freefn);
  int bind_text16(int index, const void *str16, int byte_size, free_fn_t freefn);
  int bind_blob(int index, const void *blob, int byte_size, free_fn_t freefn);

  // Named bindings (same as bind_*(binding_index(name), ...))
  int bind_null(const string &name);
  int bind_int(const string &name, int val);
  int bind_int64(const string &name, int64_t val);
  int bind_double(const string &name, double dbl);
  int bind_text(const string &name, const char *str, int byte_size, free_fn_t freefn);
  int bind_text16(const string &name, const void *str16, int byte_size, free_fn_t freefn);
  int bind_blob(const string &name, const void *blob, int byte_size, free_fn_t freefn);

  // Convenience functions
  int bind_float(int index, float flt);
  int bind_text_copy(int index, const string &str);
  int bind_text_static(int index, const string &str);
  int bind_float(const string &name, float flt);
  int bind_text_copy(const string &name, const string &str);
  int bind_text_static(const string &name, const string &str);


  int binding_index(const string &name);
  int clear_bindings();


private:
  friend struct database_t;
  friend struct dbresult_t;
  friend struct dbiterator_t;

  dbstatement_t(database_t &db, const string &sql);
  void finalize_nothrow();

  int reset();
  int step();

  bool end_;
  database_t &db_;
  sqlite3_stmt *stmt_;
  int num_columns_;
  std::map<string, int> bind_vars_;
  std::map<string, int> column_names_;
  int sequence_;
};


} // namespace snow

#endif /* end __SNOW__DATABASE_STATEMENT_HH__ include guard */
