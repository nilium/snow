#include "database_result.hh"
#include "database_statement.hh"

namespace snow {


dbresult_t::dbresult_t(dbstatement_t &stmt) :
  stmt_(stmt.stmt_)
{
  // nop
}



int dbresult_t::column_blob_size(int col)
{
  return sqlite3_column_bytes(stmt_, col);
}



int dbresult_t::column_blob_size16(int col)
{
  return sqlite3_column_bytes16(stmt_, col);
}



const void *dbresult_t::column_blob(int col)
{
  return sqlite3_column_blob(stmt_, col);
}



int dbresult_t::column_int(int col)
{
  return sqlite3_column_int(stmt_, col);
}



int64_t dbresult_t::column_int64(int col)
{
  return sqlite3_column_int64(stmt_, col);
}



double dbresult_t::column_double(int col)
{
  return sqlite3_column_double(stmt_, col);
}



const char *dbresult_t::column_text_ptr(int col)
{
  return (const char *)sqlite3_column_text(stmt_, col);
}



const void *dbresult_t::column_text16_ptr(int col)
{
  return sqlite3_column_text16(stmt_, col);
}



string dbresult_t::column_text(int col)
{
  return string((const char *)sqlite3_column_text(stmt_, col));
}


} // namespace snow
