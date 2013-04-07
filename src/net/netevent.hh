#ifndef __SNOW__NETEVENT_HH__
#define __SNOW__NETEVENT_HH__

#include <snow/config.hh>
#include <enet/enet.h>
#include <vector>


namespace snow {

struct netevent_t
{
  using charbuf_t = std::vector<uint8_t>;

  void              set_sender(uint16_t sender);
  void              set_message(uint16_t message);
  void              set_time(double time);
  void              set_buffer(charbuf_t &&buf);
  void              set_buffer(const charbuf_t &buf);

  uint16_t          sender() const;
  uint16_t          message() const;
  double            time() const;
  const charbuf_t & buffer() const;
  charbuf_t &       buffer();
  size_t            data_length() const;

  void read_from(const ENetPacket *const packet);
  void write_to(ENetPacket *packet);

  // Creates packets and sends them over the given medium
  bool send(ENetPeer *peer, enet_uint8 channel,
    int flags = ENET_PACKET_FLAG_RELIABLE);
  void broadcast(ENetHost *host, enet_uint8 channel,
    int flags = ENET_PACKET_FLAG_RELIABLE);

private:
  uint16_t    sender_   = 0;
  uint16_t    message_  = 0;
  double      time_     = 0;
  charbuf_t   buffer_   { };
};


} // namespace snow

#endif /* end __SNOW__NETEVENT_HH__ include guard */
