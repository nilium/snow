/*
  system.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__SYSTEM_HH__
#define __SNOW__SYSTEM_HH__

#include "../config.hh"


namespace snow {


struct event_t;


struct system_t
{
  virtual ~system_t() = 0;

  /*==============================================================================
    active / set_active(bool)

      Gets and sets whether the system is active. Inactive systems will not have
      their event or frame functions called. In order to prevent attempts to
      cram additional logic in before events/frames, these functions cannot be
      overridden.
  ==============================================================================*/
  bool active() const;
  void set_active(bool active);

  /*============================================================================
    event(event_t)

      Receives an event and may or may not do something with it. Returns true
      to indicate that the event should be passed on to the next system_t,
      otherwise returns false to end the event passing.

      It is considered bad form for this function to create new events, though
      it may choose to do so. Preferably, the function simply sets state and
      any events that need to be emitted get sent out in the frame function. If
      the function does emit an event, it should be careful not to create an
      infinite loop by doing so.

      Default implementation simply returns true.
  ============================================================================*/
  virtual bool event(const event_t &event);

  /*============================================================================
    frame(step)

      Performs a single frame's logic for the given timestep. In most cases,
      the timestep can be ignored as it will likely never change.

      This function may send out events as it desires, since it cannot
      accidentally create

      Default implementation does nothing.
  ============================================================================*/
  virtual void frame(double step, double timeslice);

  /*============================================================================
    FunctionName

      Description
  ============================================================================*/
  virtual void draw(double timeslice);

private:
  bool active_ = true;
};


} // namespace snow

#endif /* end __SNOW__SYSTEM_HH__ include guard */
