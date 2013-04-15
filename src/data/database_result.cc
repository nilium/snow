#include "database_result.hh"
#include "database_statement.hh"

namespace snow {


dbresult_t::dbresult_t(dbstatement_t &stmt) :
  stmt_(stmt)
{
  // nop
}



/*******************************************************************************
*                               Column by index                                *
*******************************************************************************/

int dbresult_t::column_blob_size(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_bytes(stmt_.stmt_, col);
}



int dbresult_t::column_blob_size16(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_bytes16(stmt_.stmt_, col);
}



const void *dbresult_t::column_blob(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_blob(stmt_.stmt_, col);
}



int dbresult_t::column_int(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_int(stmt_.stmt_, col);
}



int64_t dbresult_t::column_int64(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_int64(stmt_.stmt_, col);
}



double dbresult_t::column_double(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_double(stmt_.stmt_, col);
}



const char *dbresult_t::column_text_ptr(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return (const char *)sqlite3_column_text(stmt_.stmt_, col);
}



const void *dbresult_t::column_text16_ptr(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return sqlite3_column_text16(stmt_.stmt_, col);
}



string dbresult_t::column_text(int col)
{
  if (col < 0 || col > stmt_.num_columns()) {
    s_throw(std::out_of_range, "Out of range column index for prepared statement");
  }
  return string((const char *)sqlite3_column_text(stmt_.stmt_, col));
}



float dbresult_t::column_float(int col)
{
  return (float)column_double(col);
}



unsigned dbresult_t::column_uint(int col)
{
  return (unsigned)column_int64(col);
}



/*******************************************************************************
*                                Column by name                                *
*******************************************************************************/

int dbresult_t::column_blob_size(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_bytes(stmt_.stmt_, index);
}



int dbresult_t::column_blob_size16(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_bytes16(stmt_.stmt_, index);
}



const void *dbresult_t::column_blob(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_blob(stmt_.stmt_, index);
}



int dbresult_t::column_int(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_int(stmt_.stmt_, index);
}



int64_t dbresult_t::column_int64(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_int64(stmt_.stmt_, index);
}



double dbresult_t::column_double(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_double(stmt_.stmt_, index);
}



const char *dbresult_t::column_text_ptr(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return (const char *)sqlite3_column_text(stmt_.stmt_, index);
}



const void *dbresult_t::column_text16_ptr(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return sqlite3_column_text16(stmt_.stmt_, index);
}



string dbresult_t::column_text(const string &col)
{
  const int index = stmt_.column_index(col);
  if (index == -1) {
    s_throw(std::invalid_argument, "Attempt to access column name that is not "
                                "part of the prepared statement");
  }
  return string((const char *)sqlite3_column_text(stmt_.stmt_, index));
}



float dbresult_t::column_float(const string &col)
{
  return (float)column_double(col);
}



unsigned dbresult_t::column_uint(const string &col)
{
  return (unsigned)column_int64(col);
}


} // namespace snow
