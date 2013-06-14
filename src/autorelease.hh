#ifndef __SNOW__AUTORELEASE_HH__
#define __SNOW__AUTORELEASE_HH__

#include "config.hh"

#ifndef __has_feature
#define __has_feature(F) 0
#endif

#if __has_feature(objc_arc)

#define AUTORELEASE_PUSH() @autoreleasepool {
#define AUTORELEASE_POP() }

#elif S_PLATFORM_APPLE && !__has_feature(objc_arc)

#define AUTORELEASE_PUSH() do { void *OBJC_ARPOOL___ = ::snow::autorelease_push()
#define AUTORELEASE_POP() ::snow::autorelease_pop(OBJC_ARPOOL___); } while (0)

#else

#define AUTORELEASE_PUSH()
#define AUTORELEASE_POP()

#endif

namespace snow {

S_EXPORT void *autorelease_push();
S_EXPORT void autorelease_pop(void *pool);

} // namespace snow

#endif /* end __SNOW__AUTORELEASE_HH__ include guard */
