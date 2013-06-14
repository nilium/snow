#include "autorelease.hh"

#if S_PLATFORM_APPLE

#import <Foundation/NSAutoreleasePool.h>

namespace snow {


void *autorelease_push()
{
  #if !__has_feature(objc_arc)
  return (void *)[[NSAutoreleasePool alloc] init];
  #else
  #warning Compiled with ARC, AR pool functions are NOPs
  return nullptr;
  #endif
}



void autorelease_pop(void *pool)
{
  #if __has_feature(objc_arc)
  [(NSAutoreleasePool *)pool release];
  #endif
}


} // namespace snow
#endif
