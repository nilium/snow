#include "netevent.hh"
#include <snow/types/object_pool.hh>


namespace snow {


void netevent_t::set_sender(uint16_t sender)
{
  sender_ = sender;
}



void netevent_t::set_time(double time)
{
  time_ = time;
}



void netevent_t::set_message(uint16_t message)
{
  message_ = message;
}



void netevent_t::set_buffer(charbuf_t &&buf)
{
  buffer_ = buf;
}



void netevent_t::set_buffer(const charbuf_t &buf)
{
  buffer_ = buf;
}



uint16_t netevent_t::sender() const
{
  return sender_;
}



uint16_t netevent_t::message() const
{
  return message_;
}



double netevent_t::time() const
{
  return time_;
}



auto netevent_t::buffer() const -> const charbuf_t &
{
  return buffer_;
}



auto netevent_t::buffer() -> charbuf_t &
{
  return buffer_;
}



size_t netevent_t::data_length() const
{
  return sizeof(sender_) + sizeof(message_) + sizeof(time_) + buffer_.size();
}



void netevent_t::read_from(const ENetPacket *const packet)
{
  if (packet == NULL) {
    s_throw(std::invalid_argument, "ENetPacket is null");
  }

  const uint16_t *shortbuf = (const uint16_t *)packet->data;
  size_t size = packet->dataLength;
  if (size >= sizeof(sender_)) {
    sender_ = shortbuf[0];
    size -= sizeof(sender_);
  }

  if (size >= sizeof(message_)) {
    message_ = shortbuf[1];
    size -= sizeof(message_);
  }

  if (size >= sizeof(time_)) {
    time_ = *(double *)(shortbuf + 2);
    size -= sizeof(time_);
  }

  buffer_.resize(size);
  if (size > 0) {
    const int data_offset = sizeof(sender_) + sizeof(message_) + sizeof(time_);
    memcpy(buffer_.data(), packet->data + data_offset, size);
  }
}



void netevent_t::write_to(ENetPacket *packet)
{
  if (packet == NULL) {
    s_throw(std::invalid_argument, "ENetPacket is null");
  }

  const size_t datalen = data_length();

  if (packet->dataLength != datalen && enet_packet_resize(packet, datalen)) {
    s_throw(std::runtime_error, "Failed to resize ENetPacket");
  }

  uint16_t *shortbuf = (uint16_t *)packet->data;
  shortbuf[0] = sender_;
  shortbuf[1] = message_;
  *(double *)(shortbuf + 2) = time_;
  if (!buffer_.empty()) {
    const int data_offset = sizeof(sender_) + sizeof(message_) + sizeof(time_);
    memcpy(packet->data + data_offset, buffer_.data(), buffer_.size());
  }
}


bool netevent_t::send(ENetPeer *peer, enet_uint8 channel, int flags)
{
  ENetPacket *packet = enet_packet_create(NULL, data_length(), flags);
  write_to(packet);
  return enet_peer_send(peer, channel, packet) == 0;
}


void netevent_t::broadcast(ENetHost *host, enet_uint8 channel, int flags)
{
  ENetPacket *packet = enet_packet_create(NULL, data_length(), flags);
  write_to(packet);
  enet_host_broadcast(host, channel, packet);
}


} // namespace snow
