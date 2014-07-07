#ifndef __SNOW__DEFER_HH__
#define __SNOW__DEFER_HH__

#include <snow/config.hh>
#include <functional>


namespace snow
{


struct S_EXPORT deferred
{
  using function = std::function<void()>;

  deferred() = default;

  template <typename FN>
  deferred(FN &&func)
  : _fn(std::forward<FN>(func))
  {
    /* nop */
  }

  deferred(deferred const &) = delete;
  deferred(deferred &&) = delete;

  deferred &operator = (deferred const &) = delete;
  deferred &operator = (deferred &&) = delete;

  ~deferred();

private:
  function _fn;

}; // struct deferred


} // namespace snow

#endif /* end __SNOW__DEFER_HH__ include guard */
