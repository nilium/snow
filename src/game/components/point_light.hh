/*
  point_light.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__POINT_LIGHT_HH__
#define __SNOW__POINT_LIGHT_HH__

#include "../../config.hh"
#include "component.hh"


namespace snow {


struct rmaterial_t;


struct point_light_t : component_t<point_light_t, LIGHT_COMPONENT, 256>
{

  point_light_t();
  point_light_t(game_object_t *obj);
  ~point_light_t() override;

  float radius;
  vec4f_t color;
  rmaterial_t *cookie;

};


} // namespace snow

#endif /* end __SNOW__POINT_LIGHT_HH__ include guard */
