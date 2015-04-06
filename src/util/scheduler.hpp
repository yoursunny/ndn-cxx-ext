#ifndef NDNCXXEXT_UTIL_SCHEDULER_HPP
#define NDNCXXEXT_UTIL_SCHEDULER_HPP

#include "common.hpp"

namespace ndn {
namespace util {

struct SchedulerEventIdBase
{
};

typedef shared_ptr<SchedulerEventIdBase> SchedulerEventId;

/** \brief declares a scheduler
 */
class SchedulerBase : noncopyable
{
public:
  virtual SchedulerEventId
  schedule(const time::nanoseconds& after, const scheduler::Scheduler::Event& f) = 0;

  virtual void
  cancel(const SchedulerEventId& id) = 0;
};

/** \brief wraps ndn::util::scheduler::Scheduler as SchedulerBase
 */
class SchedulerWrapper : public SchedulerBase
{
public:
  SchedulerWrapper(boost::asio::io_service& io);

  virtual SchedulerEventId
  schedule(const time::nanoseconds& after,
           const scheduler::Scheduler::Event& f) NDNCXXEXT_DECL_OVERRIDE;

  virtual void
  cancel(const SchedulerEventId& id) NDNCXXEXT_DECL_OVERRIDE;

private:
  Scheduler m_scheduler;

  struct EventId : public SchedulerEventIdBase
  {
    scheduler::EventId id;
  };
};

} // namespace util
} // namespace ndn

#endif // NDNCXXEXT_UTIL_SCHEDULER_HPP
