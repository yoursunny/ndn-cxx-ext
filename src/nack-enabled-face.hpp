#ifndef NDNCXXEXT_NACK_ENABLED_FACE_HPP
#define NDNCXXEXT_NACK_ENABLED_FACE_HPP

#include "nack.hpp"
#include <list>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn {

typedef function<void(const Interest&, const Nack&)> OnNack;

/** \brief NACK-enabled client face
 */
class NackEnabledFace : noncopyable
{
public:
  explicit
  NackEnabledFace(boost::asio::io_service& io,
                  std::string endpoint = "");

  explicit
  NackEnabledFace(boost::asio::io_service& io,
                  unique_ptr<Transport> transport);

  ~NackEnabledFace();

  Scheduler&
  getScheduler()
  {
    return m_scheduler;
  }

public: // producer
  void
  listen(const Name& prefix, const OnInterest& onInterest, bool wantRegister = true);

  void
  reply(const Interest& interest, const Data& data);

  void
  reply(const Interest& interest, const Nack& nack);

  /** \brief whether to send NACK in response to unmatched Interest
   */
  bool shouldNackUnmatchedInterest;

public: // consumer
  void
  request(const Interest& interest, const OnData& onData,
          const OnNack& onNack, const OnTimeout& onTimeout,
          const time::milliseconds& timeoutOverride = time::milliseconds::min());

public: // trace
  enum class TraceEventKind {
    INTEREST_TO,
    DATA_FROM,
    NACK_FROM,
    TIMEOUT_FROM,
    INTEREST_FROM,
    DATA_TO,
    NACK_TO
  };

  util::signal::Signal<NackEnabledFace, TraceEventKind, Interest, NackCode> trace;

private:
  struct PendingInterest
  {
    Interest interest;
    OnData onData;
    OnNack onNack;
    OnTimeout onTimeout;
    EventId timeoutEvent;
  };
  typedef std::list<PendingInterest> PendingInterestList;

  struct Listener
  {
    Name prefix;
    OnInterest onInterest;
  };
  typedef std::list<Listener> ListenerList;

  void
  onInterestTimeout(PendingInterestList::iterator it);

  void
  onReceiveElement(const Block& block);

  void
  onReceiveInterest(const Interest& interest);

  void
  onReceiveData(const Data& data);

  void
  onReceiveNack(const Nack& nack);

private:
  boost::asio::io_service& m_io;
  boost::asio::io_service::work m_ioWork;
  Scheduler m_scheduler;
  unique_ptr<Transport> m_transport;
  KeyChain m_keyChain;

  PendingInterestList m_pendingInterests;
  ListenerList m_listeners;
};

} // namespace ndn

#endif // NDNCXXEXT_NACK_ENABLED_FACE_HPP
