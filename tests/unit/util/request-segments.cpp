#include "util/request-segments.hpp"

#include "boost-test.hpp"
#include "../face-pair-fixture.hpp"

namespace ndn {
namespace tests {

using ndn::util::requestSegments;
using ndn::util::AutoRetryLimited;

class RequestSegmentsProducerFixture : public FacePairFixture
{
protected:
  RequestSegmentsProducerFixture()
  {
    face2.listen("ndn:/A", [this] (const Name& prefix, const Interest& interest) {
      ++producerNInterests;
      const Name& interestName = interest.getName();
      Name dataName("ndn:/A/B/%FD%02");
      name::Component finalBlockId;
      if (interestName == "ndn:/A/B") {
        dataName.appendSegment(1);
      }
      else if (interestName.size() == 4 && interestName.at(-1).isSegment()) {
        dataName.append(interestName.at(-1));
        if (interestName.at(-1).toSegment() == 6) {
          finalBlockId = interestName.at(-1);
        }
      }
      else {
        face2.reply(interest, Nack(Nack::NODATA, interest));
        return;
      }
      Data data(dataName);
      data.setFinalBlockId(finalBlockId);
      face2.reply(interest, data);
    });
  }

protected:
  int producerNInterests;
};

BOOST_FIXTURE_TEST_SUITE(TestRequestSegments, RequestSegmentsProducerFixture)

BOOST_AUTO_TEST_CASE(AllSegments)
{
  bool isSuccess = false;
  int nData = 0;
  producerNInterests = 0;

  requestSegments(face1, "ndn:/A/B", {0, 9999},
                  bind([&nData] { ++nData; }),
                  bind([&isSuccess] { isSuccess = true; }),
                  bind([] { BOOST_ERROR("FAIL"); }),
                  AutoRetryLimited(3));

  io.poll();
  BOOST_CHECK(isSuccess);
  BOOST_CHECK_EQUAL(producerNInterests, 8);
  BOOST_CHECK_EQUAL(nData, 7);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
