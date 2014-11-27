#include "nack.hpp"

#include "boost-test.hpp"

namespace ndn {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestNack)

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  Interest interest1("ndn:/A/1");
  interest1.setMaxSuffixComponents(5);
  interest1.setNonce(0x4444);

  Nack nack1(Nack::DUPLICATE, interest1);

  Interest encoded = nack1.encode();

  Nack nack2;
  BOOST_REQUIRE(nack2.decode(encoded));
  BOOST_CHECK_EQUAL(nack2.getCode(), Nack::DUPLICATE);

  const Interest& interest2 = nack2.getInterest();
  BOOST_CHECK_EQUAL(interest2.getName(), Name("ndn:/A/1"));
  BOOST_CHECK_EQUAL(interest2.getMaxSuffixComponents(), 5);
  BOOST_CHECK_EQUAL(interest2.getNonce(), 0x4444);
}

BOOST_AUTO_TEST_CASE(EncodeDecode2)
{
  Interest interest1("ndn:/");
  interest1.setNonce(0x5555);

  Nack nack1(Nack::GIVEUP, interest1);

  Interest encoded = nack1.encode();

  Nack nack2;
  BOOST_REQUIRE(nack2.decode(encoded));
  BOOST_CHECK_EQUAL(nack2.getCode(), Nack::GIVEUP);

  const Interest& interest2 = nack2.getInterest();
  BOOST_CHECK_EQUAL(interest2.getName(), Name("ndn:/"));
  BOOST_CHECK_EQUAL(interest2.getSelectors().empty(), true);
  BOOST_CHECK_EQUAL(interest2.getNonce(), 0x5555);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
