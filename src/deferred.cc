#include "deferred.hh"


namespace snow
{


deferred::~deferred()
{
  if (_fn) {
    _fn();
  }
}


} // namespace snow

