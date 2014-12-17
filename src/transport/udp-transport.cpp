#include "udp-transport.hpp"
#include <boost/lexical_cast.hpp>

namespace ndn {

UdpTransport::UdpTransport(const ndn::util::FaceUri& faceUri, uint16_t localPort)
  : m_localPort(localPort)
{
  if (!faceUri.isCanonical()) {
    throw Error("FaceUri is not canonical");
  }
  if (faceUri.getScheme() != "udp4") {
    throw Error("FaceUri is not udp4 scheme");
  }

  boost::system::error_code ec;
  m_ep.address(boost::asio::ip::address_v4::from_string(faceUri.getHost(), ec));
  if (ec) {
    throw Error("FaceUri.host is invalid");
  }

  try {  
    m_ep.port(boost::lexical_cast<uint16_t>(faceUri.getPort()));
  }
  catch (boost::bad_lexical_cast&) {
    throw Error("FaceUri.port is invalid");
  }
}

void
UdpTransport::connect(boost::asio::io_service& io, const ReceiveCallback& receiveCallback)
{
  using boost::asio::ip::udp;
  BOOST_ASSERT(m_sock == nullptr);
  if (m_localPort == 0) { // random local port
    m_sock.reset(new udp::socket(io, udp::v4()));
  }
  else { // fixed local port
    m_sock.reset(new udp::socket(io, udp::endpoint(udp::v4(), m_localPort)));
  }
  m_sock->connect(m_ep);

  this->Transport::connect(io, receiveCallback);
  m_isConnected = m_isExpectingData = true;

  this->startReceive();
}

void
UdpTransport::close()
{
  BOOST_ASSERT(m_sock != nullptr);
  m_isConnected = m_isExpectingData = false;
  m_sock->cancel();
  m_sock.reset();
}

void
UdpTransport::send(const Block& wire)
{
  BOOST_ASSERT(m_sock != nullptr);
  m_sock->async_send(boost::asio::buffer(wire.wire(), wire.size()),
      [] (const boost::system::error_code& ec, size_t nTransferred) {
      });
}

void
UdpTransport::send(const Block& header, const Block& payload)
{
  BOOST_ASSERT_MSG(false, "not implemented");
}

void
UdpTransport::startReceive()
{
  BOOST_ASSERT(m_sock != nullptr);
  static uint8_t buffer[ndn::MAX_NDN_PACKET_SIZE];
  m_sock->async_receive(boost::asio::buffer(buffer, sizeof(buffer)),
      [this, &buffer] (const boost::system::error_code& ec, size_t nTransferred) {
        if (!ec && nTransferred) {
          Block element;
          if (Block::fromBuffer(buffer, sizeof(buffer), element)) {
            this->receive(element);
          }
        }
        this->startReceive();
      });
}

void
UdpTransport::pause()
{
  BOOST_ASSERT_MSG(false, "not implemented");
}

void
UdpTransport::resume()
{
  BOOST_ASSERT_MSG(false, "not implemented");
}

} // namespace ndn
