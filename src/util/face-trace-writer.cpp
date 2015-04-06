#include "face-trace-writer.hpp"
#include "logger.hpp"

namespace ndn {
namespace util {

signal::Connection
FaceTraceWriter::connect(ClientFace& face)
{
  return face.trace.connect(&FaceTraceWriter::onTrace);
}

void
FaceTraceWriter::onTrace(ClientFace::TraceEventKind evt,
                         const Interest& interest, NackCode nackCode)
{
  switch (evt) {
  case ClientFace::TraceEventKind::INTEREST_TO:
    LOG("[FaceTrace] " << interest.getName() << " interestTo 0");
    break;
  case ClientFace::TraceEventKind::DATA_FROM:
    LOG("[FaceTrace] " << interest.getName() << " dataFrom 0");
    break;
  case ClientFace::TraceEventKind::NACK_FROM:
    LOG("[FaceTrace] " << interest.getName() << " nackFrom 0 " << nackCode);
    break;
  case ClientFace::TraceEventKind::TIMEOUT_FROM:
    LOG("[FaceTrace] " << interest.getName() << " timeoutFrom 0");
    break;
  case ClientFace::TraceEventKind::INTEREST_FROM:
    LOG("[FaceTrace] " << interest.getName() << " interestFrom 0");
    break;
  case ClientFace::TraceEventKind::DATA_TO:
    LOG("[FaceTrace] " << interest.getName() << " dataTo 0");
    break;
  case ClientFace::TraceEventKind::NACK_TO:
    LOG("[FaceTrace] " << interest.getName() << " nackTo 0 " << nackCode);
    break;
  }
}

} // namespace util
} // namespace ndn
