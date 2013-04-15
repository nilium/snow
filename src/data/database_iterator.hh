#ifndef __SNOW__DATABASE_ITERATOR_HH__
#define __SNOW__DATABASE_ITERATOR_HH__

#include "../config.hh"
#include "database_local.hh"
#include "database_result.hh"
#include <iterator>


namespace snow {


struct dbresult_t;
struct dbstatement_t;


struct dbiterator_t : public std::iterator<std::forward_iterator_tag, dbresult_t>
{
  dbiterator_t(const dbiterator_t &other) = default;
  dbiterator_t &operator = (const dbiterator_t &other) = default;

  bool valid() const;
  dbresult_t &operator * ();
  bool operator == (const dbiterator_t &other) const;
  bool operator != (const dbiterator_t &other) const;
  // Note: incrementings invalidates all prior iterators other than end()
  dbiterator_t &operator ++ ();
  dbiterator_t operator ++ (int dummy);

private:
  friend struct dbstatement_t;

  dbiterator_t(dbstatement_t &stmt, int code);
  dbiterator_t(dbstatement_t &stmt);

  dbstatement_t &stmt_;
  dbresult_t result_;
  bool end_;
  int sequence_;
  int code_;
};


} // namespace snow

#endif /* end __SNOW__DATABASE_ITERATOR_HH__ include guard */
