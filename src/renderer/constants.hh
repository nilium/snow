/*
  constants.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__CONSTANTS_HH__
#define __SNOW__CONSTANTS_HH__

namespace snow {


enum : unsigned
{
  ATTRIB_POSITION,
  ATTRIB_COLOR,
  ATTRIB_NORMAL,
  ATTRIB_BINORMAL,
  ATTRIB_TANGENT,
  ATTRIB_TEXCOORD0,
  ATTRIB_TEXCOORD1,
  ATTRIB_TEXCOORD2,
  ATTRIB_TEXCOORD3,
  ATTRIB_BONE_INDICES,
  ATTRIB_BONE_WEIGHTS,
};


enum : int
{
  UNIFORM_MODELVIEW = 0,
  UNIFORM_PROJECTION,
  UNIFORM_TEXTURE_MATRIX,
  UNIFORM_BONES,
  UNIFORM_TEXTURE0,
  UNIFORM_TEXTURE1,
  UNIFORM_TEXTURE2,
  UNIFORM_TEXTURE3,
  UNIFORM_TEXTURE4,
  UNIFORM_TEXTURE5,
  UNIFORM_TEXTURE6,
  UNIFORM_TEXTURE7,
};


} // namespace snow

#endif /* end __SNOW__CONSTANTS_HH__ include guard */
