/*
  system.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "system.hh"

namespace snow {


system_t::~system_t()
{
  /* nop */
}



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
