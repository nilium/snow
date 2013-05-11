#ifndef __SNOW__COMPONENT_ID_HH__
#define __SNOW__COMPONENT_ID_HH__


namespace snow {


enum component_id_t : unsigned
{
  TRANSFORM_COMPONENT = 0,
  LIGHT_COMPONENT,
  PICK_UP_COMPONENT,
  DURABLE_COMPONENT,
  WEARABLE_COMPONENT,
  PROJECTILE_COMPONENT,
  MAX_COMPONENT_IDS,
};


} // namespace snow


#endif /* end __SNOW__COMPONENT_ID_HH__ include guard */
