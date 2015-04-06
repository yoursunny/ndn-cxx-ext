#include "util/request-auto-retry.hpp"

#include "boost-test.hpp"
#include "../face-pair-fixture.hpp"

namespace ndn {
namespace tests {

using ndn::util::requestAutoRetry;
using ndn::util::AutoRetryLimited;

BOOST_FIXTURE_TEST_SUITE(TestRequestAutoRetry, FacePairFixture)

BOOST_AUTO_TEST_CASE(NackTwice)
{
  int nReceiveInterest = 0;
  face2.listen("ndn:/A", [this, &nReceiveInterest] (const Name& prefix, const Interest& interest) {
    if (++nReceiveInterest >= 3) {
      face2.reply(interest, Data("ndn:/A/B/C"));
    }
    else {
      face2.reply(interest, Nack(Nack::BUSY, interest));
    }
  });


  bool hasData = false;
  Interest interest("ndn:/A/B");
  requestAutoRetry(face1, Interest("ndn:/A/B"),
                   bind([&hasData] { hasData = true; }),
                   bind([] { BOOST_ERROR("FAIL"); }),
                   AutoRetryLimited(4), time::milliseconds::min(), time::milliseconds(3));

  face1.getScheduler().schedule(time::milliseconds(12), [&io] { io.stop(); });
  io.run();

  BOOST_CHECK(hasData);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
