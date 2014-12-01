#include "nack-enabled-face.hpp"
#include "util/logger.hpp"
#include <ndn-cxx/transport/tcp-transport.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-command-options.hpp>

namespace ndn {

NackEnabledFace::NackEnabledFace(boost::asio::io_service& io, std::string endpoint)
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
      endpoint = "127.0.0.1:6363";
    }
  }
  // endpoint must have form x.x.x.x:p
  std::string host = endpoint.substr(0, endpoint.find(':'));
  std::string port = endpoint.substr(endpoint.find(':') + 1);
  m_transport.reset(new TcpTransport(host, port));
  m_transport->connect(io, bind(&NackEnabledFace::onReceiveElement, this, _1));
}

NackEnabledFace::NackEnabledFace(boost::asio::io_service& io,
                                 unique_ptr<Transport> transport)
  : m_io(io)
  , m_ioWork(io)
  , m_scheduler(io)
  , m_transport(std::move(transport))
{
  m_transport->connect(io, bind(&NackEnabledFace::onReceiveElement, this, _1));
}

NackEnabledFace::~NackEnabledFace()
{
}

static inline void
onRegisterFailure(const Name& prefix)
{
  LOG("RibRegister timeout " << prefix);
}

void
NackEnabledFace::listen(const Name& prefix, const OnInterest& onInterest)
{
  nfd::RibRegisterCommand command;
  static const Name COMMAND_PREFIX("/localhop/nfd");
  Interest requestInterest;

  // register Interest prefix
  nfd::ControlParameters parameters;
  parameters.setName(prefix);
  requestInterest.setName(command.getRequestName(COMMAND_PREFIX, parameters));
  m_keyChain.sign(requestInterest);
  this->request(requestInterest, bind([]{}), bind([]{}), bind(&onRegisterFailure, prefix));

  m_listeners.push_back(Listener());
  Listener& listener = m_listeners.back();
  listener.prefix = prefix;
  listener.onInterest = onInterest;
}

void
NackEnabledFace::reply(const Data& data)
{
  if (!data.getSignature()) {
    // add fake signature
    ndn::SignatureSha256WithRsa fakeSignature;
    fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                          static_cast<const uint8_t*>(nullptr), 0));
    const_cast<Data&>(data).setSignature(fakeSignature);
  }
  m_transport->send(data.wireEncode());
}

void
NackEnabledFace::reply(const Nack& nack)
{
  Interest encoded = nack.encode();
  m_transport->send(encoded.wireEncode());
}

void
NackEnabledFace::onInterestTimeout(int sequence)
{
  for (std::list<PendingInterest>::iterator it = m_pendingInterests.begin();
       it != m_pendingInterests.end(); ++it) {
    if (it->sequence == sequence) {
      m_pendingInterests.erase(it);
      return;
    }
  }
  BOOST_ASSERT(false);
}

void
NackEnabledFace::request(const Interest& interest, const OnData& onData,
                         const OnNack& onNack, const OnTimeout& onTimeout)
{
  static int sequence = 0;

  m_pendingInterests.push_back(PendingInterest());
  PendingInterest& pi = m_pendingInterests.back();
  pi.sequence = ++sequence;
  pi.interest = interest;
  pi.onData = onData;
  pi.onNack = onNack;

  time::milliseconds timeout = interest.getInterestLifetime();
  if (timeout < time::milliseconds::zero()) {
    timeout = time::duration_cast<time::milliseconds>(DEFAULT_INTEREST_LIFETIME);
  }
  pi.timeoutEvent = m_scheduler.scheduleEvent(timeout,
                    bind(&NackEnabledFace::onInterestTimeout, this, pi.sequence));

  m_transport->send(interest.wireEncode());
}

void
NackEnabledFace::onReceiveElement(const Block& block)
{
  if (block.type() == tlv::Interest) {
    Interest interest(block);

    Nack nack;
    if (nack.decode(interest)) {
      this->onReceiveNack(nack);
    }
    else {
      this->onReceiveInterest(interest);
    }
  }
  else if (block.type() == tlv::Data) {
    Data data(block);
    this->onReceiveData(data);
  }
}

void
NackEnabledFace::onReceiveInterest(const Interest& interest)
{
  for (std::list<Listener>::iterator it = m_listeners.begin();
       it != m_listeners.end(); ++it) {
    if (it->prefix.isPrefixOf(interest.getName())) {
      it->onInterest(it->prefix, interest);
      break;
    }
  }
}

void
NackEnabledFace::onReceiveData(const Data& data)
{
  m_pendingInterests.remove_if([&] (PendingInterest& pi) {
    if (!pi.interest.matchesData(data)) {
      return false;
    }
    m_scheduler.cancelEvent(pi.timeoutEvent);
    if (static_cast<bool>(pi.onData)) {
      pi.onData(pi.interest, const_cast<Data&>(data));
    }
    return true;
  });
}

void
NackEnabledFace::onReceiveNack(const Nack& nack)
{
  const Interest& i1 = nack.getInterest();
  m_pendingInterests.remove_if([&] (PendingInterest& pi) {
    const Interest& i2 = pi.interest;
    if (i1.getName() == i2.getName() && i1.getSelectors() == i2.getSelectors()) {
      m_scheduler.cancelEvent(pi.timeoutEvent);
      if (static_cast<bool>(pi.onNack)) {
        pi.onNack(pi.interest, nack);
      }
      return true;
    }
    return false;
  });
}

} // namespace ndn
