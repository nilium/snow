#include "database_statement.hh"
#include "database.hh"


#define throw_if_finalized(LIT) \
  if (is_finalized()) throw std::runtime_error((LIT));


namespace snow {


namespace {


// Empty string returned when no name is found for a given binding index
const string g_empty_string { "" };


} // namespace <anon>



/*******************************************************************************
*                                 Ctor / Dtor                                  *
*******************************************************************************/

dbstatement_t::dbstatement_t(database_t &db, const string &sql) :
  db_(db),
  stmt_(NULL),
  num_columns_(0),
  bind_vars_(),
  column_names_()
{
  const char *zsql = sql.c_str();
  int num_bytes = sql.size();
  int code = sqlite3_prepare_v2(db.db_, zsql, num_bytes, &stmt_, NULL);
  db_.check_error(code);
  if (code == SQLITE_OK) {
    const int columns = sqlite3_column_count(stmt_);
    num_columns_ = columns;
    for (int index = 0; index < columns; ++index) {
      string name(sqlite3_column_name(stmt_, index));
      column_names_.insert({ std::move(name), index });
    }
  }
}



dbstatement_t::dbstatement_t(dbstatement_t &&other) :
  db_(other.db_),
  stmt_(other.stmt_),
  num_columns_(other.num_columns_),
  bind_vars_(std::move(other.bind_vars_)),
  column_names_(std::move(other.column_names_))
{
  other.stmt_ = NULL;
}



dbstatement_t::~dbstatement_t()
{
  finalize_nothrow();
}



/*******************************************************************************
*                                 Finalization                                 *
*******************************************************************************/

bool dbstatement_t::finalize()
{
  finalize_nothrow();
  const int code = db_.check_error(db_.error_);
  return !dbis_error_code(code);
}



void dbstatement_t::finalize_nothrow()
{
  if (stmt_ != NULL) {
    int code = sqlite3_finalize(stmt_);
    if (db_.check_error_nothrow(code) == SQLITE_OK) {
      stmt_ = NULL;
      num_columns_ = 0;
      bind_vars_.clear();
      column_names_.clear();
    }
  }
}



/*******************************************************************************
*                                  Execution                                   *
*******************************************************************************/

int dbstatement_t::execute()
{
  throw_if_finalized("Cannot execute statement: statement is finalized");

  reset();
  int code = SQLITE_OK;
  do {
    code = step();
  } while (code == SQLITE_ROW);

  return code;
}



int dbstatement_t::execute(const dbresult_t::result_fn_t &fn)
{
  throw_if_finalized("Cannot execute statement: statement is finalized");

  reset();
  int code = SQLITE_OK;
  dbresult_t result(*this);
  do {
    step();
    if (code == SQLITE_ROW && fn) {
      fn(result);
    }
  } while (code == SQLITE_ROW);

  return code;
}



/*******************************************************************************
*                              Column information                              *
*******************************************************************************/

int dbstatement_t::num_columns() const
{
  throw_if_finalized("Cannot get number of columns: statement is finalized");
  return num_columns_;
}



const string &dbstatement_t::column_name(int index) const
{
  throw_if_finalized("Cannot get column name: statement is finalized");
  for (const auto &kp : column_names_) {
    if (kp.second == index)
      return kp.first;
  }
  return g_empty_string;
}



int dbstatement_t::column_index(const string &name) const
{
  throw_if_finalized("Cannot get column index: statement is finalized");
  auto iter = column_names_.find(name);
  if (iter != column_names_.end())
    return iter->second;
  else
    return -1;
}



/*******************************************************************************
*                          Untyped bindings - indexed                          *
*******************************************************************************/

int dbstatement_t::bind(int index, int v)
{
  return bind_int(index, v);
}



int dbstatement_t::bind(int index, int64_t v)
{
  return bind_int64(index, v);
}



int dbstatement_t::bind(int index, float v)
{
  return bind_float(index, v);
}



int dbstatement_t::bind(int index, double v)
{
  return bind_double(index, v);
}



int dbstatement_t::bind(int index, const string &v)
{
  return bind_text_copy(index, v);
}



int dbstatement_t::bind(int index, std::nullptr_t v)
{
  (void)v;
  return bind_null(index);
}



/*******************************************************************************
*                           Untyped bindings - named                           *
*******************************************************************************/

int dbstatement_t::bind(const string &name, int v)
{
  return bind_int(name, v);
}



int dbstatement_t::bind(const string &name, int64_t v)
{
  return bind_int64(name, v);
}



int dbstatement_t::bind(const string &name, float v)
{
  return bind_float(name, v);
}



int dbstatement_t::bind(const string &name, double v)
{
  return bind_double(name, v);
}



int dbstatement_t::bind(const string &name, const string &v)
{
  return bind_text_copy(name, v);
}



int dbstatement_t::bind(const string &name, std::nullptr_t v)
{
  (void)v;
  return bind_null(name);
}



/*******************************************************************************
*                           Typed bindings - indexed                           *
*******************************************************************************/

int dbstatement_t::bind_null(int index)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_null(stmt_, index));
}



int dbstatement_t::bind_int(int index, int val)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_int(stmt_, index, val));
}



int dbstatement_t::bind_int64(int index, int64_t val)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_int64(stmt_, index, (sqlite3_int64)val));
}



int dbstatement_t::bind_double(int index, double dbl)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_double(stmt_, index, dbl));
}



int dbstatement_t::bind_text(int index, const char *str, int byte_size, free_fn_t freefn)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_text(stmt_, index, str, byte_size, freefn));
}



int dbstatement_t::bind_text16(int index, const void *str16, int byte_size, free_fn_t freefn)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_text16(stmt_, index, str16, byte_size, freefn));
}



int dbstatement_t::bind_blob(int index, const void *blob, int byte_size, free_fn_t freefn)
{
  throw_if_finalized("Cannot bind value: statement is finalized");
  return db_.check_error(sqlite3_bind_blob(stmt_, index, blob, byte_size, freefn));
}



/*******************************************************************************
*                            Typed bindings - named                            *
*******************************************************************************/

int dbstatement_t::bind_null(const string &name)
{
  return bind_null(binding_index(name));
}



int dbstatement_t::bind_int(const string &name, int val)
{
  return bind_int(binding_index(name), val);
}



int dbstatement_t::bind_int64(const string &name, int64_t val)
{
  return bind_int64(binding_index(name), val);
}



int dbstatement_t::bind_double(const string &name, double dbl)
{
  return bind_double(binding_index(name), dbl);
}



int dbstatement_t::bind_text(const string &name, const char *str, int byte_size, free_fn_t freefn)
{
  return bind_text(binding_index(name), str, byte_size, freefn);
}



int dbstatement_t::bind_text16(const string &name, const void *str16, int byte_size, free_fn_t freefn)
{
  return bind_text16(binding_index(name), str16, byte_size, freefn);
}



int dbstatement_t::bind_blob(const string &name, const void *blob, int byte_size, free_fn_t freefn)
{
  return bind_blob(binding_index(name), blob, byte_size, freefn);
}



/*******************************************************************************
*                   Binding convenience functions - indexed                    *
*******************************************************************************/

int dbstatement_t::bind_float(int index, float flt)
{
  return bind_double(index, (double)flt);
}



int dbstatement_t::bind_text_copy(int index, const string &str)
{
  return bind_text(index, str.c_str(), str.size(), (free_fn_t)SQLITE_TRANSIENT);
}



int dbstatement_t::bind_text_static(int index, const string &str)
{
  return bind_text(index, str.c_str(), str.size(), (free_fn_t)SQLITE_STATIC);
}



/*******************************************************************************
*                    Binding convenience functions - named                     *
*******************************************************************************/

int dbstatement_t::bind_float(const string &name, float flt)
{
  return bind_double(name, (double)flt);
}



int dbstatement_t::bind_text_copy(const string &name, const string &str)
{
  return bind_text(name, str.c_str(), str.size(), (free_fn_t)SQLITE_TRANSIENT);
}



int dbstatement_t::bind_text_static(const string &name, const string &str)
{
  return bind_text(name, str.c_str(), str.size(), (free_fn_t)SQLITE_STATIC);
}



/*******************************************************************************
*                             Binding information                              *
*******************************************************************************/

int dbstatement_t::binding_index(const string &name)
{
  throw_if_finalized("Cannot get named binding index: statement is finalized");
  const auto iter = bind_vars_.find(name);
  int index = 0;
  if (iter == bind_vars_.end()) {
    const char *const zstr = name.c_str();
    index = sqlite3_bind_parameter_index(stmt_, zstr);
    if (index == 0) {
      throw std::invalid_argument("Parameter name does not exist in prepared statement");
    }
    bind_vars_.insert({name, index});
  } else {
    index = iter->second;
  }
  return index;
}



/*******************************************************************************
*                                Clear bindings                                *
*******************************************************************************/

int dbstatement_t::clear_bindings()
{
  throw_if_finalized("Cannot clear bindings: statement is finalized");
  return db_.check_error(sqlite3_clear_bindings(stmt_));
}



/*******************************************************************************
*                            Execution step / reset                            *
*******************************************************************************/

int dbstatement_t::step()
{
  int code = sqlite3_step(stmt_);
  return db_.check_error(code);
}



int dbstatement_t::reset()
{
  ++sequence_;
  int code = sqlite3_reset(stmt_);
  return db_.check_error(code);
}



/*******************************************************************************
*                                   Iterator                                   *
*******************************************************************************/

auto dbstatement_t::begin() -> iterator
{
  reset();
  return iterator(*this); // calls reset() and step()
}



auto dbstatement_t::end() -> iterator
{
  return iterator(*this, SQLITE_DONE);
}


} // namespace snow
