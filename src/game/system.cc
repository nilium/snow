#include "system.hh"

namespace snow {


bool system_t::active() const
{
  return active_;
}



void system_t::set_active(bool active)
{
  active_ = active;
}



bool system_t::event(const event_t &event)
{
  return true;
}



void system_t::frame(double step, double timeslice)
{
  // NOP
}



void system_t::draw(double timeslice)
{
  // NOP
}


} // namespace snow
