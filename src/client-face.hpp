#ifndef NDNCXXEXT_CLIENT_FACE_HPP
#define NDNCXXEXT_CLIENT_FACE_HPP

#include "nack.hpp"
#include "util/scheduler.hpp"
#include <list>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn {

typedef function<void(const Interest&, const Nack&)> OnNack;

/** \brief NACK-enabled client face
 */
class ClientFace : noncopyable
{
public:
  ClientFace();

  virtual
  ~ClientFace();

  virtual util::SchedulerBase&
  getScheduler() = 0;

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

  util::signal::Signal<ClientFace, TraceEventKind, Interest, NackCode> trace;

protected: // receive path
  void
  receiveElement(const Block& block);

  void
  receiveInterestOrNack(const Interest& interestOrNack);

  void
  receiveInterest(const Interest& interest);

  void
  receiveData(const Data& data);

  void
  receiveNack(const Nack& nack);

private: // send path
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);

  virtual void
  sendNack(const Nack& nack);

  virtual void
  sendInterestOrNack(const Interest& interestOrNack);

  virtual void
  sendElement(const Block& block);

private: // management
  virtual void
  registerPrefix(const Name& prefix) = 0;

private:
  struct PendingInterest
  {
    Interest interest;
    OnData onData;
    OnNack onNack;
    OnTimeout onTimeout;
    util::SchedulerEventId timeoutEvent;
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

private:
  PendingInterestList m_pendingInterests;
  ListenerList m_listeners;
};

} // namespace ndn

#endif // NDNCXXEXT_CLIENT_FACE_HPP
