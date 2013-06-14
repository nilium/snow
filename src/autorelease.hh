#ifndef __SNOW__AUTORELEASE_HH__
#define __SNOW__AUTORELEASE_HH__

#include "config.hh"

#ifndef __has_feature
#define __has_feature(F) 0
#endif

/*
AUTORELEASE_PUSH and AUTORELEASE_POP work on more or less the same idea as
Obj-C's @autoreleasepool { ... } blocks. So, they must remain in the same scope,
variables declared between them will cease to exist outside of them, and so on.

Basically, treat them as though they're { ... } blocks, because they are.
*/

#if __has_feature(objc_arc)

#define AUTORELEASE_PUSH() do { @autoreleasepool {
#define AUTORELEASE_POP() } } while (0)

#elif S_PLATFORM_APPLE && !__has_feature(objc_arc)

#define AUTORELEASE_PUSH() do { void *OBJC_ARPOOL___ = ::snow::autorelease_push()
#define AUTORELEASE_POP() ::snow::autorelease_pop(OBJC_ARPOOL___); } while (0)

#else

#define AUTORELEASE_PUSH() do {
#define AUTORELEASE_POP() } while (0)

#endif

namespace snow {

S_EXPORT void *autorelease_push();
S_EXPORT void autorelease_pop(void *pool);

} // namespace snow

#endif /* end __SNOW__AUTORELEASE_HH__ include guard */
