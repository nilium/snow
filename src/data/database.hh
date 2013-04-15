#ifndef __SNOW__DATABASE_HH__
#define __SNOW__DATABASE_HH__

#include "database_local.hh"
#include "database_result.hh"
#include <functional>


/**
  A thin wrapper around sqlite3's C API. Intended to reduce the amount of work
  needed to work with databases.

  A database is open on construction unless an error occurs while opening it.
  Check is_open to see if it opened, or has_error/error/error_msg (any), to see
  if the DB was successfully opened. A closed database is considered invalid.

  The static function database_t::temp_db() will return an in-memory temporary
  database.

  database_t::execute(sql) prepares a statement, executes it, and destroys it -
  optionally with a function to handle the results, if there are any. You may
  not need to provide a function for INSERT or UPDATE statements, for example.

  database_t::prepare(sql) returns a prepared SQL statement. The returned
  statement must be finalized before the database is closed. Once finalized,
  the statement is considered invalid.
**/


namespace snow {


struct database_t
{
  using prepare_fn_t = const std::function<void(dbstatement_t &)>;


  database_t(
    const string &path,
    bool throw_on_error = true,
    int flags = SQLITE_OPEN_READONLY,
    const string &vfs = "");
  ~database_t();

  database_t(database_t &&);

  database_t(const database_t &) = delete;
  database_t &operator = (const database_t &) = delete;
  database_t &operator = (database_t &&) = delete;

  // All *_physfs functions assume the PhysFS VFS has already been registered.
  // Opens a database for reading only via PhysFS.
  static database_t read_physfs(const string &path, bool throw_on_error = true);
  // Opens a file for reading and writing. If the file does not exist in the
  // write directory already, it will be created as if it didn't exist
  // regardless of whether it's in the read-only path.
  static database_t append_physfs(const string &path, bool throw_on_error = true);
  // Creates a new file in the write directory specifically for storing user
  // specific data, caches, etc. This will always create a new file.
  static database_t create_physfs(const string &path, bool throw_on_error = true);

  // Creates a temporary in-memory database.
  static database_t temp_db();

  inline const string &error_msg() const { return error_msg_; }
  inline int error() const { return error_; }
  inline bool has_error() const { return dbis_error_code(error_); }

  inline bool is_open() const { return db_ != NULL; }

  inline bool throw_on_error() const { return throw_on_error_; }
  inline void set_throw_on_error(bool enabled) { throw_on_error_ = enabled; }

  dbstatement_t prepare(const string &sql);
  // Note: for prepare(sql, fn), the function may not be null
  void prepare(const string &sql, const prepare_fn_t &fn);

  int execute(const string &sql);
  inline int execute(const string &sql, std::nullptr_t np) { return execute(sql); }
  int execute(const string &sql, const dbresult_t::result_fn_t &fn);

  bool close();


private:
  friend dbstatement_t;

  // Checks the result (from an sqlite3_ function) to see if it's an error code.
  // If it is, error_msg_ is set to the current error message.
  // If not, error_msg_ is cleared.
  // Either way, error_ is set to the result code and the method returns the
  // error code it was given.
  // If calling check_error inside a destructor, pass true for disable_throw.
  int check_error(int code);
  int check_error_nothrow(int code);

  void close_nothrow();

  sqlite3 *db_;
  string error_msg_;
  int error_;
  bool throw_on_error_;
};


} // namespace snow


#include "database_statement.hh"


#endif /* end __SNOW__DATABASE_HH__ include guard */
