#ifndef NDNCXXEXT_STANDALONE_CLIENT_FACE_HPP
#define NDNCXXEXT_STANDALONE_CLIENT_FACE_HPP

#include "client-face.hpp"

namespace ndn {

/** \brief Boost.Asio based ClientFace
 */
class StandaloneClientFace : public ClientFace
{
public:
  explicit
  StandaloneClientFace(boost::asio::io_service& io,
                       std::string endpoint = "");

  explicit
  StandaloneClientFace(boost::asio::io_service& io,
                       unique_ptr<Transport> transport);

  virtual
  ~StandaloneClientFace();

  virtual util::SchedulerBase&
  getScheduler() NDNCXXEXT_DECL_OVERRIDE;

private:
  virtual void
  sendElement(const Block& block) NDNCXXEXT_DECL_OVERRIDE;

  virtual void
  registerPrefix(const Name& prefix) NDNCXXEXT_DECL_OVERRIDE;

private:
  boost::asio::io_service& m_io;
  boost::asio::io_service::work m_ioWork;
  util::SchedulerWrapper m_scheduler;
  unique_ptr<Transport> m_transport;
  KeyChain m_keyChain;
};

} // namespace ndn

#endif // NDNCXXEXT_STANDALONE_CLIENT_FACE_HPP
