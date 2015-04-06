#include "client-face.hpp"
#include "util/logger.hpp"

namespace ndn {

ClientFace::ClientFace()
  : shouldNackUnmatchedInterest(false)
{
}

ClientFace::~ClientFace()
{
}

void
ClientFace::listen(const Name& prefix, const OnInterest& onInterest, bool wantRegister)
{
  if (wantRegister) {
    this->registerPrefix(prefix);
  }

  m_listeners.push_back(Listener());
  Listener& listener = m_listeners.back();
  listener.prefix = prefix;
  listener.onInterest = onInterest;
}

void
ClientFace::reply(const Interest& interest, const Data& data)
{
  if (!data.getSignature()) {
    // add fake signature
    ndn::SignatureSha256WithRsa fakeSignature;
    fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                          static_cast<const uint8_t*>(nullptr), 0));
    const_cast<Data&>(data).setSignature(fakeSignature);
  }

  this->sendData(data);
  this->trace(TraceEventKind::DATA_TO, interest, Nack::NONE);
}

void
ClientFace::reply(const Interest& interest, const Nack& nack)
{
  this->sendNack(nack);
  this->trace(TraceEventKind::NACK_TO, interest, nack.getCode());
}

void
ClientFace::onInterestTimeout(PendingInterestList::iterator it)
{
  this->trace(TraceEventKind::TIMEOUT_FROM, it->interest, Nack::NONE);
  if (it->onTimeout) {
    it->onTimeout(it->interest);
  }
  m_pendingInterests.erase(it);
}

void
ClientFace::request(const Interest& interest, const OnData& onData,
                         const OnNack& onNack, const OnTimeout& onTimeout,
                         const time::milliseconds& timeoutOverride)
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
  if (timeoutOverride > time::milliseconds::zero() && timeoutOverride < timeout) {
    // XXX hack! allow library and network use different timeout,
    //     so that caller doesn't have to manage retx
    timeout = timeoutOverride;
  }
  pi.timeoutEvent = this->getScheduler().schedule(timeout,
                    bind(&ClientFace::onInterestTimeout, this, it));

  this->sendInterest(interest);
  this->trace(TraceEventKind::INTEREST_TO, interest, Nack::NONE);
}

void
ClientFace::receiveElement(const Block& block)
{
  if (block.type() == tlv::Interest) {
    Interest interest(block);

    Nack nack;
    if (nack.decode(interest)) {
      this->receiveNack(nack);
    }
    else {
      this->receiveInterest(interest);
    }
  }
  else if (block.type() == tlv::Data) {
    Data data(block);
    this->receiveData(data);
  }
}

void
ClientFace::receiveInterestOrNack(const Interest& interest)
{
}

void
ClientFace::receiveInterest(const Interest& interest)
{
  this->trace(TraceEventKind::INTEREST_FROM, interest, Nack::NONE);
  for (Listener& listener : m_listeners) {
    if (listener.prefix.isPrefixOf(interest.getName())) {
      listener.onInterest(listener.prefix, interest);
      return;
    }
  }
  if (this->shouldNackUnmatchedInterest) {
    this->reply(interest, Nack(Nack::NODATA, interest));
  }
}

void
ClientFace::receiveData(const Data& data)
{
  PendingInterestList satisfied;
  m_pendingInterests.remove_if([&] (PendingInterest& pi) -> bool {
    if (!pi.interest.matchesData(data)) {
      return false;
    }
    this->getScheduler().cancel(pi.timeoutEvent);
    if (static_cast<bool>(pi.onData)) {
      satisfied.push_back(pi);
    }
    return true;
  });

  for (auto&& pi : satisfied) {
    this->trace(TraceEventKind::DATA_FROM, pi.interest, Nack::NONE);
    pi.onData(pi.interest, const_cast<Data&>(data));
  }
}

void
ClientFace::receiveNack(const Nack& nack)
{
  const Interest& i1 = nack.getInterest();
  PendingInterestList satisfied;
  m_pendingInterests.remove_if([&] (PendingInterest& pi) -> bool {
    const Interest& i2 = pi.interest;
    if (i1.getName() == i2.getName() && i1.getSelectors() == i2.getSelectors()) {
      this->getScheduler().cancel(pi.timeoutEvent);
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
    this->trace(TraceEventKind::NACK_FROM, pi.interest, nack.getCode());
    pi.onNack(pi.interest, nack);
  }
}

void
ClientFace::sendInterest(const Interest& interest)
{
  this->sendInterestOrNack(interest);
}

void
ClientFace::sendData(const Data& data)
{
  this->sendElement(data.wireEncode());
}

void
ClientFace::sendNack(const Nack& nack)
{
  this->sendInterestOrNack(nack.encode());
}

void
ClientFace::sendInterestOrNack(const Interest& interestOrNack)
{
  this->sendElement(interestOrNack.wireEncode());
}

void
ClientFace::sendElement(const Block& block)
{
  BOOST_ASSERT(false); // override this, or sendInterestOrNack+sendData,
                       // or sendInterest+sendData+sendNack
}

} // namespace ndn
