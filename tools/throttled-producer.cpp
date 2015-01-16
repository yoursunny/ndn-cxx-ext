/**
 *  throttled-producer is a producer that simulates server processing time.
 *  It has a RandomEarlyNack mechanism, which starts to NACK requests if queue is somewhat full.
 *
 *  The last Name component of each request Interest must be ASCII-number x,
 *  where (x >> 48) gives the processing duration of the request.
 */

#include "nack-enabled-face.hpp"
#include "util/logger.hpp"
#include <queue>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace ndn {
namespace throttled_producer {

class RandomEarlyNack
{
public:
  RandomEarlyNack(size_t minThres, size_t maxThres)
    : m_dist(minThres, maxThres)
  {
  }

  bool
  shouldAccept(size_t queueLength)
  {
    return queueLength < m_dist(s_gen);
  }

private:
  static boost::random::mt19937 s_gen;
  boost::random::uniform_int_distribution<size_t> m_dist;
};
boost::random::mt19937 RandomEarlyNack::s_gen;

class ThrottledProducer : noncopyable
{
public:
  ThrottledProducer(NackEnabledFace& face, Scheduler& scheduler,
                    const Name& prefix, RandomEarlyNack& ren)
    : m_face(face)
    , m_scheduler(scheduler)
    , m_ren(ren)
  {
    face.listen(prefix, bind(&ThrottledProducer::onInterest, this, std::placeholders::_2));
  }

private:
  void
  onInterest(const Interest& interest)
  {
    if (!m_ren.shouldAccept(m_queue.size())) {
      LOG("REJECT " << interest.getName() << " queue=" << m_queue.size());
      Nack nack(Nack::BUSY, interest);
      m_face.reply(interest, nack);
      return;
    }
    LOG("ACCEPT " << interest.getName() << " queue=" << m_queue.size());

    bool isIdle = m_queue.empty();
    m_queue.push(interest);
    if (isIdle) {
      this->startJob();
    }
  }

  time::milliseconds
  extractDuration(const Name& name)
  {
    uint64_t x = boost::lexical_cast<uint64_t>(name.at(-1).toUri());
    return time::milliseconds(x >> 48);
  }

  void
  startJob()
  {
    BOOST_ASSERT(!m_queue.empty());
    const Name& name = m_queue.front().getName();
    time::milliseconds duration = this->extractDuration(name);
    LOG("START " << name << " duration=" << duration.count());

    m_scheduler.scheduleEvent(duration, bind(&ThrottledProducer::onJobFinish, this));
  }

  void
  onJobFinish()
  {
    BOOST_ASSERT(!m_queue.empty());
    LOG("FINISH " << m_queue.front().getName() << " queue=" << (m_queue.size() - 1));

    Data data(m_queue.front().getName());
    m_keyChain.signWithSha256(data);
    m_face.reply(m_queue.front(), data);
    m_queue.pop();

    if (m_queue.empty()) {
      LOG("IDLE");
      return;
    }
    this->startJob();
  }

private:
  NackEnabledFace& m_face;
  KeyChain m_keyChain;
  Scheduler& m_scheduler;
  std::queue<Interest> m_queue;
  RandomEarlyNack& m_ren;
};

int
main(int argc, char* argv[])
{
  if (argc != 4) {
    std::cerr << "USAGE: ./throttled-producer ndn:/name minThres maxThres" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  NackEnabledFace face(io);
  Scheduler scheduler(io);
  RandomEarlyNack ren(boost::lexical_cast<size_t>(argv[2]), boost::lexical_cast<size_t>(argv[3]));

  ThrottledProducer tp(face, scheduler, Name(argv[1]), ren);
  io.run();

  return 0;
}

} // namespace ndn
} // namespace throttled_producer

int
main(int argc, char* argv[])
{
  return ndn::throttled_producer::main(argc, argv);
}
