/*
  database_iterator.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "database_iterator.hh"
#include "database_statement.hh"

namespace snow {


dbiterator_t::dbiterator_t(dbstatement_t &stmt) :
  stmt_(stmt),
  result_(stmt_),
  end_(false),
  sequence_(stmt.sequence_),
  code_(stmt.step())
{
  // nop
}



dbiterator_t::dbiterator_t(dbstatement_t &stmt, int code) :
  stmt_(stmt),
  result_(stmt_),
  end_(true),
  sequence_(0),
  code_(code)
{
  // nop
}



bool dbiterator_t::valid() const
{
  return stmt_.sequence_ == sequence_ &&
    (end_ || code_ == SQLITE_ROW || code_ == SQLITE_DONE);
}



dbresult_t &dbiterator_t::operator * ()
{
  if (!valid()) {
    s_throw(std::runtime_error, "Attempt to dereference invalid iterator.");
  }
  return result_;
}



bool dbiterator_t::operator == (const dbiterator_t &other) const
{
  if ((other.end_ && !end_) || (!other.end_ && end_)) {
    return code_ == other.code_;
  } else {
    // works for end() == end() since all end iterators have the a sequence of 0
    // aside from that, an iterator is the same as any other statement iterator
    // provides its statement and sequence are equal. If they're inequal, one
    // or both iterators must be invalid.
    return sequence_ == other.sequence_ && (&stmt_ == &other.stmt_);
  }
}



bool dbiterator_t::operator != (const dbiterator_t &other) const
{
  return !(other == *this);
}



auto dbiterator_t::operator ++ () -> dbiterator_t &
{
  if (!valid()) {
    s_throw(std::runtime_error, "Attempt to increment invalid iterator.");
  } else if (code_ == SQLITE_ROW) {
    code_ = stmt_.step();
  } else {
    s_throw(std::runtime_error, "Attempt to increment iterator at end of execution");
  }
  return *this;
}



auto dbiterator_t::operator ++ (int dummy) -> dbiterator_t
{
  (void)dummy;
  dbiterator_t iter = *this;
  return ++iter;
}


} // namespace snow
