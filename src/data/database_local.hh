#ifndef __SNOW__DATABASE_LOCAL_HH__
#define __SNOW__DATABASE_LOCAL_HH__

#include <snow/config.hh>
#include <sqlite3.h>


namespace snow {


struct database_t;
struct dbresult_t;
struct dbstatement_t;


constexpr bool dbis_error_code(int code) {
  return !(code == SQLITE_OK || code == SQLITE_ROW || code == SQLITE_DONE);
}


} // namespace snow

#endif /* end __SNOW__DATABASE_LOCAL_HH__ include guard */
