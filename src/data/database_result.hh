#ifndef __SNOW__DATABASE_RESULT_HH__
#define __SNOW__DATABASE_RESULT_HH__

#include "database_local.hh"
#include <functional>


namespace snow {


struct dbresult_t
{
  using result_fn_t = std::function<void(dbresult_t &)>;

  ~dbresult_t() = default;

  dbresult_t(const dbresult_t &) = delete;
  dbresult_t &operator = (const dbresult_t &) = delete;

  int           column_blob_size  (int col);
  int           column_blob_size16(int col);
  const void *  column_blob       (int col);
  int           column_int        (int col);
  int64_t       column_int64      (int col);
  double        column_double     (int col);
  const char *  column_text_ptr   (int col);
  const void *  column_text16_ptr (int col);
  string        column_text       (int col);
  inline float  column_float      (int col) { return (float)column_double(col); }

private:
  friend struct dbstatement_t;

  dbresult_t(dbstatement_t &stmt);

  sqlite3_stmt *stmt_;
};


} // namespace snow

#endif /* end __SNOW__DATABASE_RESULT_HH__ include guard */
