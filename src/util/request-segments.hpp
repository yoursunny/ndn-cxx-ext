#ifndef NDNCXXEXT_UTIL_REQUEST_SEGMENTS_HPP
#define NDNCXXEXT_UTIL_REQUEST_SEGMENTS_HPP

#include "request-auto-retry.hpp"

namespace ndn {
namespace util {

typedef std::function<void(Interest&)> EditInterest;

/** \brief send Interests to request segments
 *  \param baseName prefix of the segments;
 *                  if last component has no version marker, first Interest will have
 *                  ChildSelector=rightmost, and first segment will be requested again
 *  \param segmentRange segment number of first and last segment;
 *                      FinalBlockId will be honored
 *  \param onData invoked upon each Data arrival
 *  \param editInterest a hook for editing the Interest before it's sent
 */
void
requestSegments(NackEnabledFace& face, const Name& baseName,
                std::pair<uint64_t, uint64_t> segmentRange,
                const OnData& onData = nullptr,
                const std::function<void()>& onSuccess = nullptr,
                const OnTimeout& onFail = nullptr,
                const AutoRetryDecision& retryDecision = AutoRetryForever(),
                const EditInterest& editInterest = nullptr);

} // namespace util
} // namespace ndn

#endif // NDNCXXEXT_UTIL_REQUEST_SEGMENTS_HPP
