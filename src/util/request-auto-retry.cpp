#include "request-auto-retry.hpp"

namespace ndn {
namespace util {

class RequestAutoRetry
{
public:
  RequestAutoRetry(NackEnabledFace& face, const Interest& interest,
                   const OnData& onData, const OnTimeout& onFail,
                   const AutoRetryDecision& retryDecision);

private:
  void
  sendInterest();

  void
  handleData(Data& data);

  void
  handleNack(const Nack& nack);

  void
  handleTimeout();

public:
  /** \brief retains a self pointer, to be deleted after onData or onFail
   */
  unique_ptr<RequestAutoRetry> self;

private:
  NackEnabledFace& m_face;
  int m_nSent;
  Interest m_interest;
  OnData m_onData;
  OnTimeout m_onFail;
  AutoRetryDecision m_retryDecision;
};


RequestAutoRetry::RequestAutoRetry(NackEnabledFace& face, const Interest& interest,
                                   const OnData& onData, const OnTimeout& onFail,
                                   const AutoRetryDecision& retryDecision)
  : m_face(face)
  , m_nSent(0)
  , m_interest(interest)
  , m_onData(onData)
  , m_onFail(onFail)
  , m_retryDecision(retryDecision)
{
  m_interest.setNonce(1);
  this->sendInterest();
}

void
RequestAutoRetry::sendInterest()
{
  m_interest.refreshNonce();
  BOOST_ASSERT(m_interest.hasNonce());

  ++m_nSent;
  m_face.request(m_interest,
                 bind(&RequestAutoRetry::handleData, this, _2),
                 bind(&RequestAutoRetry::handleNack, this, _2),
                 bind(&RequestAutoRetry::handleTimeout, this));
}

void
RequestAutoRetry::handleData(Data& data)
{
  m_onData(m_interest, data);
  self.reset();
}

void
RequestAutoRetry::handleNack(const Nack& nack)
{
  if (m_retryDecision(m_nSent, false, nack.getCode())) {
    this->sendInterest();
  }
  else {
    m_onFail(m_interest);
    self.reset();
  }
}

void
RequestAutoRetry::handleTimeout()
{
  if (m_retryDecision(m_nSent, true, Nack::NONE)) {
    this->sendInterest();
  }
  else {
    m_onFail(m_interest);
    self.reset();
  }
}

void
requestAutoRetry(NackEnabledFace& face, const Interest& interest,
                 const OnData& onData, const OnTimeout& onFail,
                 const AutoRetryDecision& retryDecision)
{
  auto rar = new RequestAutoRetry(face, interest, onData, onFail, retryDecision);
  rar->self.reset(rar);
}

} // namespace util
} // namespace ndn
