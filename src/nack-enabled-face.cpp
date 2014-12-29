#include "nack-enabled-face.hpp"
#include "util/logger.hpp"
#include "transport/udp-transport.hpp"
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
NackEnabledFace::listen(const Name& prefix, const OnInterest& onInterest, bool wantRegister)
{
  nfd::RibRegisterCommand command;
  static const Name COMMAND_PREFIX("/localhost/nfd");
  Interest requestInterest;

  if (wantRegister) {
    // register Interest prefix
    nfd::ControlParameters parameters;
    parameters.setName(prefix);
    requestInterest.setName(command.getRequestName(COMMAND_PREFIX, parameters));
    m_keyChain.sign(requestInterest);
    this->request(requestInterest, bind([]{}), bind([]{}), bind(&onRegisterFailure, prefix));
  }

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
NackEnabledFace::onInterestTimeout(PendingInterestList::iterator it)
{
  if (it->onTimeout) {
    it->onTimeout(it->interest);
  }
  m_pendingInterests.erase(it);
}

void
NackEnabledFace::request(const Interest& interest, const OnData& onData,
                         const OnNack& onNack, const OnTimeout& onTimeout)
{
  PendingInterestList::iterator it = m_pendingInterests.insert(m_pendingInterests.end(),
                                                               PendingInterest());
  PendingInterest& pi = *it;
  pi.interest = interest;
  pi.onData = onData;
  pi.onNack = onNack;
  pi.onTimeout = onTimeout;

  time::milliseconds timeout = interest.getInterestLifetime();
  if (timeout < time::milliseconds::zero()) {
    timeout = time::duration_cast<time::milliseconds>(DEFAULT_INTEREST_LIFETIME);
  }
  pi.timeoutEvent = m_scheduler.scheduleEvent(timeout,
                    bind(&NackEnabledFace::onInterestTimeout, this, it));

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
  for (Listener& listener : m_listeners) {
    if (listener.prefix.isPrefixOf(interest.getName())) {
      listener.onInterest(listener.prefix, interest);
      break;
    }
  }
}

void
NackEnabledFace::onReceiveData(const Data& data)
{
  PendingInterestList satisfied;
  m_pendingInterests.remove_if([&] (PendingInterest& pi) -> bool {
    if (!pi.interest.matchesData(data)) {
      return false;
    }
    m_scheduler.cancelEvent(pi.timeoutEvent);
    if (static_cast<bool>(pi.onData)) {
      satisfied.push_back(pi);
    }
    return true;
  });

  for (auto&& pi : satisfied) {
    pi.onData(pi.interest, const_cast<Data&>(data));
  }
}

void
NackEnabledFace::onReceiveNack(const Nack& nack)
{
  const Interest& i1 = nack.getInterest();
  PendingInterestList satisfied;
  m_pendingInterests.remove_if([&] (PendingInterest& pi) -> bool {
    const Interest& i2 = pi.interest;
    if (i1.getName() == i2.getName() && i1.getSelectors() == i2.getSelectors()) {
      m_scheduler.cancelEvent(pi.timeoutEvent);
      if (static_cast<bool>(pi.onNack)) {
        satisfied.push_back(pi);
      }
      return true;
    }
    return false;
  });

  // invoke callback after PI is deleted from m_pendingInterests,
  // otherwise if callback expresses new Interest, remove_if would be affected
  for (auto&& pi : satisfied) {
    pi.onNack(pi.interest, nack);
  }
}

} // namespace ndn
