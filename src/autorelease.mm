#include "autorelease.hh"

#if S_PLATFORM_APPLE
#import <Foundation/NSAutoreleasePool.h>
#endif

namespace snow {

namespace aux_apple_ {

#if S_PLATFORM_APPLE

void *autorelease_push()
{
  #if !__has_feature(objc_arc)
  return (void *)[[NSAutoreleasePool alloc] init];
  #else
  #warning Compiled with ARC, AR pool functions are NOPs
  return nullptr;
  #endif
}



void autorelease_pop(void *pool) noexcept
{
  #if !__has_feature(objc_arc)
  [(NSAutoreleasePool *)pool release];
  #else
  #warning Compiled with ARC, AR pool functions are NOPs
  #endif
}

#else

void *autorelease_push()
{
  return nullptr;
}



void autorelease_pop(void *pool) noexcept
{
  (void)pool;
}

#endif

} // namespace aux_apple_

} // namespace snow

