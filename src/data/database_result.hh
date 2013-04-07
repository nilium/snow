#ifndef __SNOW__DATABASE_RESULT_HH__
#define __SNOW__DATABASE_RESULT_HH__

#include "database_local.hh"
#include <functional>


namespace snow {


struct dbresult_t
{
  using result_fn_t = std::function<void(dbresult_t &)>;

  ~dbresult_t() = default;

  // Indexed columns
  const char *  column_text_ptr   (const string &col);
  const char *  column_text_ptr   (int col);
  const void *  column_blob       (const string &col);
  const void *  column_blob       (int col);
  const void *  column_text16_ptr (const string &col);
  const void *  column_text16_ptr (int col);
  double        column_double     (const string &col);
  double        column_double     (int col);
  float         column_float      (const string &col);
  float         column_float      (int col);
  int           column_blob_size  (const string &col);
  int           column_blob_size  (int col);
  int           column_blob_size16(const string &col);
  int           column_blob_size16(int col);
  int           column_int        (const string &col);
  int           column_int        (int col);
  int64_t       column_int64      (const string &col);
  int64_t       column_int64      (int col);
  string        column_text       (const string &col);
  string        column_text       (int col);
  unsigned      column_uint       (const string &col);
  unsigned      column_uint       (int col);

  // Named columns

private:
  dbresult_t(const dbresult_t &) = default;
  dbresult_t &operator = (const dbresult_t &) = default;

  friend struct dbstatement_t;
  friend struct dbiterator_t;;

  dbresult_t(dbstatement_t &stmt);

  dbstatement_t &stmt_;
};


} // namespace snow

#endif /* end __SNOW__DATABASE_RESULT_HH__ include guard */
