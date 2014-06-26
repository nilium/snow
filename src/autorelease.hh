#ifndef __SNOW__AUTORELEASE_HH__
#define __SNOW__AUTORELEASE_HH__

#include "config.hh"

#ifndef __has_feature
#define __has_feature(F) 0
#endif


namespace snow {


namespace aux_apple_ {

S_EXPORT void *autorelease_push();
S_EXPORT void autorelease_pop(void *pool) noexcept;

} // namespace aux_apple_


template <typename Block, typename... Args>
auto with_autorelease(Block &&block, Args &&... argv)
  -> decltype(block(std::forward<Args>(argv)...))
{

#if S_PLATFORM_APPLE

#if __has_feature(objc_arc)
  // Use ARC block

  @autoreleasepool {
    return block(std::forward<Args>(argv)...);
  };

#else
  // Use non-ARC wrapper functions

  void *pool = aux_apple_::autorelease_push();
  try {
    return block(std::forward<Args>(argv)...);
  } catch (...) {
    aux_apple_::autorelease_pop(pool);
    throw;
  }

#endif

#else // !S_PLATFORM_APPLE
  // No-op, just execute the block (likely inlined)

  return block(std::forward<Args>(argv)...);

#endif

}


} // namespace snow

#endif /* end __SNOW__AUTORELEASE_HH__ include guard */
