#ifndef NDNCXXEXT_TESTS_TRANSPORT_PAIR_HPP
#define NDNCXXEXT_TESTS_TRANSPORT_PAIR_HPP

#include "common.hpp"
#include <ndn-cxx/transport/transport.hpp>

namespace ndn {
namespace tests {

class PairedTransport : public Transport
{
public:
  virtual void
  connect(boost::asio::io_service& io_service,
          const ReceiveCallback& receiveCallback)
  {
    m_io = &io_service;
    m_receive = receiveCallback;
  }

  virtual void
  close()
  {
  }

  virtual void
  send(const Block& wire)
  {
    BOOST_ASSERT(m_other != nullptr);
    m_io->post([=] { m_other->m_receive(wire); });
  }

  virtual void
  send(const Block& header, const Block& payload)
  {
    BOOST_ASSERT_MSG(false, "not implemented");
  }

  virtual void
  pause()
  {
    BOOST_ASSERT_MSG(false, "not implemented");
  }

  virtual void
  resume()
  {
    BOOST_ASSERT_MSG(false, "not implemented");
  }

private:
  /** \brief construct first transport in pair
   */
  PairedTransport()
  {
  }

  /** \brief construct second transport in pair
   */
  explicit
  PairedTransport(PairedTransport* other)
    : m_other(other)
  {
    m_other->m_other = this;
  }

  friend std::pair<unique_ptr<Transport>, unique_ptr<Transport>>
  makeTransportPair();

private:
  boost::asio::io_service* m_io;
  PairedTransport* m_other;
  ReceiveCallback m_receive;
};

inline std::pair<unique_ptr<Transport>, unique_ptr<Transport>>
makeTransportPair()
{
  auto first = new PairedTransport();
  auto second = new PairedTransport(first);
  return {unique_ptr<Transport>(first), unique_ptr<Transport>(second)};
}

} // namespace tests
} // namespace ndn

#endif // NDNCXXEXT_TESTS_TRANSPORT_PAIR_HPP
