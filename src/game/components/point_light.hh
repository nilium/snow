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
  virtual ~point_light_t();

  float radius;
  vec4f_t color;
  rmaterial_t *cookie;

};


} // namespace snow

#endif /* end __SNOW__POINT_LIGHT_HH__ include guard */
