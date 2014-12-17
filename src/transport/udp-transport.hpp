#ifndef NDNCXXEXT_TRANSPORT_UDP_TRANSPORT_HPP
#define NDNCXXEXT_TRANSPORT_UDP_TRANSPORT_HPP

#include "../common.hpp"
#include <ndn-cxx/transport/transport.hpp>
#include <ndn-cxx/util/face-uri.hpp>

namespace ndn {

class UdpTransport : public Transport
{
public:
  UdpTransport(const ndn::util::FaceUri& faceUri, uint16_t localPort = 0);

  virtual void
  connect(boost::asio::io_service& io, const ReceiveCallback& receiveCallback);

  virtual void
  close();

  virtual void
  send(const Block& wire);

  virtual void
  send(const Block& header, const Block& payload);

  virtual void
  pause();

  virtual void
  resume();

private:
  void
  startReceive();

private:
  uint16_t m_localPort;
  boost::asio::ip::udp::endpoint m_ep;
  unique_ptr<boost::asio::ip::udp::socket> m_sock;
};

} // namespace ndn

#endif // NDNCXXEXT_TRANSPORT_UDP_TRANSPORT_HPP
