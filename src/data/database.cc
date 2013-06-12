/*
  database.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "database.hh"
#include "physicsfs_vfs.hh"


namespace snow {


#define throw_if_closed(LIT) \
  if (!is_open()) s_throw(std::runtime_error, LIT);



database_t::database_t(const string &path, bool throw_on_error, int flags, const string &vfs) :
  db_(NULL),
  error_msg_(),
  error_(SQLITE_OK),
  throw_on_error_(throw_on_error)
{
  const char *zvfs = vfs.size() == 0 ? NULL : vfs.c_str();
  int result = sqlite3_open_v2(path.c_str(), &db_, flags, zvfs);
  check_error_nothrow(result);
}



database_t::~database_t()
{
  if (is_open())
    close_nothrow();
}



database_t::database_t(database_t &&other) :
  db_(other.db_),
  error_msg_(std::move(other.error_msg_)),
  error_(other.error_)
{
  other.db_ = NULL;
  other.error_ = SQLITE_OK;
}



database_t database_t::read_physfs(const string &path, bool throw_on_error)
{
  return database_t(path, throw_on_error, SQLITE_OPEN_READONLY, "physfs");
}



database_t database_t::append_physfs(const string &path, bool throw_on_error)
{
  return database_t(path, throw_on_error, SQLITE_OPEN_READWRITE, "physfs");
}



database_t database_t::create_physfs(const string &path, bool throw_on_error)
{
  return database_t(path, throw_on_error, SQLITE_OPEN_CREATE, "physfs");
}



database_t database_t::temp_db()
{
  return database_t(":memory:");
}



dbstatement_t database_t::prepare(const string &sql)
{
  throw_if_closed("Cannot prepare a statement: DB is closed");
  dbstatement_t stmt(*this, sql);
  check_error(error_);
  return stmt;
}



void database_t::prepare(const string &sql, const prepare_fn_t &fn)
{
  throw_if_closed("Cannot prepare a statement: DB is closed");
  if (!fn) {
    s_throw(std::invalid_argument, "Cannot call prepare(sql, fn) with null function");
  } else {
    dbstatement_t stmt = prepare(sql);
    if (!stmt.is_finalized()) {
      fn(stmt);
    }
  }
}



int database_t::execute(const string &sql, const dbresult_t::result_fn_t &fn)
{
  throw_if_closed("Cannot execute SQL: DB is closed");
  int code = SQLITE_OK;
  dbstatement_t stmt = prepare(sql);
  if (!stmt.is_finalized() && !has_error()) {
    code = stmt.execute(fn);
    check_error(code);
  }
  code = stmt.finalize();
  return code;
}



int database_t::execute(const string &sql)
{
  throw_if_closed("Cannot execute SQL: DB is closed");

  dbstatement_t stmt = prepare(sql);
  if (!stmt.is_finalized()) {
    int code = stmt.execute();
    check_error(code);
  }
  if (!stmt.finalize())
    return error_;
  else
    return SQLITE_OK;
}



bool database_t::close()
{
  close_nothrow();
  return check_error(error_) == SQLITE_OK;
}



void database_t::close_nothrow()
{
  if (db_ != NULL) {
    int code = sqlite3_close(db_);
    if (check_error_nothrow(code) == SQLITE_OK) {
      db_ = NULL;
    }
  } else {
    check_error_nothrow(SQLITE_OK);
  }
}



int database_t::check_error(int code)
{
  error_ = code;
  if (dbis_error_code(code)) {
    error_msg_ = (db_ != NULL
#if SQLITE_VERSION_NUMBER >= 3007015
                  ? sqlite3_errmsg(db_)
                  : sqlite3_errstr(code));
#else
                  ? sqlite3_errmsg(db_)
                  : "Unable to determine error code");
#endif
    s_log_error("SQLite3 Error: %s", error_msg_.c_str());
    if (throw_on_error_) {
      s_throw(std::runtime_error, "SQLite3 Error: %s", error_msg_.c_str());
    }
  } else {
    error_msg_.clear();
  }
  return code;
}



int database_t::check_error_nothrow(int code)
{
  error_ = code;
  if (dbis_error_code(code)) {
    error_msg_ = (db_ != NULL
#if SQLITE_VERSION_NUMBER >= 3007015
                  ? sqlite3_errmsg(db_)
                  : sqlite3_errstr(code));
#else
                  ? sqlite3_errmsg(db_)
                  : "Unable to determine error code");
#endif
    s_log_error("SQLite3 Error: %s", error_msg_.c_str());
  } else {
    error_msg_.clear();
  }
  return code;
}


} // namespace snow
