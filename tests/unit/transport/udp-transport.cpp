#include "transport/udp-transport.hpp"

#include "boost-test.hpp"

namespace ndn {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestUdpTransport)

BOOST_AUTO_TEST_CASE(UdpPair)
{
  boost::asio::io_service io;
  UdpTransport transport1(ndn::util::FaceUri("udp4://127.0.0.1:4002"), 4001);
  UdpTransport transport2(ndn::util::FaceUri("udp4://127.0.0.1:4001"), 4002);

  transport1.connect(io, bind([]{}));
  transport2.connect(io, [&] (const Block& block) {
    BOOST_CHECK_EQUAL(block.type(), 0x01);
    io.stop();
  });

  Block block(0x01);
  block.encode();
  transport1.send(block);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
