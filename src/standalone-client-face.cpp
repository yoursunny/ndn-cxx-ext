#include "standalone-client-face.hpp"
#include "util/logger.hpp"
#include "transport/udp-transport.hpp"
#include <ndn-cxx/transport/tcp-transport.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-command-options.hpp>

namespace ndn {

StandaloneClientFace::StandaloneClientFace(boost::asio::io_service& io, std::string endpoint)
  : m_io(io)
  , m_ioWork(io)
  , m_scheduler(io)
{
  if (endpoint.empty()) {
    char* endpointEnv = getenv("FACE_ENDPOINT");
    if (endpointEnv != nullptr) {
      endpoint = endpointEnv;
    }
    if (endpoint.empty()) {
      endpoint = "tcp4://127.0.0.1:6363";
    }
  }
  ndn::util::FaceUri faceUri(endpoint);
  if (faceUri.getScheme() == "tcp4") {
    m_transport.reset(new TcpTransport(faceUri.getHost(), faceUri.getPort()));
  }
  else if (faceUri.getScheme() == "udp4") {
    m_transport.reset(new UdpTransport(faceUri));
  }
  m_transport->connect(io, bind(&StandaloneClientFace::receiveElement, this, _1));
}

StandaloneClientFace::StandaloneClientFace(boost::asio::io_service& io,
                                           unique_ptr<Transport> transport)
  : m_io(io)
  , m_ioWork(io)
  , m_scheduler(io)
  , m_transport(std::move(transport))
{
  m_transport->connect(io, bind(&StandaloneClientFace::receiveElement, this, _1));
}

StandaloneClientFace::~StandaloneClientFace()
{
}

util::SchedulerBase&
StandaloneClientFace::getScheduler()
{
  return m_scheduler;
}

void
StandaloneClientFace::sendElement(const Block& block)
{
  m_transport->send(block);
}

static inline void
onRegisterFailure(const Name& prefix)
{
  LOG("RibRegister timeout " << prefix);
}

void
StandaloneClientFace::registerPrefix(const Name& prefix)
{
  nfd::ControlParameters parameters;
  parameters.setName(prefix);

  nfd::RibRegisterCommand command;
  static const Name COMMAND_PREFIX("/localhost/nfd");

  Interest requestInterest(command.getRequestName(COMMAND_PREFIX, parameters));
  m_keyChain.sign(requestInterest);

  this->request(requestInterest, bind([]{}), bind([]{}), bind(&onRegisterFailure, prefix));
}

} // namespace ndn
