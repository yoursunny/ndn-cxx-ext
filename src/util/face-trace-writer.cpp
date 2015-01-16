#include "face-trace-writer.hpp"
#include "logger.hpp"

namespace ndn {
namespace util {

signal::Connection
FaceTraceWriter::connect(NackEnabledFace& face)
{
  return face.trace.connect(&FaceTraceWriter::onTrace);
}

void
FaceTraceWriter::onTrace(NackEnabledFace::TraceEventKind evt,
                         const Interest& interest, NackCode nackCode)
{
  switch (evt) {
  case NackEnabledFace::TraceEventKind::INTEREST_TO:
    LOG("[FaceTrace] " << interest.getName() << " interestTo 0");
    break;
  case NackEnabledFace::TraceEventKind::DATA_FROM:
    LOG("[FaceTrace] " << interest.getName() << " dataFrom 0");
    break;
  case NackEnabledFace::TraceEventKind::NACK_FROM:
    LOG("[FaceTrace] " << interest.getName() << " nackFrom 0 " << nackCode);
    break;
  case NackEnabledFace::TraceEventKind::TIMEOUT_FROM:
    LOG("[FaceTrace] " << interest.getName() << " timeoutFrom 0");
    break;
  case NackEnabledFace::TraceEventKind::INTEREST_FROM:
    LOG("[FaceTrace] " << interest.getName() << " interestFrom 0");
    break;
  case NackEnabledFace::TraceEventKind::DATA_TO:
    LOG("[FaceTrace] " << interest.getName() << " dataTo 0");
    break;
  case NackEnabledFace::TraceEventKind::NACK_TO:
    LOG("[FaceTrace] " << interest.getName() << " nackTo 0 " << nackCode);
    break;
  }
}

} // namespace util
} // namespace ndn
