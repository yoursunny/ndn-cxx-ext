#ifndef NDNCXXEXT_UTIL_FACE_TRACE_WRITER_HPP
#define NDNCXXEXT_UTIL_FACE_TRACE_WRITER_HPP

#include "common.hpp"
#include "../client-face.hpp"

namespace ndn {
namespace util {

class FaceTraceWriter : noncopyable
{
public:
  static signal::Connection
  connect(ClientFace& face);

private:
  static void
  onTrace(ClientFace::TraceEventKind evt, const Interest& interest, NackCode nackCode);
};

} // namespace util
} // namespace ndn

#endif // NDNCXXEXT_UTIL_FACE_TRACE_WRITER_HPP
