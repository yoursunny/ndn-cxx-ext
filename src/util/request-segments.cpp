#include "request-segments.hpp"

namespace ndn {
namespace util {

class RequestSegments
{
public:
  RequestSegments(NackEnabledFace& face, const Name& baseName,
                  std::pair<uint64_t, uint64_t> segmentRange, const OnData& onData,
                  const std::function<void()>& onSuccess, const OnTimeout& onFail,
                  const AutoRetryDecision& retryDecision,
                  const time::milliseconds& retxInterval,
                  const EditInterest& editInterest);

private:
  void
  sendVersionDiscoveryInterest();

  void
  handleVersionDiscoveryData(Data& data);

  void
  sendInterest();

  void
  handleData(Data& data);

  void
  handleFail();

public:
  /** \brief retains a self pointer, to be deleted after onSuccess or onFail
   */
  unique_ptr<RequestSegments> self;

private:
  NackEnabledFace& m_face;
  Name m_baseName;
  Name m_versionedName;
  std::pair<uint64_t, uint64_t> m_segmentRange;
  uint64_t m_currentSegment;
  Interest m_interest;
  OnData m_onData;
  std::function<void()> m_onSuccess;
  OnTimeout m_onFail;
  AutoRetryDecision m_retryDecision;
  time::milliseconds m_retxInterval;
  EditInterest m_editInterest;
};


RequestSegments::RequestSegments(NackEnabledFace& face, const Name& baseName,
                                 std::pair<uint64_t, uint64_t> segmentRange, const OnData& onData,
                                 const std::function<void()>& onSuccess, const OnTimeout& onFail,
                                 const AutoRetryDecision& retryDecision,
                                 const time::milliseconds& retxInterval,
                                 const EditInterest& editInterest)
  : m_face(face)
  , m_baseName(baseName)
  , m_segmentRange(segmentRange)
  , m_currentSegment(segmentRange.first)
  , m_onData(onData)
  , m_onSuccess(onSuccess)
  , m_onFail(onFail)
  , m_retryDecision(retryDecision)
  , m_retxInterval(retxInterval)
  , m_editInterest(editInterest)
{
  if (!static_cast<bool>(m_onData))
    m_onData = bind([]{});
  if (!static_cast<bool>(m_onSuccess))
    m_onSuccess = bind([]{});
  if (!static_cast<bool>(m_onFail))
    m_onFail = bind([]{});
  if (!static_cast<bool>(m_editInterest))
    m_editInterest = bind([]{});

  bool hasKnownVersion = baseName.size() >= 1 && baseName.at(-1).isVersion();
  if (hasKnownVersion) {
    m_versionedName = baseName;
    this->sendInterest();
  }
  else {
    this->sendVersionDiscoveryInterest();
  }
}

void
RequestSegments::sendVersionDiscoveryInterest()
{
  m_interest = Interest(m_baseName);
  m_interest.setChildSelector(1);

  requestAutoRetry(m_face, m_interest,
                   bind(&RequestSegments::handleVersionDiscoveryData, this, _2),
                   bind(&RequestSegments::handleFail, this),
                   m_retryDecision, m_retxInterval);
}

void
RequestSegments::handleVersionDiscoveryData(Data& data)
{
  const Name& dataName = data.getName();
  bool hasSegment = dataName.size() >= 1 && dataName.at(-1).isSegment();
  if (!hasSegment) {
    this->handleFail();
    return;
  }

  m_versionedName = dataName.getPrefix(-1); // unversioned Names are permitted

  if (dataName.at(-1).toSegment() == m_segmentRange.first) {
    this->handleData(data);
  }
  else {
    this->sendInterest();
  }
}

void
RequestSegments::sendInterest()
{
  m_interest = Interest(Name(m_versionedName).appendSegment(m_currentSegment));
  m_editInterest(m_interest);

  requestAutoRetry(m_face, m_interest,
                   bind(&RequestSegments::handleData, this, _2),
                   bind(&RequestSegments::handleFail, this),
                   m_retryDecision, m_retxInterval);
}

void
RequestSegments::handleData(Data& data)
{
  BOOST_ASSERT(data.getName().size() == m_versionedName.size() + 1);
  const name::Component& segmentComponent = data.getName().at(-1);
  BOOST_ASSERT(segmentComponent.toSegment() == m_currentSegment);

  m_onData(m_interest, data);

  if (m_currentSegment == m_segmentRange.second ||
      data.getFinalBlockId() == segmentComponent) {
    m_onSuccess();
    self.reset();
    return;
  }
  ++m_currentSegment;
  this->sendInterest();
}

void
RequestSegments::handleFail()
{
  m_onFail(m_interest);
  self.reset();
}

void
requestSegments(NackEnabledFace& face, const Name& baseName,
                std::pair<uint64_t, uint64_t> segmentRange, const OnData& onData,
                const std::function<void()>& onSuccess, const OnTimeout& onFail,
                const AutoRetryDecision& retryDecision,
                const time::milliseconds& retxInterval,
                const EditInterest& editInterest)
{
  auto rs = new RequestSegments(face, baseName, segmentRange,
                                onData, onSuccess, onFail,
                                retryDecision, retxInterval, editInterest);
  rs->self.reset(rs);
}

} // namespace util
} // namespace ndn
