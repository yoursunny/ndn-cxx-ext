#include "scheduler.hpp"
#include "logger.hpp"

namespace ndn {
namespace util {

SchedulerWrapper::SchedulerWrapper(boost::asio::io_service& io)
  : m_scheduler(io)
{
}

SchedulerEventId
SchedulerWrapper::schedule(const time::nanoseconds& after, const scheduler::Scheduler::Event& f)
{
  scheduler::EventId id = m_scheduler.scheduleEvent(after, f);
  auto id2 = make_shared<EventId>();
  id2->id = id;
  return id2;
}

void
SchedulerWrapper::cancel(const SchedulerEventId& id)
{
  auto id2 = std::static_pointer_cast<EventId>(id);
  m_scheduler.cancelEvent(id2->id);
}

} // namespace util
} // namespace ndn
