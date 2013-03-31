#include "database_statement.hh"
#include "database.hh"


#define throw_if_finalized(LIT) \
  if (is_finalized()) throw std::runtime_error((LIT));


namespace snow {


dbstatement_t::dbstatement_t(database_t &db, const string &sql) :
  db_(db),
  stmt_(NULL),
  bind_vars_()
{
  const char *zsql = sql.c_str();
  int num_bytes = sql.size();
  int code = sqlite3_prepare_v2(db.db_, zsql, num_bytes, &stmt_, NULL);
  db_.check_error(code);
}



dbstatement_t::dbstatement_t(dbstatement_t &&other) :
  db_(other.db_),
  stmt_(other.stmt_),
  bind_vars_(std::move(other.bind_vars_))
{
  other.stmt_ = NULL;
}



dbstatement_t::~dbstatement_t()
{
  finalize_nothrow();
}



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
      bind_vars_.clear();
    }
  }
}



int dbstatement_t::execute()
{
  throw_if_finalized("Cannot execute statement: statement is finalized");

  int code = SQLITE_OK;
  do {
    code = sqlite3_step(stmt_);
    if (dbis_error_code(db_.check_error(code))) {
      return code;
    }
  } while (code == SQLITE_ROW);

  code = sqlite3_reset(stmt_);
  db_.check_error(code);

  return code;
}



int dbstatement_t::execute(const dbresult_t::result_fn_t &fn)
{
  throw_if_finalized("Cannot execute statement: statement is finalized");

  int code = SQLITE_OK;
  dbresult_t result(*this);
  do {
    code = sqlite3_step(stmt_);
    if (dbis_error_code(db_.check_error(code))) {
      return code;
    } else if (code == SQLITE_ROW && fn) {
      fn(result);
    }
  } while (code == SQLITE_ROW);

  code = sqlite3_reset(stmt_);
  db_.check_error(code);

  return code;
}



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



int dbstatement_t::binding_index(const string &name)
{
  throw_if_finalized("Cannot get named binding index: statement is finalized");
  const auto iter = bind_vars_.find(name);
  int index = 0;
  if (iter == bind_vars_.end()) {
    bind_vars_.insert({name, index});
  } else {
    index = iter->second;
  }
  return index;
}



int dbstatement_t::clear_bindings()
{
  throw_if_finalized("Cannot clear bindings: statement is finalized");
  return db_.check_error(sqlite3_clear_bindings(stmt_));
}


} // namespace snow
