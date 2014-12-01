#include "nack-enabled-face.hpp"

#include "boost-test.hpp"
#include "face-pair-fixture.hpp"

namespace ndn {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestNackEnabledFace, FacePairFixture)

BOOST_AUTO_TEST_CASE(RequestData)
{
  bool hasData = false;
  face2.listen("ndn:/A", bind([this] { face2.reply(Data("ndn:/A/B/C")); }));

  face1.request(Interest("ndn:/A/B"),
                bind([&hasData] { hasData = true; }),
                bind([] { BOOST_ERROR("NACK"); }),
                bind([] { BOOST_ERROR("TIMEOUT"); }));

  io.poll();
  io.poll();
  BOOST_CHECK(hasData);
}

BOOST_AUTO_TEST_CASE(RequestNack)
{
  bool hasNack = false;
  face2.listen("ndn:/A", [this] (const Name& prefix, const Interest& interest) { face2.reply(Nack(Nack::BUSY, interest)); });

  face1.request(Interest("ndn:/A/B"),
                bind([] { BOOST_ERROR("NACK"); }),
                bind([&hasNack] { hasNack = true; }),
                bind([] { BOOST_ERROR("TIMEOUT"); }));

  io.poll();
  io.poll();
  BOOST_CHECK(hasNack);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
