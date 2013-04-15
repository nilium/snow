#ifndef __SNOW__DISPATCH_HH__
#define __SNOW__DISPATCH_HH__


#include "config.hh"
#if TARGET_OS_MAC
#include <dispatch/dispatch.h>
#endif
#include <functional>


using s_dispatch_work_t = std::function<void()>;


void dispatch_async_s(dispatch_queue_t queue, const s_dispatch_work_t &);
void dispatch_async_s(dispatch_queue_t queue, s_dispatch_work_t &&);
void dispatch_sync_s(dispatch_queue_t queue, const s_dispatch_work_t &);

void dispatch_barrier_async_s(dispatch_queue_t queue, const s_dispatch_work_t &);
void dispatch_barrier_async_s(dispatch_queue_t queue, s_dispatch_work_t &&);
void dispatch_barrier_sync_s(dispatch_queue_t queue, const s_dispatch_work_t &);

void dispatch_group_async_s(dispatch_group_t group, dispatch_queue_t queue, const s_dispatch_work_t &);
void dispatch_group_async_s(dispatch_group_t group, dispatch_queue_t queue, s_dispatch_work_t &&);


#endif /* end __SNOW__DISPATCH_HH__ include guard */
