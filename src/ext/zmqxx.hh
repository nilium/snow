/* zmqxx.hh -- Noel Cower -- Public Domain */

#ifndef __ZMQXX_HH__
#define __ZMQXX_HH__

#include <zmq.h>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <memory>
#include <mutex>


namespace zmq {


#ifndef NDEBUG
#define ZMQ_ASSERT__(STMT, EQTO, TAG) \
  do { int ZMQRC___ = (STMT); assert(ZMQRC___ == (EQTO)); } while(0)
#else
#define ZMQ_ASSERT__(STMT, EQTO, TAG) STMT
#endif


using pollitem_t = zmq_pollitem_t;


inline int poll(pollitem_t *items, int nitems, long timeout)
{
  return zmq_poll(items, nitems, timeout);
}


struct socket_t;
struct context_t;
struct msg_t;


struct context_t
{
  static inline context_t &shared()
  {
    static std::once_flag shared_ctx_initer;
    static std::unique_ptr<context_t> shared_ctx;
    std::call_once(shared_ctx_initer, [] {
      shared_ctx.reset(new context_t());
    });
    return *shared_ctx;
  }

  inline context_t() :
    ctx_(zmq_ctx_new())
  {
    /* nop */
  }

  inline ~context_t()
  {
    destroy();
  }

  inline context_t(context_t &&other) :
    ctx_(other.ctx_)
  {
    other.ctx_ = nullptr;
  }

  inline context_t &operator = (context_t &&other)
  {
    if ((&other == this) || (ctx_ && !destroy())) {
      return *this;
    }
    std::swap(ctx_, other.ctx_);
    return *this;
  }

  context_t(const context_t &other) = delete;
  context_t &operator = (const context_t &other) = delete;

  inline bool destroy()
  {
    if (!ctx_) {
      return 0;
    }
    int result = zmq_ctx_destroy(ctx_);
    assert(result == 0);
    if (result == 0 || (result == -1 && errno != EINTR)) {
      ctx_ = nullptr;
    }
    return result == 0;
  }

  inline operator void *() const
  {
    return ctx_;
  }

  inline int io_threads() const
  {
    int result = zmq_ctx_get(ctx_, ZMQ_IO_THREADS);
    assert(result == 0);
    return result;
  }

  inline bool set_io_threads(int num)
  {
    int result = zmq_ctx_set(ctx_, ZMQ_IO_THREADS, num);
    assert(result == 0);
    return result == 0;
  }

  inline int max_sockets() const
  {
    int result = zmq_ctx_get(ctx_, ZMQ_MAX_SOCKETS);
    assert(result == 0);
    return result;
  }

  inline bool set_max_sockets(int num)
  {
    int result = zmq_ctx_set(ctx_, ZMQ_MAX_SOCKETS, num);
    assert(result == 0);
    return result == 0;
  }

  mutable void *ctx_;
}; // struct context_t


struct msg_t
{
  inline msg_t()
  {
    ZMQ_ASSERT__(
      zmq_msg_init(&msg_)
      , 0, "Initializing message");
  }

  inline explicit msg_t(size_t size)
  {
    ZMQ_ASSERT__(
      zmq_msg_init_size(&msg_, size)
      , 0, "Initializing message");
  }

  inline msg_t(void *data, size_t size, zmq_free_fn *ffn, void *hint)
  {
    ZMQ_ASSERT__(
      zmq_msg_init_data(&msg_, data, size, ffn, hint)
      ,0, "Initializing message");
  }

  inline msg_t(const msg_t &other)
  {
    ZMQ_ASSERT__(
      zmq_msg_init(&msg_)
      , 0, "Initializing message");
    ZMQ_ASSERT__(
      zmq_msg_copy(&msg_, const_cast<zmq_msg_t *>(&other.msg_))
      , 0, "Copying message");
  }

  inline msg_t(msg_t &&other)
  {
    ZMQ_ASSERT__(
      zmq_msg_init(&msg_)
      , 0, "Initializing message");
    std::swap(msg_, other.msg_);
  }

  inline ~msg_t()
  {
    ZMQ_ASSERT__(
      zmq_msg_close(&msg_)
      , 0, "Closing message");
  }

  inline msg_t &operator = (const msg_t &other)
  {
    if (&other == this) {
      return *this;
    }
    ZMQ_ASSERT__(
      zmq_msg_copy(&msg_, const_cast<zmq_msg_t *>(&other.msg_))
      , 0, "Copying message");
    return *this;
  }

  inline msg_t &operator = (msg_t &&other)
  {
    if (&other == this) {
      return *this;
    }
    ZMQ_ASSERT__(
      zmq_msg_move(&msg_, &other.msg_)
      , 0, "Moving message");
    return *this;
  }

  inline bool move(msg_t &other)
  {
    int result = zmq_msg_move(&msg_, &other.msg_);
    assert(result == 0);
    return result == 0;
  }

  inline bool copy(const msg_t &other)
  {
    int result = zmq_msg_copy(&msg_, const_cast<zmq_msg_t *>(&other.msg_));
    assert(result == 0);
    return result == 0;
  }

  inline bool more() const
  {
    return zmq_msg_more(&msg_) > 0;
  }

  inline void *data()
  {
    return zmq_msg_data(&msg_);
  }

  inline const void *data() const
  {
    return zmq_msg_data(&msg_);
  }

  inline size_t size() const
  {
    return zmq_msg_size(&msg_);
  }

  mutable zmq_msg_t msg_;
}; // struct msg_t


struct socket_t
{
  inline socket_t() :
    socket_(nullptr)
  {
    /* nop */
  }

  inline socket_t(context_t &ctx, int type) :
    socket_(zmq_socket(ctx.ctx_, type))
  {
    /* nop */
  }

  inline socket_t(socket_t &&other) :
    socket_(nullptr)
  {
    std::swap(socket_, other.socket_);
  }

  inline socket_t &operator = (socket_t &&other)
  {
    if (&other == this) {
      return *this;
    }
    close();
    std::swap(socket_, other.socket_);
    return *this;
  }

  socket_t(const socket_t &other) = delete;
  socket_t &operator = (const socket_t &other) = delete;

  inline int type() const
  {
    int type = 0;
    size_t type_size = sizeof(type);
    if (zmq_getsockopt(socket_, ZMQ_TYPE, &type, &type_size) == 0) {
      return type;
    }
    assert(type != -1);
    return -1;
  }

  inline int linger() const
  {
    int millisecs = 0;
    size_t millisecs_size = sizeof(millisecs);
    if (zmq_getsockopt(socket_, ZMQ_LINGER, &millisecs, &millisecs_size) == 0) {
      return millisecs;
    }
    assert(0);
    return -1;
  }

  inline bool set_linger(int millisecs)
  {
    int result;
    result = zmq_setsockopt(socket_, ZMQ_LINGER, &millisecs, sizeof(millisecs)),
    assert(result == 0);
    return result == 0;
  }

  inline bool bind(const char *endpoint)
  {
    int result = zmq_bind(socket_, endpoint);
    assert(result == 0);
    return result == 0;
  }

  inline bool unbind(const char *endpoint)
  {
    int result = zmq_unbind(socket_, endpoint);
    assert(result == 0);
    return result == 0;
  }

  inline bool connect(const char *endpoint)
  {
    int result = zmq_connect(socket_, endpoint);
    assert(result == 0);
    return result == 0;
  }

  inline bool disconnect(const char *endpoint)
  {
    int result = zmq_disconnect(socket_, endpoint);
    assert(result == 0);
    return result == 0;
  }

  inline int send(const void *buffer, size_t length, int flags = 0)
  {
    int result = zmq_send(socket_, const_cast<void *>(buffer), length, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline int send(zmq_msg_t *msg, int flags = 0)
  {
    int result = zmq_msg_send(msg, socket_, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline int send(msg_t &msg, int flags = 0)
  {
    int result = zmq_msg_send(&msg.msg_, socket_, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline int recv(void *buffer, size_t length, int flags = 0)
  {
    int result = zmq_recv(socket_, buffer, length, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline int recv(zmq_msg_t *msg, int flags = 0)
  {
    int result = zmq_msg_recv(msg, socket_, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline int recv(msg_t &msg, int flags = 0)
  {
    int result = zmq_msg_recv(&msg.msg_, socket_, flags);
    assert(result >= 0 || flags&ZMQ_DONTWAIT);
    return result;
  }

  inline bool close()
  {
    if (!socket_) {
      return true;
    }
    int result = zmq_close(socket_);
    assert(result == 0);
    socket_ = nullptr;
    return result == 0;
  }

  inline operator void * () const
  {
    return socket_;
  }

  void *socket_;
}; // struct socket_t


#undef ZMQ_ASSERT__


} // namespace zmq

#endif /* end __ZMQXX_HH__ include guard */
