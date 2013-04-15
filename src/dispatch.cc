#include "dispatch.hh"


static void s_work_bouncer_nofree(void *context)
{
  s_dispatch_work_t *fn = (s_dispatch_work_t *)context;
  (*fn)();
}



static void s_work_bouncer(void *context)
{
  s_dispatch_work_t *fn = (s_dispatch_work_t *)context;
  (*fn)();
  delete fn;
}



static s_dispatch_work_t *s_move_work(s_dispatch_work_t &&work)
{
  return new s_dispatch_work_t(std::forward<s_dispatch_work_t &&>(work));
}



static s_dispatch_work_t *s_copy_work(const s_dispatch_work_t &work)
{
  return new s_dispatch_work_t(work);
}


/*******************************************************************************
*                              Regular scheduling                              *
*******************************************************************************/

void dispatch_async_s(dispatch_queue_t queue, const s_dispatch_work_t &work)
{
  dispatch_async_f(queue, (void *)s_copy_work(work), s_work_bouncer);
}



void dispatch_async_s(dispatch_queue_t queue, s_dispatch_work_t &&work)
{
  s_dispatch_work_t *work_copy = nullptr;
  work_copy = s_move_work(std::forward<s_dispatch_work_t &&>(work));
  dispatch_async_f(queue, (void *)work_copy, s_work_bouncer);
}



void dispatch_sync_s(dispatch_queue_t queue, const s_dispatch_work_t &work)
{
  dispatch_sync_f(queue, (void *)&work, s_work_bouncer_nofree);
}



/*******************************************************************************
*                              Barrier scheduling                              *
*******************************************************************************/

void dispatch_barrier_async_s(dispatch_queue_t queue, const s_dispatch_work_t &work)
{
  dispatch_barrier_async_f(queue, (void *)s_copy_work(work), s_work_bouncer);
}



void dispatch_barrier_async_s(dispatch_queue_t queue, s_dispatch_work_t &&work)
{
  s_dispatch_work_t *work_copy = nullptr;
  work_copy = s_move_work(std::forward<s_dispatch_work_t &&>(work));
  dispatch_barrier_async_f(queue, (void *)work_copy, s_work_bouncer);
}



void dispatch_barrier_sync_s(dispatch_queue_t queue, const s_dispatch_work_t &work)
{
  dispatch_barrier_sync_f(queue, (void *)&work, s_work_bouncer_nofree);
}



/*******************************************************************************
*                               Group scheduling                               *
*******************************************************************************/

void dispatch_group_async_s(dispatch_group_t group, dispatch_queue_t queue, const s_dispatch_work_t &work)
{
  dispatch_group_async_f(group, queue, (void *)s_copy_work(work), s_work_bouncer);
}



void dispatch_group_async_s(dispatch_group_t group, dispatch_queue_t queue, s_dispatch_work_t &&work)
{
  s_dispatch_work_t *work_copy = nullptr;
  work_copy = s_move_work(std::forward<s_dispatch_work_t &&>(work));
  dispatch_group_async_f(group, queue, (void *)work_copy, s_work_bouncer);
}
