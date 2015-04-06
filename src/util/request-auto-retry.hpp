#ifndef NDNCXXEXT_UTIL_REQUEST_AUTO_RETRY_HPP
#define NDNCXXEXT_UTIL_REQUEST_AUTO_RETRY_HPP

#include "common.hpp"
#include "../client-face.hpp"

namespace ndn {
namespace util {

typedef std::function<bool(int nSent, bool isTimeout, NackCode nackCode)> AutoRetryDecision;

class AutoRetryForever
{
public:
  bool
  operator()(int nSent, bool isTimeout, NackCode nackCode)
  {
    return true;
  }
};

class AutoRetryLimited
{
public:
  AutoRetryLimited(int nMaxSent)
    : m_nMaxSent(nMaxSent)
  {
  }

  bool
  operator()(int nSent, bool isTimeout, NackCode nackCode)
  {
    return nSent < m_nMaxSent;
  }

private:
  int m_nMaxSent;
};

/** \brief send an Interest repeatedly until a Data comes back
 */
void
requestAutoRetry(ClientFace& face, const Interest& interest,
                 const OnData& onData, const OnTimeout& onFail = nullptr,
                 const AutoRetryDecision& retryDecision = AutoRetryForever(),
                 const time::milliseconds& retxInterval = time::milliseconds::min(),
                 const time::milliseconds& nackRetxDelay = time::milliseconds(200));

} // namespace util
} // namespace ndn

#endif // NDNCXXEXT_UTIL_REQUEST_AUTO_RETRY_HPP
