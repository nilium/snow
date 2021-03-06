/*
  timing.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__TIMING_HH__
#define __SNOW__TIMING_HH__

namespace snow {


constexpr double FRAME_SECOND = 1.0;
constexpr double FRAME_HERTZ = 50.0;
constexpr double FRAME_SEQ_TIME = FRAME_SECOND / FRAME_HERTZ;


} // namespace snow

#endif /* end __SNOW__TIMING_HH__ include guard */
